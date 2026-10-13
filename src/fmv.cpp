#include "assets/assets.hpp"
#include <SDL.h>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}
#include <algorithm>
#include <cstdio>
#include <deque>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace chase;

constexpr int audioRate = 22050;
constexpr int audioFrameBytes = 4;
constexpr double audioLeadSeconds = 0.4;
constexpr int maxPendingFrames = 24;

struct Options {
    std::string video, subtitleFile, language, screenshot;
    std::string fib = "artifacts/romfs/romfs/lego_city3ds.fib", names = "artifacts/romfs/romfs/lego_city3ds.csv";
    std::string fontName = "frontendfont";
    double time = 0;
    bool audio = true;
};

template <typename T, void (*Free)(T**)> struct Deleter {
    void operator()(T* pointer) const { Free(&pointer); }
};
using FormatContext = std::unique_ptr<AVFormatContext, Deleter<AVFormatContext, avformat_close_input>>;
using CodecContext = std::unique_ptr<AVCodecContext, Deleter<AVCodecContext, avcodec_free_context>>;
using Packet = std::unique_ptr<AVPacket, Deleter<AVPacket, av_packet_free>>;
using Frame = std::unique_ptr<AVFrame, Deleter<AVFrame, av_frame_free>>;
using Resampler = std::unique_ptr<SwrContext, Deleter<SwrContext, swr_free>>;
struct ScalerFree {
    void operator()(SwsContext* c) const { sws_freeContext(c); }
};
using Scaler = std::unique_ptr<SwsContext, ScalerFree>;

[[noreturn]] void fail(const std::string& what, int code) {
    char text[AV_ERROR_MAX_STRING_SIZE] = {};
    av_strerror(code, text, sizeof text);
    throw std::runtime_error(what + ": " + text);
}

CodecContext openDecoder(AVStream* stream) {
    const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) throw std::runtime_error("no decoder for stream " + std::to_string(stream->index));
    CodecContext context(avcodec_alloc_context3(codec));
    avcodec_parameters_to_context(context.get(), stream->codecpar);
    int result = avcodec_open2(context.get(), codec, nullptr);
    if (result < 0) fail("cannot open decoder", result);
    return context;
}

struct VideoFrame {
    double time = 0;
    std::vector<std::uint8_t> planes;
};

