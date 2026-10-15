#pragma once
// The original's fragment lighting (docs/re/rendering-lighting.md): the light objects of the level, the light list the game
// fills each frame (0x250688), the register values the per-draw setup 0x12d884 derives from a light and a material, and the
// lookup tables the lighting init 0x115850 uploads once. No SDL and no OpenGL dependency.
#include "assets/assets.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace chase::runtime {

constexpr std::size_t kMaxLights = 8;
constexpr std::size_t kLightLutEntries = 256;

// A light as 0x15dd50 builds it: colours divided by 255, kind from LightType, and the vector 0x250688 stores at +0x44
// (direction towards the light for kind 1, position for kind 2).
struct Light {
    int type = 0;
    std::array<float, 3> colour = {0, 0, 0};
    std::array<float, 3> specular = {0, 0, 0};
    float intensity = 1, maxRange = 10;
    std::array<float, 3> vector = {0, 0, 0};
};
Light lightFromObject(const assets::LightObject& object);

struct LightSet {
    std::array<float, 3> globalAmbient = {0, 0, 0};  // 0x5943d0: sum of the kind 0 lights' colour times intensity
    std::vector<Light> list;                          // 0x5943b0: kinds 1 and 2 in registration order, at most 8
};
// STAND-IN RENDER-S60: the lights that apply are those without a room or in `currentRoom` (the rule of levelFog); the original
// registers light nodes through 0x2523cc, whose choice by room was not read.
LightSet levelLights(const std::vector<assets::LightObject>& objects, int currentRoom);

// Table 0x3798f8: which light kinds a material lighting key uses (bit 0 global ambient, bit k light kind k). Zero means the
// draw runs with fragment lighting off (0x12d780 with 2) and the fragment colours are 0.
unsigned lightMask(unsigned lightingKey);

// One light slot as 0x12d884 writes it (registers 0x140 + 16 n): colours are the 8-bit register values.
struct LightSlot {
    std::array<std::uint8_t, 3> diffuse = {0, 0, 0}, ambient = {0, 0, 0}, specular = {0, 0, 0};
    bool directional = false;            // 0x149 bit 0; kind 2 lights write 2 (two-sided diffuse, positional)
    bool twoSided = false;
    bool spotAttenuation = false;        // config1 bit 8 + n cleared for kind 2
    bool distanceAttenuation = false;    // config1 bit 24 + n cleared for kind 2 with MaxRange > 0
    float distanceScale = 0;             // 0x14b: 1 / MaxRange
    std::array<float, 3> vector = {0, 0, 0};
};
struct FragmentLighting {
    bool enabled = false;
    std::array<std::uint8_t, 3> globalAmbient = {0, 0, 0};
    std::vector<LightSlot> slots;
};
// 0x12d884 for a material: `colour` and `specular` are the RGBA bytes at material +0x10 and +0x14, `doubled` is material byte
// 0xb bit 5 (the last stage scales x2), which halves the colours of kind 2 lights.
FragmentLighting fragmentLighting(const LightSet& lights, unsigned lightingKey, const std::uint8_t colour[4],
                                  const std::uint8_t specular[4], bool doubled);

// Lookup tables as 0x24cae0 fills them: entry i = floor(f(i / 255) * 4096) | floor((f((i + 1) / 255) - f(i / 255)) * 2048) << 12,
// both clamped (difference to 0..0x7ff). The PICA reads value / 4096 and the signed 12-bit difference / 2048.
using LightLut = std::array<std::uint32_t, kLightLutEntries>;
LightLut distributionLut();   // LUT 0 (D0): pow(x, 20) with the exponent at 0x37798c
LightLut distanceLut();       // LUTs 16..23 (DA): 1 - x
LightLut spotLut();           // LUTs 8..15 (SP), signed input: entries 0..127 for x = i / 127, 128..255 for x = -1 + i / 127; 0.5 + 0.5 x

// Register values of the lighting init 0x115850 and the per-frame list (oracle command list, docs/re/rendering-lighting.md).
constexpr std::uint32_t kLightingConfig0 = 0x88000400;  // 0x1c3: LUT set 0, clamp highlights (bit 27)
constexpr std::uint32_t kLutInputAbs = 0x200;           // 0x1d0: absolute input for D0 and DA, signed for SP
constexpr std::uint32_t kLutInputSelect = 0x2300;       // 0x1d1: D0 = N.H, SP = L.N

}
