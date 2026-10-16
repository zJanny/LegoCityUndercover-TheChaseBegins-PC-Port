#pragma once
// Near and far plane of the gameplay render camera (docs/re/camera-vehicle.md): the follow update 0x2e0bf0 writes
// (near plane test result, K+0x14c) through 0x23f168 to camera +0xc4 / +0xc8 and the projection and the fog table are built from them.
// No SDL and no OpenGL dependency.

namespace chase::runtime {

struct ClipPlanes {
    float nearZ, farZ;
};

// K+0x14c: 260 in every recorded trace; its writer was not found (CAMERA-S50).
constexpr float kFollowFarPlane = 260.0f;

constexpr ClipPlanes followClipPlanes(float nearPlaneTestResult) { return {nearPlaneTestResult, kFollowFarPlane}; }

}
