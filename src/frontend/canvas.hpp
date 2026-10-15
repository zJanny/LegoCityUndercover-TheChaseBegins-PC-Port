#pragma once
// CPU framebuffer for the two 3DS screens of the front end (top 400x240, bottom 320x240). No SDL dependency.
#include "assets/assets.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace chase::frontend {

constexpr int kTopWidth = 400;
constexpr int kBottomWidth = 320;
constexpr int kScreenHeight = 240;

struct Colour {
    float r = 1, g = 1, b = 1, a = 1;
};

struct IntRect {
    int x = 0, y = 0, width = 0, height = 0;
};

class Canvas;
// The two screens stacked as the emulator's screenshot shows them: 400x480, the bottom screen centred under the top.
Canvas composeScreens(const Canvas& top, const Canvas& bottom);

class Canvas {
public:
    Canvas(int width, int height);
    int width() const { return width_; }
    int height() const { return height_; }
    const std::vector<std::uint8_t>& rgba() const { return pixels_; }
    std::vector<std::uint8_t>& rgba() { return pixels_; }
    void clear(std::uint8_t r, std::uint8_t g, std::uint8_t b);
    void clearTransparent();
    void draw(const assets::Image& image, const IntRect& source, int x, int y, const Colour& tint = {});
    void drawScaled(const assets::Image& image, const IntRect& source, float x, float y, float width, float height,
                    const Colour& tint = {});
    void drawNineSlice(const assets::Image& image, int edge, int x, int y, int width, int height, const Colour& tint = {});
    void drawRotated(const assets::Image& image, const IntRect& source, float cx, float cy, float width, float height,
                     float radians, const Colour& tint = {});
    void drawLine(float x0, float y0, float x1, float y1, float width, const Colour& tint = {});
    void dim(float factor);
    void blit(const Canvas& other, int x, int y);
    void writeBmp(const std::string& path) const;
    std::uint8_t* pixel(int x, int y) { return &pixels_[(std::size_t(y) * width_ + x) * 4]; }
    const std::uint8_t* pixel(int x, int y) const { return &pixels_[(std::size_t(y) * width_ + x) * 4]; }

private:
    void blend(int x, int y, float r, float g, float b, float a);
    void drawScaledFlipped(const assets::Image& image, const IntRect& source, float x, float y, float width, float height,
                           const Colour& tint, bool flipX, bool flipY);
    int width_, height_;
    std::vector<std::uint8_t> pixels_;
};

}
