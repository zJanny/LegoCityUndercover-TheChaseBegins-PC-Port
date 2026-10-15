#pragma once
#include <array>

namespace chase::runtime {

// docs/re/rendering-sky.md: the sky pass (0x180fac) draws the level's sky model with the camera's orientation and the
// camera placed at (0, lift, 0), so the dome follows the camera and is lifted by an amount that grows with the
// player's height above the "Sea" object.
// Class of the objects that place a sky model.
constexpr const char* kSkyClass = "props/prop_skybox";

constexpr float kSkyHeightRange = 120.0f;  // literal at 0x1813a0
constexpr float kSkyMaxLift = 20.0f;       // literal at 0x1813a4

// 0x22a048 over (0, 120) -> (0, 20) applied at 0x18106c to the player's y minus the Sea object's y; 0 without a Sea object.
float skyLift(float playerY, float seaY);

// Translation that puts the sky model, drawn through the normal camera at `eye`, where a camera at (0, lift, 0) sees it.
std::array<float, 3> skyTranslation(const float eye[3], float lift);

}
