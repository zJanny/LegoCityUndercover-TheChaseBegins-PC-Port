// Test of the head icon (src/runtime/head_icon.*, docs/re/vehicles-boarding.md): the numbers are the literals of 0x1a0954 (distance fade 6 to 10,
// lift 1 + 2 * scale), of the HeadIcons loader 0x2e7104 (fade 0.15) and of 0x22d7c0 (hold 0.1); the projection is the inverse of the unprojection
// the studs use (0x24c1d0). The scale 0.85 is read from the oracle traces (stand-in HUD-S51).
#include "runtime/head_icon.hpp"

#include <cmath>
#include <cstdio>

namespace {

using namespace chase::runtime;

int failures = 0;

void near(double got, double want, double tolerance, const char* what) {
    if (std::fabs(got - want) > tolerance) {
        std::printf("FAIL: %s (got %.5f want %.5f)\n", what, got, want);
        ++failures;
    }
}

void expect(bool ok, const char* what) {
    if (!ok) {
        std::printf("FAIL: %s\n", what);
        ++failures;
    }
}

void alpha() {
    expect(headIconAlphaByte(1.0f, 5.99f) == 0, "closer than 6: invisible");
    expect(headIconAlphaByte(1.0f, 6.0f) == 0, "at 6: still 0");
    expect(headIconAlphaByte(1.0f, 8.0f) == 127, "halfway: half alpha");
    expect(headIconAlphaByte(1.0f, 10.0f) == 255, "at 10: full");
    expect(headIconAlphaByte(1.0f, 30.0f) == 255, "far: full");
    expect(headIconAlphaByte(0.5f, 30.0f) == 127, "the fade scales it");
}

void fade() {
    HeadIcon icon;
    expect(icon.fade() == 0.0f, "hidden at first");
    icon.request();
    icon.update(0.075f);
    near(icon.fade(), 0.5, 1e-5, "fade in over 0.15 s");
    icon.update(0.075f);
    icon.update(0.0f);
    near(icon.fade(), 1.0, 1e-5, "shown");
    icon.request();
    icon.update(0.09f);
    near(icon.fade(), 1.0, 1e-5, "held for 0.1 s after a request");
    icon.update(0.02f);
    icon.update(0.075f);
    near(icon.fade(), 0.5, 1e-4, "fades out over 0.15 s");
}

void position() {
    const float node[3] = {10, 20, 30};
    const std::array<float, 3> p = headIconPosition(node, kPlayerEntityScaleMeasured);
    near(p[0], 10, 1e-6, "x");
    near(p[1], 20 + 1.0 + 2.0 * 0.85, 1e-5, "y lifted by 1 + 2 * 0.85");
    near(p[2], 30, 1e-6, "z");
}

void projection() {
    const float eye[3] = {0, 0, 0};
    const float fov = 1.0f;
    const HeadIconScreen centre = projectHeadIcon({0, 0, 10}, eye, 0.0f, 0.0f, fov);
    expect(centre.visible, "in front");
    near(centre.centreX, 200, 1e-3, "centred x");
    near(centre.centreY, 120, 1e-3, "centred y");
    const double scaleY = 1.0 / std::tan(0.5 * fov);
    near(centre.height, scaleY / 10.0 * 120.0, 1e-3, "height of a unit quad at depth 10");
    near(centre.width, scaleY / (400.0 / 240.0) / 10.0 * 200.0, 1e-3, "width of a unit quad at depth 10");
    expect(!projectHeadIcon({0, 0, -5}, eye, 0.0f, 0.0f, fov).visible, "behind the camera");
    const HeadIconScreen above = projectHeadIcon({0, 2, 10}, eye, 0.0f, 0.0f, fov);
    expect(above.centreY < 120.0f, "up is towards the top of the screen");
}

}

int main() {
    alpha();
    fade();
    position();
    projection();
    if (failures) return 1;
    std::printf("head icon ok\n");
    return 0;
}
