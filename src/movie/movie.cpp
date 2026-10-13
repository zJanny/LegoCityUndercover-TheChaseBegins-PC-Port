#include "movie/movie.hpp"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}
#include <fstream>
#include <stdexcept>
#include <utility>

namespace chase::movie {

namespace {

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
    const int result = avcodec_open2(context.get(), codec, nullptr);
    if (result < 0) fail("cannot open decoder", result);
    return context;
}

constexpr std::uint8_t kLayoutEyePairs = 0;

bool hasEyePairs(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    std::uint8_t header[29] = {};
    if (!file.read(reinterpret_cast<char*>(header), sizeof header)) return false;
    const bool sync = header[0] == 0x4c && header[1] == 0x32;
    const bool videoRecord = header[14] == 3 && header[15] == 13;
    return sync && videoRecord && header[28] == kLayoutEyePairs;
}

}

struct Decoder::Impl {
    Impl(const std::string& path, PixelFormat format) : format_(format), eyePairs_(hasEyePairs(path)) {
        AVFormatContext* raw = nullptr;
        int result = avformat_open_input(&raw, path.c_str(), nullptr, nullptr);
        if (result < 0) fail("cannot open " + path, result);
        format_context_.reset(raw);
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
            result = swr_alloc_set_opts2(&swr, &stereo, AV_SAMPLE_FMT_S16, kAudioRate, &audio_->ch_layout,
                                         audio_->sample_fmt, audio_->sample_rate, 0, nullptr);
            if (result < 0) fail("cannot create resampler", result);
            resampler_.reset(swr);
            result = swr_init(swr);
            if (result < 0) fail("cannot initialise resampler", result);
        }
        packet_.reset(av_packet_alloc());
        frame_.reset(av_frame_alloc());
    }

    bool readMore(std::deque<VideoFrame>& video, std::vector<std::uint8_t>& audio) {
        if (eof_) return false;
        const int result = av_read_frame(format_context_.get(), packet_.get());
        if (result < 0) {
            eof_ = true;
            drain(video_.get(), videoIndex_, nullptr, &video, audio);
            if (audio_) drain(audio_.get(), audioIndex_, nullptr, &video, audio);
            return false;
        }
        const int stream = packet_->stream_index;
        if (stream == videoIndex_) feed(video_.get(), videoIndex_, &video, audio);
        else if (stream == audioIndex_ && audio_) feed(audio_.get(), audioIndex_, &video, audio);
        av_packet_unref(packet_.get());
        return true;
    }

    void feed(AVCodecContext* codec, int stream, std::deque<VideoFrame>* video, std::vector<std::uint8_t>& audio) {
        const int result = avcodec_send_packet(codec, packet_.get());
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
        AVStream* s = format_context_->streams[stream];
        const std::int64_t origin = s->start_time == AV_NOPTS_VALUE ? 0 : s->start_time;
        return double(pts - origin) * av_q2d(s->time_base);
    }

    void store(std::deque<VideoFrame>& video) {
        const bool secondEye = eyePairs_ && decodedPictures_++ % 2 == 1;
        if (secondEye) return;
        const AVPixelFormat target = format_ == PixelFormat::Rgba ? AV_PIX_FMT_RGBA : AV_PIX_FMT_YUV420P;
        if (!scaler_) {
            scaler_.reset(sws_getContext(width_, height_, AVPixelFormat(frame_->format), width_, height_, target,
                                         SWS_BILINEAR, nullptr, nullptr, nullptr));
            if (!scaler_) throw std::runtime_error("cannot create video scaler");
        }
        VideoFrame out;
        const std::int64_t pts = frame_->best_effort_timestamp;
        out.time = pts == AV_NOPTS_VALUE ? (video.empty() ? 0 : video.back().time + 1.0 / 30) : seconds(pts, videoIndex_);
        if (format_ == PixelFormat::Rgba) {
            out.data.resize(std::size_t(width_) * height_ * 4);
            std::uint8_t* destination[4] = {out.data.data(), nullptr, nullptr, nullptr};
            const int strides[4] = {width_ * 4, 0, 0, 0};
            sws_scale(scaler_.get(), frame_->data, frame_->linesize, 0, height_, destination, strides);
        } else {
            const int luma = width_ * height_, chroma = ((width_ + 1) / 2) * ((height_ + 1) / 2);
            out.data.resize(std::size_t(luma) + 2 * std::size_t(chroma));
            std::uint8_t* destination[4] = {out.data.data(), out.data.data() + luma, out.data.data() + luma + chroma, nullptr};
            const int strides[4] = {width_, (width_ + 1) / 2, (width_ + 1) / 2, 0};
            sws_scale(scaler_.get(), frame_->data, frame_->linesize, 0, height_, destination, strides);
        }
        video.push_back(std::move(out));
    }

    void convertAudio(std::vector<std::uint8_t>& audio) {
        const int capacity = swr_get_out_samples(resampler_.get(), frame_->nb_samples);
        const std::size_t before = audio.size();
        audio.resize(before + std::size_t(capacity) * kAudioFrameBytes);
        std::uint8_t* out = audio.data() + before;
        const int produced = swr_convert(resampler_.get(), &out, capacity,
                                         const_cast<const std::uint8_t**>(frame_->extended_data), frame_->nb_samples);
        if (produced < 0) fail("resample error", produced);
        audio.resize(before + std::size_t(produced) * kAudioFrameBytes);
    }

    PixelFormat format_;
    FormatContext format_context_;
    CodecContext video_, audio_;
    Resampler resampler_;
    Scaler scaler_;
    Packet packet_;
    Frame frame_;
    bool eyePairs_ = false;
    std::uint64_t decodedPictures_ = 0;
    int videoIndex_ = -1, audioIndex_ = -1, width_ = 0, height_ = 0;
    bool eof_ = false;
};

Decoder::Decoder(const std::string& path, PixelFormat format) : impl_(std::make_unique<Impl>(path, format)) {}
Decoder::~Decoder() = default;
int Decoder::width() const { return impl_->width_; }
int Decoder::height() const { return impl_->height_; }
bool Decoder::hasAudio() const { return impl_->audioIndex_ >= 0; }
bool Decoder::finished() const { return impl_->eof_; }
bool Decoder::eyePairs() const { return impl_->eyePairs_; }
bool Decoder::readMore(std::deque<VideoFrame>& video, std::vector<std::uint8_t>& audio) {
    return impl_->readMore(video, audio);
}

SteppedPlayer::SteppedPlayer(const std::string& path) : decoder_(std::make_unique<Decoder>(path, PixelFormat::Rgba)) {}
SteppedPlayer::~SteppedPlayer() = default;
int SteppedPlayer::width() const { return decoder_->width(); }
int SteppedPlayer::height() const { return decoder_->height(); }
bool SteppedPlayer::hasAudio() const { return decoder_->hasAudio(); }

bool SteppedPlayer::advance(double seconds) {
    clock_ += seconds;
    for (;;) {
        while (pending_.empty() && decoder_->readMore(pending_, audio_)) {
        }
        if (pending_.empty() || (haveFrame_ && pending_.front().time > clock_)) break;
        const double previous = frame_.time;
        frame_ = std::move(pending_.front());
        pending_.pop_front();
        if (haveFrame_ && frame_.time > previous) framePeriod_ = frame_.time - previous;
        haveFrame_ = true;
    }
    return !(pending_.empty() && decoder_->finished() && haveFrame_ && clock_ >= frame_.time + framePeriod_);
}

std::vector<std::uint8_t> SteppedPlayer::takeAudio() {
    std::vector<std::uint8_t> out;
    out.swap(audio_);
    return out;
}

}
