#include "frontend/canvas.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>

namespace chase::frontend {

namespace {
std::uint8_t toByte(float value) {
    return std::uint8_t(std::clamp(int(std::lround(value)), 0, 255));
}
}

Canvas::Canvas(int width, int height) : width_(width), height_(height), pixels_(std::size_t(width) * height * 4, 0) {
    for (std::size_t i = 3; i < pixels_.size(); i += 4) pixels_[i] = 255;
}

void Canvas::clear(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
    for (std::size_t i = 0; i < pixels_.size(); i += 4) {
        pixels_[i] = r;
        pixels_[i + 1] = g;
        pixels_[i + 2] = b;
        pixels_[i + 3] = 255;
    }
}

void Canvas::clearTransparent() {
    std::fill(pixels_.begin(), pixels_.end(), std::uint8_t(0));
}

void Canvas::blend(int x, int y, float r, float g, float b, float a) {
    if (x < 0 || y < 0 || x >= width_ || y >= height_ || a <= 0) return;
    std::uint8_t* destination = pixel(x, y);
    if (destination[3] != 255) {
        const float below = destination[3] / 255.0f * (1 - a), alpha = a + below;
        destination[0] = toByte((r * a + destination[0] * below) / alpha);
        destination[1] = toByte((g * a + destination[1] * below) / alpha);
        destination[2] = toByte((b * a + destination[2] * below) / alpha);
        destination[3] = toByte(alpha * 255);
        return;
    }
    destination[0] = toByte(r * a + destination[0] * (1 - a));
    destination[1] = toByte(g * a + destination[1] * (1 - a));
    destination[2] = toByte(b * a + destination[2] * (1 - a));
}

void Canvas::draw(const assets::Image& image, const IntRect& source, int x, int y, const Colour& tint) {
    for (int row = 0; row < source.height; ++row) {
        const int sy = source.y + row;
        if (sy < 0 || sy >= image.height) continue;
        for (int column = 0; column < source.width; ++column) {
            const int sx = source.x + column;
            if (sx < 0 || sx >= image.width) continue;
            const std::uint8_t* texel = &image.rgba[(std::size_t(sy) * image.width + sx) * 4];
            blend(x + column, y + row, texel[0] * tint.r, texel[1] * tint.g, texel[2] * tint.b, texel[3] / 255.0f * tint.a);
        }
    }
}

void Canvas::drawScaled(const assets::Image& image, const IntRect& source, float x, float y, float width, float height,
                        const Colour& tint) {
    if (width == 0 || height == 0) return;
    if (width < 0 || height < 0) {
        const bool flipX = width < 0, flipY = height < 0;
        if (flipX) {
            x += width;
            width = -width;
        }
        if (flipY) {
            y += height;
            height = -height;
        }
        drawScaledFlipped(image, source, x, y, width, height, tint, flipX, flipY);
        return;
    }
    drawScaledFlipped(image, source, x, y, width, height, tint, false, false);
}

void Canvas::drawScaledFlipped(const assets::Image& image, const IntRect& source, float x, float y, float width,
                               float height, const Colour& tint, bool flipX, bool flipY) {
    const int x0 = int(std::floor(x)), x1 = int(std::ceil(x + width));
    const int y0 = int(std::floor(y)), y1 = int(std::ceil(y + height));
    auto texel = [&](int sx, int sy, float out[4]) {
        sx = std::clamp(sx, source.x, source.x + source.width - 1);
        sy = std::clamp(sy, source.y, source.y + source.height - 1);
        const std::uint8_t* t = &image.rgba[(std::size_t(std::clamp(sy, 0, image.height - 1)) * image.width +
                                             std::clamp(sx, 0, image.width - 1)) * 4];
        for (int i = 0; i < 4; ++i) out[i] = t[i];
    };
    const int firstX = std::max(x0, 0), lastX = std::min(x1, width_);
    const int firstY = std::max(y0, 0), lastY = std::min(y1, height_);
    for (int py = firstY; py < lastY; ++py) {
        for (int px = firstX; px < lastX; ++px) {
            ++scaledPixelsVisited_;
            const float fu = (px + 0.5f - x) / width, fv = (py + 0.5f - y) / height;
            const float u = (flipX ? 1 - fu : fu) * source.width - 0.5f + source.x;
            const float v = (flipY ? 1 - fv : fv) * source.height - 0.5f + source.y;
            const int ux = int(std::floor(u)), vy = int(std::floor(v));
            const float fx = u - ux, fy = v - vy;
            float a[4], b[4], c[4], d[4], mixed[4];
            texel(ux, vy, a);
            texel(ux + 1, vy, b);
            texel(ux, vy + 1, c);
            texel(ux + 1, vy + 1, d);
            for (int i = 0; i < 4; ++i)
                mixed[i] = (a[i] * (1 - fx) + b[i] * fx) * (1 - fy) + (c[i] * (1 - fx) + d[i] * fx) * fy;
            blend(px, py, mixed[0] * tint.r, mixed[1] * tint.g, mixed[2] * tint.b, mixed[3] / 255.0f * tint.a);
        }
    }
}

void Canvas::drawNineSlice(const assets::Image& image, int edge, int x, int y, int width, int height, const Colour& tint) {
    const int middleWidth = image.width - 2 * edge, middleHeight = image.height - 2 * edge;
    const int columns[3] = {0, edge, image.width - edge}, columnWidths[3] = {edge, middleWidth, edge};
    const int rows[3] = {0, edge, image.height - edge}, rowHeights[3] = {edge, middleHeight, edge};
    const int destX[3] = {x, x + edge, x + width - edge}, destWidth[3] = {edge, width - 2 * edge, edge};
    const int destY[3] = {y, y + edge, y + height - edge}, destHeight[3] = {edge, height - 2 * edge, edge};
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            const IntRect source{columns[column], rows[row], columnWidths[column], rowHeights[row]};
            if (destWidth[column] <= 0 || destHeight[row] <= 0) continue;
            drawScaled(image, source, float(destX[column]), float(destY[row]), float(destWidth[column]),
                       float(destHeight[row]), tint);
        }
    }
}

