#pragma once
#include "frontend/canvas.hpp"
#include "render/window.hpp"
#include <algorithm>
#include <cmath>

namespace game_hud {

namespace fe = chase::frontend;

using Rect = chase::render::PixelRect;

struct Screens {
    Rect top, bottom;
};

inline Screens screensFor(int width, int height) {
    const float scale = std::min(float(width) / fe::kTopWidth, float(height) / (2 * fe::kScreenHeight));
    const float originX = (float(width) - scale * fe::kTopWidth) / 2, originY = (float(height) - scale * 2 * fe::kScreenHeight) / 2;
    auto rect = [&](float x, float yFromTop, float w, float h) {
        const int left = int(std::lround(x)), right = int(std::lround(x + w));
        const int bottom = height - int(std::lround(yFromTop + h)), top = height - int(std::lround(yFromTop));
        return Rect{left, bottom, right - left, top - bottom};
    };
    Screens screens;
    screens.top = rect(originX, originY, scale * fe::kTopWidth, scale * fe::kScreenHeight);
    screens.bottom = rect(originX + scale * (fe::kTopWidth - fe::kBottomWidth) / 2, originY + scale * fe::kScreenHeight,
                          scale * fe::kBottomWidth, scale * fe::kScreenHeight);
    return screens;
}

inline chase::render::Pixels pixelsOf(const fe::Canvas& canvas) {
    return {canvas.width(), canvas.height(), canvas.rgba().data()};
}

}
