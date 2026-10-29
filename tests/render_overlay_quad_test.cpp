// Placement of an overlay canvas on a window rectangle (render/overlay_quad.hpp), no OpenGL context. Values by hand computation,
// 20261029-qolmenu-G, 2026-10-29: clip x = 2 * x / width - 1 and clip y = 2 * y / height - 1 for the rectangle's edges (y up from the
// window's bottom row), texture coordinates (0,0) top-left to (1,1) bottom-right in the vertex order top-left, top-right,
// bottom-left, bottom-right.
#include "render/overlay_quad.hpp"
#include <cmath>
#include <cstdio>

namespace {

int failures = 0;

void check(const chase::render::PixelRect& rect, int width, int height, const float (&expected)[16], const char* what) {
    const chase::render::OverlayQuad quad = chase::render::overlayQuad(rect, width, height);
    for (int i = 0; i < 16; ++i) {
        if (std::fabs(quad.vertices[i] - expected[i]) > 1e-6f) {
            std::printf("FAIL: %s, vertex value %d: %f, expected %f\n", what, i, double(quad.vertices[i]), double(expected[i]));
            ++failures;
            return;
        }
    }
}

}

int main() {
    check({0, 360, 800, 240}, 800, 600, {-1, 1, 0, 0, 1, 1, 1, 0, -1, 0.2f, 0, 1, 1, 0.2f, 1, 1}, "the top screen's rectangle of an 800x600 window");
    check({100, 300, 200, 100}, 800, 600, {-0.75f, 1.0f / 3, 0, 0, -0.25f, 1.0f / 3, 1, 0, -0.75f, 0, 0, 1, -0.25f, 0, 1, 1},
          "a part rectangle inside the window");
    check({0, 0, 1, 1}, 2, 2, {-1, 0, 0, 0, 0, 0, 1, 0, -1, -1, 0, 1, 0, -1, 1, 1}, "a one pixel rectangle at the window's lower left");
    return failures ? 1 : 0;
}
