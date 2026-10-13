#include "assets/assets.hpp"
#include "movie/movie.hpp"
#include <SDL.h>
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
using chase::movie::kAudioFrameBytes;
using chase::movie::kAudioRate;
using chase::movie::VideoFrame;

constexpr double audioLeadSeconds = 0.4;
constexpr int maxPendingFrames = 24;

struct Options {
    std::string video, subtitleFile, language, screenshot;
    std::string fib = "artifacts/romfs/romfs/lego_city3ds.fib", names = "artifacts/romfs/romfs/lego_city3ds.csv";
    std::string fontName = "frontendfont";
    double time = 0;
    bool audio = true;
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
    movie::Decoder movie(options.video, movie::PixelFormat::Yuv420);
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
        want.freq = kAudioRate;
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
        const std::uint8_t* y = frame.data.data();
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
                return played / (kAudioRate * kAudioFrameBytes);
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
                return device ? double(SDL_GetQueuedAudioSize(device)) / (kAudioRate * kAudioFrameBytes) : 0.0;
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
