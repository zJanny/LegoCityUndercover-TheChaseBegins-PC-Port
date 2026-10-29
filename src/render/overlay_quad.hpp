#pragma once
#include "render/window.hpp"

namespace chase::render {

struct OverlayQuad {
    float vertices[16];
};

OverlayQuad overlayQuad(const PixelRect& rect, int windowWidth, int windowHeight);

}
