#pragma once
// The region of the window the 3D top screen is drawn into: a pixel rectangle and the aspect ratio of the projection.
// Plain types only; the OpenGL calls live in window.cpp.
#include "render/window.hpp"

namespace chase::render {

struct ViewRegion {
    PixelRect rect;
    int aspectWidth = 0, aspectHeight = 0;
};

constexpr int kTopScreenAspectWidth = 400;
constexpr int kTopScreenAspectHeight = 240;

// The original top screen: `rect` at the aspect 400:240.
ViewRegion originalViewRegion(const PixelRect& rect);

// Throws std::invalid_argument when the rectangle or an aspect term is not positive.
ViewRegion makeViewRegion(const PixelRect& rect, int aspectWidth, int aspectHeight);

float viewAspect(const ViewRegion& region);

// Clears the whole window black, then limits viewport and scissor to the region and clears it to `inside`.
void beginViewRegion(int windowWidth, int windowHeight, const ViewRegion& region, const ClearColour& inside);

}