class Movie {
public:
    explicit Movie(const std::string& path) {
        AVFormatContext* raw = nullptr;
        int result = avformat_open_input(&raw, path.c_str(), nullptr, nullptr);
        if (result < 0) fail("cannot open " + path, result);
        format_.reset(raw);
        result = avformat_find_stream_info(raw, nullptr);
        if (result < 0) fail("cannot read stream info", result);
        videoIndex_ = av_find_best_stream(raw, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        audioIndex_ = av_find_best_stream(raw, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
        if (videoIndex_ < 0) throw std::runtime_error("no video stream in " + path);
        video_ = openDecoder(raw->streams[videoIndex_]);
        width_ = video_->width;
        height_ = video_->height;
        if (audioIndex_ >= 0) {
            audio_ = openDecoder(raw->streams[audioIndex_]);
            AVChannelLayout stereo = AV_CHANNEL_LAYOUT_STEREO;
            SwrContext* swr = nullptr;
            result = swr_alloc_set_opts2(&swr, &stereo, AV_SAMPLE_FMT_S16, audioRate, &audio_->ch_layout,
                                         audio_->sample_fmt, audio_->sample_rate, 0, nullptr);
            if (result < 0) fail("cannot create resampler", result);
            resampler_.reset(swr);
            result = swr_init(swr);
            if (result < 0) fail("cannot initialise resampler", result);
        }
        packet_.reset(av_packet_alloc());
        frame_.reset(av_frame_alloc());
    }

    int width() const { return width_; }
    int height() const { return height_; }
    bool hasAudio() const { return audioIndex_ >= 0; }
    bool finished() const { return eof_; }

    bool readMore(std::deque<VideoFrame>& video, std::vector<std::uint8_t>& audio) {
        if (eof_) return false;
        int result = av_read_frame(format_.get(), packet_.get());
        if (result < 0) {
            eof_ = true;
            drain(video_.get(), videoIndex_, nullptr, &video, audio);
            if (audio_) drain(audio_.get(), audioIndex_, nullptr, &video, audio);
            return false;
        }
        int stream = packet_->stream_index;
        if (stream == videoIndex_) feed(video_.get(), videoIndex_, &video, audio);
        else if (stream == audioIndex_ && audio_) feed(audio_.get(), audioIndex_, &video, audio);
        av_packet_unref(packet_.get());
        return true;
    }

private:
    void feed(AVCodecContext* codec, int stream, std::deque<VideoFrame>* video, std::vector<std::uint8_t>& audio) {
        int result = avcodec_send_packet(codec, packet_.get());
        if (result < 0 && result != AVERROR(EAGAIN)) fail("decode error", result);
        drain(codec, stream, packet_.get(), video, audio);
    }

    void drain(AVCodecContext* codec, int stream, AVPacket* packet, std::deque<VideoFrame>* video,
               std::vector<std::uint8_t>& audio) {
        if (!packet) avcodec_send_packet(codec, nullptr);
        while (avcodec_receive_frame(codec, frame_.get()) >= 0) {
            if (stream == videoIndex_) store(*video);
            else convertAudio(audio);
            av_frame_unref(frame_.get());
        }
    }

    double seconds(std::int64_t pts, int stream) const {
        AVStream* s = format_->streams[stream];
        std::int64_t origin = s->start_time == AV_NOPTS_VALUE ? 0 : s->start_time;
        return double(pts - origin) * av_q2d(s->time_base);
    }

    void store(std::deque<VideoFrame>& video) {
        if (!scaler_) {
            scaler_.reset(sws_getContext(width_, height_, AVPixelFormat(frame_->format), width_, height_,
                                         AV_PIX_FMT_YUV420P, SWS_BILINEAR, nullptr, nullptr, nullptr));
            if (!scaler_) throw std::runtime_error("cannot create video scaler");
        }
        VideoFrame out;
        std::int64_t pts = frame_->best_effort_timestamp;
        out.time = pts == AV_NOPTS_VALUE ? (video.empty() ? 0 : video.back().time + 1.0 / 30) : seconds(pts, videoIndex_);
        int luma = width_ * height_, chroma = ((width_ + 1) / 2) * ((height_ + 1) / 2);
        out.planes.resize(std::size_t(luma) + 2 * std::size_t(chroma));
        std::uint8_t* destination[4] = {out.planes.data(), out.planes.data() + luma,
                                        out.planes.data() + luma + chroma, nullptr};
        int strides[4] = {width_, (width_ + 1) / 2, (width_ + 1) / 2, 0};
        sws_scale(scaler_.get(), frame_->data, frame_->linesize, 0, height_, destination, strides);
        video.push_back(std::move(out));
    }

    void convertAudio(std::vector<std::uint8_t>& audio) {
        int capacity = swr_get_out_samples(resampler_.get(), frame_->nb_samples);
        std::size_t before = audio.size();
        audio.resize(before + std::size_t(capacity) * audioFrameBytes);
        std::uint8_t* out = audio.data() + before;
        int produced = swr_convert(resampler_.get(), &out, capacity,
                                   const_cast<const std::uint8_t**>(frame_->extended_data), frame_->nb_samples);
        if (produced < 0) fail("resample error", produced);
        audio.resize(before + std::size_t(produced) * audioFrameBytes);
    }

    FormatContext format_;
    CodecContext video_, audio_;
    Resampler resampler_;
    Scaler scaler_;
    Packet packet_;
    Frame frame_;
    int videoIndex_ = -1, audioIndex_ = -1, width_ = 0, height_ = 0;
    bool eof_ = false;
};

assets::Bytes readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("cannot read " + path);
    return assets::Bytes(std::istreambuf_iterator<char>(file), {});
}

std::string stemOf(const std::string& path) {
    std::size_t slash = path.find_last_of('/');
    std::string name = slash == std::string::npos ? path : path.substr(slash + 1);
    std::size_t dot = name.rfind('.');
    return dot == std::string::npos ? name : name.substr(0, dot);
}

struct Subtitles {
    std::vector<assets::SubtitleCue> cues;
    const assets::SubtitleCue* at(double time) const {
        auto ms = std::int64_t(time * 1000);
        for (const auto& cue : cues)
            if (ms >= cue.startMs && ms < cue.endMs) return &cue;
        return nullptr;
    }
};

class SubtitleRenderer {
public:
    SubtitleRenderer(SDL_Renderer* renderer, assets::FuseArchive& archive, const std::string& fontName)
        : extended_(assets::extendedCharacters(read(archive, "localisation/extended_chars_utf8.txt"))),
          font_(assets::parseFont(read(archive, "fonts/" + fontName + ".tnf"))) {
        assets::Image image = assets::decodeTexture(read(archive, "fonts/" + fontName + ".btga"));
        SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(image.rgba.data(), image.width, image.height, 32,
                                                                  image.width * 4, SDL_PIXELFORMAT_ABGR8888);
        if (!surface) throw std::runtime_error(SDL_GetError());
        atlas_ = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
        if (!atlas_) throw std::runtime_error(SDL_GetError());
        SDL_SetTextureBlendMode(atlas_, SDL_BLENDMODE_BLEND);
    }
    ~SubtitleRenderer() { SDL_DestroyTexture(atlas_); }
    SubtitleRenderer(const SubtitleRenderer&) = delete;
    SubtitleRenderer& operator=(const SubtitleRenderer&) = delete;

