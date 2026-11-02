#pragma once
// SpawnDebris2Colour 0x2860f0 (docs/re/scripts-missions-1-4.md): SpawnDebris2Colour(object, r, g, b, r2, g2, b2) packs each float as an integer low byte
// with alpha 255 into two colour words and calls 0x23aebc(entity, colour, colour2, 6, 1, 1), which builds the position from the entity's node and runs
// 0x20c938: the class's DebrisColour and DebrisColour2 win over the call's colours, DebrisAmount over the 6 bricks. No SDL dependency.
#include "runtime/debris.hpp"
#include "runtime/prop_destroy.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace chase::runtime {

constexpr unsigned kSpawnDebris2ColourArguments = 7;

struct ScriptDebrisRequest {
    std::string object;   // lower case
    DebrisBurst burst;
};

// One colour component as 0x2860f0 stores it: vcvt.s32.f32 (truncated, saturated), low byte.
std::uint8_t scriptDebrisByte(float value);
// The colour word r | g << 8 | b << 16 | 255 << 24 of three components.
std::uint32_t scriptDebrisColour(float r, float g, float b);

// 0x20c938 for the position `position`: the colours of `classEffects` when the object or its class gives them, else the call's.
// A missing second colour (SpawnDebris) keeps the default of 0x20ca8c.
DebrisBurst scriptDebrisBurst(const PropDestroyEffects& classEffects, const DebrisVec3& position, std::uint32_t colour, std::optional<std::uint32_t> secondColour);

}
