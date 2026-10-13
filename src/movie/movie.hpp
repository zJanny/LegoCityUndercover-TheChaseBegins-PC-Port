#pragma once
#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <vector>

namespace chase::movie {

constexpr int kAudioRate = 22050;
constexpr int kAudioFrameBytes = 4;

enum class PixelFormat { Yuv420, Rgba };

struct VideoFrame {
    double time = 0;
    std::vector<std::uint8_t> data;
};

class Decoder {
public:
    Decoder(const std::string& path, PixelFormat format);
    ~Decoder();
    Decoder(const Decoder&) = delete;
    Decoder& operator=(const Decoder&) = delete;

    int width() const;
    int height() const;
    bool hasAudio() const;
    bool finished() const;

    bool readMore(std::deque<VideoFrame>& video, std::vector<std::uint8_t>& audio);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

class SteppedPlayer {
public:
    explicit SteppedPlayer(const std::string& path);
    ~SteppedPlayer();
    SteppedPlayer(const SteppedPlayer&) = delete;
    SteppedPlayer& operator=(const SteppedPlayer&) = delete;

    int width() const;
    int height() const;
    bool hasAudio() const;
    bool advance(double seconds);
    const std::vector<std::uint8_t>& rgba() const { return frame_.data; }
    double frameTime() const { return frame_.time; }
    std::vector<std::uint8_t> takeAudio();

private:
    std::unique_ptr<Decoder> decoder_;
    std::deque<VideoFrame> pending_;
    std::vector<std::uint8_t> audio_;
    VideoFrame frame_;
    double clock_ = 0, framePeriod_ = 1.0 / 30;
    bool haveFrame_ = false;
};

}
