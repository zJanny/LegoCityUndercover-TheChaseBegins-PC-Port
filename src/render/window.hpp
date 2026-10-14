#pragma once
// Window state for the 2D passes of chase-game and chase-viewer: viewports, the scissor box, clears and the depth,
// culling and GPU-sync calls. Plain types only; the OpenGL calls live in window.cpp.
#include <cstdint>

namespace chase::render {

// Window pixels; y counts from the bottom of the window, as GL does.
struct PixelRect {
    int x = 0, y = 0, width = 0, height = 0;
};

// CPU image, RGBA8, row 0 at the top.
struct Pixels {
    int width = 0, height = 0;
    const std::uint8_t* rgba = nullptr;
};

struct ClearColour {
    float r, g, b, a;
};

void setDepthTest(bool enabled);
void setCullFace(bool enabled);
void setScissorTest(bool enabled);
void setViewport(const PixelRect& rect);
void setScissorBox(const PixelRect& rect);
void clearWindow(const ClearColour& colour, bool depth);
void waitForGpu();

}
