// Pure parts of render/view_region.hpp (no OpenGL context): the default region and the aspect fed to perspective().
// Provenance of the values: hand computation, 20261029-qolset-B, 2026-10-29. Default aspect 400:240 as today's
// game_level.cpp draw(); widened aspect 16:9 from docs/design/qol-screen-layout.md §2026-10-28T09:21:33 (same vertical
// field of view, wider aspect). perspective() with fovY = pi/2 has f = 1, so m[0] = -1/aspect and m[5] = 1.
#include "render/scene.hpp"
#include "render/view_region.hpp"
#include <cmath>
#include <cstdio>
#include <stdexcept>

namespace {

int failures = 0;

void check(bool ok, const char* what) {
    if (!ok) {
        std::printf("FAIL: %s\n", what);
        ++failures;
    }
}

bool near(float a, float b) { return std::fabs(a - b) < 1e-5f; }

template <typename F>
bool throws(F f) {
    try {
        f();
    } catch (const std::invalid_argument&) {
        return true;
    }
    return false;
}

}

int main() {
    using namespace chase::render;
    const float halfPi = 1.57079632679f;

    const ViewRegion original = originalViewRegion({20, 120, 400, 240});
    check(original.aspectWidth == 400 && original.aspectHeight == 240, "default aspect terms 400:240");
    check(original.rect.x == 20 && original.rect.y == 120 && original.rect.width == 400 && original.rect.height == 240, "default keeps the rectangle");
    check(near(viewAspect(original), 400.0f / 240.0f), "default aspect 5/3");
    const Mat4 originalProjection = perspective(halfPi, viewAspect(original), 1.0f, 100.0f);
    check(near(originalProjection[0], -0.6f), "default projection x scale -1/(5/3) = -0.6");
    check(near(originalProjection[5], 1.0f), "default projection y scale 1");

    const ViewRegion wide = makeViewRegion({0, 0, 1280, 720}, 16, 9);
    check(near(viewAspect(wide), 16.0f / 9.0f), "wide aspect 16/9");
    const Mat4 wideProjection = perspective(halfPi, viewAspect(wide), 1.0f, 100.0f);
    check(near(wideProjection[0], -0.5625f), "wide projection x scale -9/16 = -0.5625");
    check(near(wideProjection[5], originalProjection[5]), "vertical field of view unchanged");

    const ViewRegion same = makeViewRegion({20, 120, 400, 240}, 400, 240);
    check(same.aspectWidth == original.aspectWidth && same.aspectHeight == original.aspectHeight, "makeViewRegion at 400:240 equals the default");

    check(throws([] { makeViewRegion({0, 0, 0, 240}, 16, 9); }), "zero width rejected");
    check(throws([] { makeViewRegion({0, 0, 400, -1}, 16, 9); }), "negative height rejected");
    check(throws([] { makeViewRegion({0, 0, 400, 240}, 0, 9); }), "zero aspect width rejected");
    check(throws([] { makeViewRegion({0, 0, 400, 240}, 16, -9); }), "negative aspect height rejected");

    if (failures) return 1;
    std::printf("render-viewport-rect OK\n");
    return 0;
}