void Canvas::drawRotated(const assets::Image& image, const IntRect& source, float cx, float cy, float width, float height,
                         float radians, const Colour& tint) {
    const float cosine = std::cos(radians), sine = std::sin(radians);
    const float reach = (std::fabs(width * cosine) + std::fabs(height * sine)) / 2 + 1;
    const float reachY = (std::fabs(width * sine) + std::fabs(height * cosine)) / 2 + 1;
    auto texel = [&](int sx, int sy, float out[4]) {
        sx = std::clamp(sx, source.x, source.x + source.width - 1);
        sy = std::clamp(sy, source.y, source.y + source.height - 1);
        const std::uint8_t* t = &image.rgba[(std::size_t(sy) * image.width + sx) * 4];
        for (int i = 0; i < 4; ++i) out[i] = t[i];
    };
    for (int py = int(std::floor(cy - reachY)); py <= int(std::ceil(cy + reachY)); ++py) {
        for (int px = int(std::floor(cx - reach)); px <= int(std::ceil(cx + reach)); ++px) {
            const float dx = px + 0.5f - cx, dy = py + 0.5f - cy;
            const float localX = dx * cosine + dy * sine, localY = -dx * sine + dy * cosine;
            const float fu = localX / width + 0.5f, fv = localY / height + 0.5f;
            if (fu < 0 || fu >= 1 || fv < 0 || fv >= 1) continue;
            const float u = fu * source.width - 0.5f + source.x, v = fv * source.height - 0.5f + source.y;
            const int ux = int(std::floor(u)), vy = int(std::floor(v));
            const float fx = u - ux, fy = v - vy;
            float a[4], b[4], c[4], d[4], mixed[4];
            texel(ux, vy, a);
            texel(ux + 1, vy, b);
            texel(ux, vy + 1, c);
            texel(ux + 1, vy + 1, d);
            for (int i = 0; i < 4; ++i)
                mixed[i] = (a[i] * (1 - fx) + b[i] * fx) * (1 - fy) + (c[i] * (1 - fx) + d[i] * fx) * fy;
            blend(px, py, mixed[0] * tint.r, mixed[1] * tint.g, mixed[2] * tint.b, mixed[3] / 255.0f * tint.a);
        }
    }
}

