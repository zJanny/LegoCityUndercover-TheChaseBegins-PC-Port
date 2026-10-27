#pragma once
// The near-camera entries of the rooms' semi-transparent lists (docs/re/rendering-draw-order.md): AddToSemiTransNearCameraList 0x2cb734
// adds the nodes of a model to the list at room +0x80 (0x1f40d8, entry flag bit 2, alpha +0x12 = 255), and every update 0x2e5b14 runs
// 0x1ddde4 over the lists with the camera matrix, the player node's position and the distance 0x374630: a node nearer the camera than the
// distance takes at most the alpha its distance gives and loses 6 per update down to 1, a node beyond it gains 6 per update up to 255
// (0x2bac50); the entry alpha becomes the node alpha +0x11b (0x23f708 key 0, 0x10c6a4). No SDL dependency.
#include <array>
#include <cstdint>

namespace chase::runtime {

using Vector3 = std::array<float, 3>;

// 0x374630 in normal play: [0x374600 + 0x1c] (8.0, static set-up 0x343e04) plus 0 x [+0x28] (0x17b760).
constexpr float kNearCameraDistance = 8.0f;
constexpr int kSemiTransStep = 6;
constexpr int kSemiTransFloor = 50;
constexpr int kNearCameraFloor = 1;
constexpr std::int16_t kSemiTransOpaque = 255;

struct NearCameraView {
    Vector3 eye;      // camera matrix +0x30
    Vector3 forward;  // camera matrix +0x20
    Vector3 target;   // the player node's world translation
    float distance = kNearCameraDistance;
};

// The direction 0x1ddde4 measures along: target - eye without its height, normalised by 0x257ea0; the camera's forward row when that
// length is below 1e-5.
Vector3 nearCameraDirection(const NearCameraView& view);

struct NearCameraTarget {
    bool seeThrough = false;
    int floor = kSemiTransFloor;
    int alpha = kSemiTransOpaque;
};
// One entry with flag bit 2: `centre` is the node box centre (+0xa0) plus the node's world translation, `half` the box half extents (+0xac).
NearCameraTarget nearCameraTarget(const NearCameraView& view, const Vector3& direction, const Vector3& centre, const Vector3& half);

// 0x2bac50 for an entry without occluders (+0x1c) or linked entry (+4).
std::int16_t semiTransStep(std::int16_t alpha, const NearCameraTarget& target);

// 0x10c6a4: the node alpha byte the entry alpha gives.
std::uint8_t semiTransNodeAlpha(std::int16_t alpha);

}