    const std::vector<std::string>& extended() const { return extended_; }

    void draw(SDL_Renderer* renderer, const assets::SubtitleCue& cue, int width, int height) {
        int lineHeight = std::max<int>(font_.lineHeight, 1);
        int top = height - 8 - lineHeight * int(cue.lines.size());
        for (std::size_t row = 0; row < cue.lines.size(); ++row) {
            auto placed = assets::layoutText(font_, cue.lines[row], extended_);
            if (placed.empty()) continue;
            int lineWidth = placed.back().x + placed.back().glyph.width;
            int left = (width - lineWidth) / 2, y = top + lineHeight * int(row);
            for (int pass = 0; pass < 2; ++pass) {
                int shadow = pass == 0 ? 1 : 0;
                SDL_SetTextureColorMod(atlas_, pass == 0 ? 0 : 255, pass == 0 ? 0 : 255, pass == 0 ? 0 : 255);
                for (const auto& glyph : placed) {
                    SDL_Rect source{glyph.glyph.x, glyph.glyph.y, glyph.glyph.width, glyph.glyph.height};
                    SDL_Rect target{left + glyph.x + shadow, y + shadow, glyph.glyph.width, glyph.glyph.height};
                    SDL_RenderCopy(renderer, atlas_, &source, &target);
                }
            }
        }
    }

private:
    static assets::Bytes read(assets::FuseArchive& archive, const std::string& name) {
        const assets::FuseEntry* entry = archive.find(name);
        if (!entry) throw std::runtime_error("not in archive: " + name);
        return archive.read(*entry);
    }