void Canvas::drawLine(float x0, float y0, float x1, float y1, float width, const Colour& tint) {
    const float half = width * 0.5f;
    const float lengthSquared = (x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0);
    const int left = std::max(0, int(std::floor(std::min(x0, x1) - half)) - 1), right = std::min(width_ - 1, int(std::ceil(std::max(x0, x1) + half)) + 1);
    const int top = std::max(0, int(std::floor(std::min(y0, y1) - half)) - 1), bottom = std::min(height_ - 1, int(std::ceil(std::max(y0, y1) + half)) + 1);
    for (int y = top; y <= bottom; ++y)
        for (int x = left; x <= right; ++x) {
            const float px = float(x) + 0.5f - x0, py = float(y) + 0.5f - y0;
            const float along = lengthSquared > 0 ? std::clamp((px * (x1 - x0) + py * (y1 - y0)) / lengthSquared, 0.0f, 1.0f) : 0.0f;
            const float distance = std::hypot(px - along * (x1 - x0), py - along * (y1 - y0));
            const float coverage = std::clamp(half + 0.5f - distance, 0.0f, 1.0f);
            if (coverage > 0) blend(x, y, 255.0f * tint.r, 255.0f * tint.g, 255.0f * tint.b, coverage * tint.a);
        }
}

void Canvas::dim(float factor) {
    for (std::size_t i = 0; i < pixels_.size(); i += 4)
        for (int c = 0; c < 3; ++c) pixels_[i + c] = toByte(pixels_[i + c] * factor);
}

void Canvas::blit(const Canvas& other, int x, int y) {
    for (int row = 0; row < other.height_; ++row) {
        for (int column = 0; column < other.width_; ++column) {
            if (x + column < 0 || y + row < 0 || x + column >= width_ || y + row >= height_) continue;
            std::copy_n(other.pixel(column, row), 4, pixel(x + column, y + row));
        }
    }
}

Canvas composeScreens(const Canvas& top, const Canvas& bottom) {
    Canvas out(kTopWidth, 2 * kScreenHeight);
    out.blit(top, 0, 0);
    out.blit(bottom, (kTopWidth - kBottomWidth) / 2, kScreenHeight);
    return out;
}

void Canvas::writeBmp(const std::string& path) const {
    const int rowBytes = (width_ * 3 + 3) & ~3;
    const std::uint32_t imageBytes = std::uint32_t(rowBytes) * height_;
    std::vector<std::uint8_t> file(54 + imageBytes, 0);
    auto put32 = [&](std::size_t at, std::uint32_t v) {
        for (int i = 0; i < 4; ++i) file[at + i] = std::uint8_t(v >> (8 * i));
    };
    file[0] = 'B';
    file[1] = 'M';
    put32(2, std::uint32_t(file.size()));
    put32(10, 54);
    put32(14, 40);
    put32(18, std::uint32_t(width_));
    put32(22, std::uint32_t(height_));
    file[26] = 1;
    file[28] = 24;
    put32(34, imageBytes);
    for (int row = 0; row < height_; ++row) {
        std::uint8_t* out = &file[54 + std::size_t(height_ - 1 - row) * rowBytes];
        for (int column = 0; column < width_; ++column) {
            const std::uint8_t* in = pixel(column, row);
            out[column * 3] = in[2];
            out[column * 3 + 1] = in[1];
            out[column * 3 + 2] = in[0];
        }
    }
    std::ofstream stream(path, std::ios::binary);
    if (!stream) throw std::runtime_error("cannot write " + path);
    stream.write(reinterpret_cast<const char*>(file.data()), std::streamsize(file.size()));
}

}
