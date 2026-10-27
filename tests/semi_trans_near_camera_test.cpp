// The near-camera rule of the semi-transparent lists (src/runtime/semi_trans_near_camera.cpp, docs/re/rendering-draw-order.md).
// Provenance of the pinned values (20261027-graphics-D, 2026-10-27): REA (Ghidra 12.1.2) pseudo code and llvm-objdump of
// artifacts/game/game.arm.elf. 0x1ddde4: direction = (target - eye) with y = 0 normalised by 0x257ea0 (zeroed at a length up to the bits
// 0x34000000), the camera row +0x20 when the length bits are below 0x3727c5ac (1e-5); reach = dot(centre - eye, direction) - (half.x +
// half.z) x 0.5; reach below the distance: see-through with floor 1 and alpha 0 below half the distance, else int((0.5 - (distance - reach)
// / distance) x 2 x 255); an alpha of 0 becomes 1; otherwise alpha 255, floor 50. 0x2bac50 (step 6, no occluders): not see-through
// min(alpha + 6, target); see-through alpha - 6, the target when that is not below it, the floor when it is not above it. 0x10c6a4
// clamps to 0 .. 255. Distance 8: [0x374600 + 0x1c] set to 8.0 by 0x343e04, plus 0 x [+0x28] in 0x17b760. Expected numbers are worked
// by hand from these rules.
#include "runtime/semi_trans_near_camera.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using namespace chase::runtime;

void require(bool ok, const std::string& what) {
    if (!ok) throw std::runtime_error(what);
}

bool near(float a, float b) { return std::fabs(a - b) < 1e-6f; }

NearCameraView view() {
    NearCameraView v;
    v.eye = {0.0f, 3.0f, 0.0f};
    v.forward = {0.0f, -0.6f, 0.8f};
    v.target = {0.0f, 1.0f, 10.0f};
    return v;
}

void directions() {
    const Vector3 along = nearCameraDirection(view());
    require(near(along[0], 0.0f) && near(along[1], 0.0f) && near(along[2], 1.0f), "horizontal direction to the player");
    NearCameraView above = view();
    above.target = {0.0f, -5.0f, 0.0f};
    const Vector3 down = nearCameraDirection(above);
    require(near(down[1], -0.6f) && near(down[2], 0.8f), "camera forward when the player is straight below");
}

void targets() {
    const NearCameraView v = view();
    const Vector3 direction = nearCameraDirection(v);
    const Vector3 half{1.0f, 5.0f, 1.0f};
    const NearCameraTarget far = nearCameraTarget(v, direction, {0.0f, 0.0f, 9.0f}, half);
    require(!far.seeThrough && far.alpha == 255 && far.floor == 50, "reach 8 is not nearer than the distance");
    const NearCameraTarget middle = nearCameraTarget(v, direction, {0.0f, 0.0f, 7.0f}, half);
    require(middle.seeThrough && middle.alpha == 127 && middle.floor == 1, "reach 6 gives int(0.25 x 2 x 255)");
    const NearCameraTarget close = nearCameraTarget(v, direction, {5.0f, 0.0f, 2.0f}, half);
    require(close.seeThrough && close.alpha == 1, "reach 1 is below half the distance: alpha 0 becomes 1");
    const NearCameraTarget edge = nearCameraTarget(v, direction, {0.0f, 0.0f, 5.0f}, half);
    require(edge.seeThrough && edge.alpha == 1, "reach 4: alpha int(0) becomes 1");
}

void steps() {
    const NearCameraTarget out{true, 1, 127};
    std::int16_t alpha = kSemiTransOpaque;
    alpha = semiTransStep(alpha, out);
    require(alpha == 127, "the first step falls to the target");
    alpha = semiTransStep(alpha, out);
    require(alpha == 121, "then 6 per update");
    for (int i = 0; i < 30; ++i) alpha = semiTransStep(alpha, out);
    require(alpha == 1, "down to the floor");
    const NearCameraTarget back{false, 50, 255};
    alpha = semiTransStep(alpha, back);
    require(alpha == 7, "fades in by 6");
    for (int i = 0; i < 60; ++i) alpha = semiTransStep(alpha, back);
    require(alpha == 255, "up to 255");
    require(semiTransNodeAlpha(-3) == 0 && semiTransNodeAlpha(300) == 255 && semiTransNodeAlpha(40) == 40, "node alpha clamp");
}

}

int main() {
    try {
        directions();
        targets();
        steps();
    } catch (const std::exception& error) {
        std::cerr << "semi-trans-near-camera-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "semi-trans-near-camera-test: ok\n";
    return 0;
}