    std::vector<std::string> extended_;
    assets::Font font_;
    SDL_Texture* atlas_ = nullptr;
};

Options parseOptions(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto value = [&]() -> std::string {
            if (i + 1 >= argc) throw std::runtime_error("missing value for " + arg);
            return argv[++i];
        };
        if (arg == "--subtitles") options.subtitleFile = value();
        else if (arg == "--lang") options.language = value();
        else if (arg == "--screenshot") options.screenshot = value();
        else if (arg == "--time") options.time = std::stod(value());
        else if (arg == "--no-audio") options.audio = false;
        else if (arg == "--fib") options.fib = value();
        else if (arg == "--names") options.names = value();
        else if (arg == "--font") options.fontName = value();
        else if (arg.rfind("--", 0) == 0) throw std::runtime_error("unknown option " + arg);
        else options.video = arg;
    }
    if (options.video.empty())
        throw std::runtime_error("usage: chase-fmv VIDEO.moflex [--subtitles FILE.srt | --lang us] "
                                 "[--screenshot out.bmp --time SECONDS] [--no-audio]");
    return options;
}

void saveScreenshot(SDL_Renderer* renderer, const std::string& path) {
    int width = 0, height = 0;
    SDL_GetRendererOutputSize(renderer, &width, &height);
    std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)> surface(
        SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_ARGB8888), SDL_FreeSurface);
    if (!surface || SDL_RenderReadPixels(renderer, nullptr, SDL_PIXELFORMAT_ARGB8888, surface->pixels, surface->pitch) != 0 ||
        SDL_SaveBMP(surface.get(), path.c_str()) != 0)
        throw std::runtime_error(SDL_GetError());
}

