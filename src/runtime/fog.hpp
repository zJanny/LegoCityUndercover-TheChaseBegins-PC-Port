#pragma once
// The original's fog: per-level parameters and the 128-entry PICA fog table built by 0x24e9d8 (docs/re/rendering-fog.md).
// No SDL and no OpenGL dependency.
#include "assets/assets.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace chase::runtime {

constexpr std::size_t kFogLutEntries = 128;
// RENDER-S5: near and far of the projection matrix read in the oracle (0x529e30+736: A = far / (far - near) = 1.0038609,
// near = 1); where the original sets them was not found.
constexpr float kFogProjectionNear = 1.0f;
constexpr float kFogProjectionFar = 260.0f;

// One word per entry as the game writes it to GPU register 0xe8: value (11 bits, /2048) << 13 | signed 13-bit difference (/2048).
using FogLut = std::array<std::uint32_t, kFogLutEntries>;
FogLut buildFogLut(float start, float end);

struct FogSetting {
    bool enabled = false;
    float start = 0, end = 0;
    std::array<std::uint8_t, 3> colour = {255, 255, 255};
};

// Setup 0x1a293c: the fog of the ambient light objects that apply to the room the scene is in. RENDER-S6: an object applies
// when it has no room or sits in `currentRoom`, it is accepted when start >= 0 and start < end, and the last accepted object
// wins; the original's two loops, its enable byte and its type test are not replicated. Without an accepted object fog is off.
FogSetting levelFog(const std::vector<assets::FogObject>& objects, int currentRoom);

}