int run(int argc, char** argv) {
    Options options = parseOptions(argc, argv);
    bool screenshotMode = !options.screenshot.empty();
    Movie movie(options.video);
    bool wantSubtitles = !options.subtitleFile.empty() || !options.language.empty();

    std::unique_ptr<assets::FuseArchive> archive;
    if (wantSubtitles) {
        archive = std::make_unique<assets::FuseArchive>(options.fib);
        archive->loadNames(options.names);
    }

    if (SDL_Init(SDL_INIT_VIDEO | (options.audio && !screenshotMode ? SDL_INIT_AUDIO : 0)) != 0)
        throw std::runtime_error(SDL_GetError());
    int scale = 2;
    SDL_Window* window = SDL_CreateWindow("chase-fmv", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          movie.width() * scale, movie.height() * scale, SDL_WINDOW_RESIZABLE);
    if (!window) throw std::runtime_error(SDL_GetError());
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, screenshotMode ? SDL_RENDERER_SOFTWARE : SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) throw std::runtime_error(SDL_GetError());
    SDL_RenderSetLogicalSize(renderer, movie.width(), movie.height());
    SDL_Texture* picture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING,
                                             movie.width(), movie.height());
    if (!picture) throw std::runtime_error(SDL_GetError());

    Subtitles subtitles;
    std::unique_ptr<SubtitleRenderer> subtitleRenderer;
    if (wantSubtitles) {
        subtitleRenderer = std::make_unique<SubtitleRenderer>(renderer, *archive, options.fontName);
        assets::Bytes srt;
        if (!options.subtitleFile.empty()) {
            srt = readFile(options.subtitleFile);
        } else {
            std::string name = "cutscenes/" + stemOf(options.video) + "_" + options.language + ".srt";
            const assets::FuseEntry* entry = archive->find(name);
            if (!entry) throw std::runtime_error("no subtitles for this video and language: " + name);
            srt = archive->read(*entry);
        }
        subtitles.cues = assets::parseSubtitles(srt, subtitleRenderer->extended());
    }

    SDL_AudioDeviceID device = 0;
    bool useAudio = options.audio && !screenshotMode && movie.hasAudio();
    if (useAudio) {
        SDL_AudioSpec want{}, have{};
        want.freq = audioRate;
        want.format = AUDIO_S16SYS;
        want.channels = 2;
        want.samples = 1024;
        device = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
        if (!device) throw std::runtime_error(SDL_GetError());
    }

    std::deque<VideoFrame> pending;
    std::vector<std::uint8_t> audioBuffer;
    std::uint64_t audioQueuedBytes = 0;
    VideoFrame current;
    bool haveCurrent = false, audioStarted = false;
    std::uint64_t startCounter = 0;

    auto upload = [&](const VideoFrame& frame) {
        int w = movie.width(), h = movie.height(), cw = (w + 1) / 2, ch = (h + 1) / 2;
        const std::uint8_t* y = frame.planes.data();
        SDL_UpdateYUVTexture(picture, nullptr, y, w, y + w * h, cw, y + w * h + cw * ch, cw);
    };
    auto flushAudio = [&] {
        if (device && !audioBuffer.empty()) {
            SDL_QueueAudio(device, audioBuffer.data(), Uint32(audioBuffer.size()));
            audioQueuedBytes += audioBuffer.size();
            audioBuffer.clear();
        }
    };
    auto render = [&](double time) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        if (haveCurrent) SDL_RenderCopy(renderer, picture, nullptr, nullptr);
        const assets::SubtitleCue* cue = subtitles.at(time);
        if (cue && subtitleRenderer) subtitleRenderer->draw(renderer, *cue, movie.width(), movie.height());
        return cue;
    };

    if (screenshotMode) {
        while (pending.empty() || pending.back().time < options.time) {
            if (!movie.readMore(pending, audioBuffer)) break;
            audioBuffer.clear();
        }
        if (pending.empty()) throw std::runtime_error("video has no frames");
        auto chosen = pending.begin();
        for (auto it = pending.begin(); it != pending.end() && it->time <= options.time; ++it) chosen = it;
        current = std::move(*chosen);
        haveCurrent = true;
        upload(current);
        const assets::SubtitleCue* cue = render(options.time);
        SDL_RenderPresent(renderer);
        saveScreenshot(renderer, options.screenshot);
        std::cout << "frame at " << current.time << " s, subtitle lines: " << (cue ? cue->lines.size() : 0) << ", saved "
                  << options.screenshot << '\n';
        if (cue)
            for (const auto& line : cue->lines) std::cout << "  " << line << '\n';
    } else {
        startCounter = SDL_GetPerformanceCounter();
        auto clock = [&]() -> double {
            if (device && audioStarted) {
                double played = double(audioQueuedBytes) - double(SDL_GetQueuedAudioSize(device));
                return played / (audioRate * audioFrameBytes);
            }
            if (device) return 0;
            return double(SDL_GetPerformanceCounter() - startCounter) / double(SDL_GetPerformanceFrequency());
        };
        bool running = true;
        while (running) {
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))
                    running = false;
            }
            auto queuedSeconds = [&] {
                return device ? double(SDL_GetQueuedAudioSize(device)) / (audioRate * audioFrameBytes) : 0.0;
            };
            while (!movie.finished() && (device ? queuedSeconds() < audioLeadSeconds : true) &&
                   pending.size() < maxPendingFrames) {
                movie.readMore(pending, audioBuffer);
                flushAudio();
            }
            if (device && !audioStarted &&
                (queuedSeconds() >= audioLeadSeconds || movie.finished() || pending.size() >= maxPendingFrames)) {
                SDL_PauseAudioDevice(device, 0);
                audioStarted = true;
            }
            double now = clock();
            bool changed = false;
            while (!pending.empty() && (!haveCurrent || pending.front().time <= now)) {
                current = std::move(pending.front());
                pending.pop_front();
                haveCurrent = changed = true;
            }
            if (changed) upload(current);
            render(now);
            SDL_RenderPresent(renderer);
            bool lastFrameShown = device ? SDL_GetQueuedAudioSize(device) == 0 : now > current.time + 1.0 / 30;
            if (movie.finished() && pending.empty() && lastFrameShown) running = false;
            SDL_Delay(4);
        }
    }

    if (device) SDL_CloseAudioDevice(device);
    SDL_DestroyTexture(picture);
    subtitleRenderer.reset();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
}

int main(int argc, char** argv) {
    try {
        return run(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << "chase-fmv: " << error.what() << '\n';
        return 1;
    }
}
