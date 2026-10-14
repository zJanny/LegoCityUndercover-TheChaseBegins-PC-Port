#include "runtime/fog.hpp"

#include "runtime/scene_start.hpp"

#include <algorithm>
#include <cmath>

namespace chase::runtime {

FogLut buildFogLut(float start, float end) {
    const float depthScale = kFogProjectionFar / (kFogProjectionFar - kFogProjectionNear);
    FogLut lut{};
    float previous = 1.0f;
    for (std::size_t i = 0; i < kFogLutEntries; ++i) {
        const float depth = float(i + 1) * (1.0f / float(kFogLutEntries));
        const float distance = kFogProjectionNear / (1.0f - depth / depthScale);
        const float fraction = std::clamp((distance - start) / (end - start), 0.0f, 1.0f);
        const float current = 1.0f - fraction;
        const int value = std::clamp(int(std::floor(previous * 2048.0f)), 0, 0x7ff);
        const int difference = std::clamp(int(std::floor((current - previous + 2.0f) * 2048.0f)), 0, 0x1fff) - 4096;
        lut[i] = std::uint32_t(difference & 0x1fff) | std::uint32_t(value) << 13;
        previous = current;
    }
    return lut;
}

FogSetting levelFog(const std::vector<assets::FogObject>& objects, int currentRoom) {
    FogSetting fog;
    for (const assets::FogObject& object : objects) {
        const int room = roomNumber(object.room);
        if (room >= 0 && room != currentRoom) continue;
        if (!(object.start >= 0.0f && object.start < object.end)) continue;
        fog.enabled = true;
        fog.start = object.start;
        fog.end = object.end;
        fog.colour = object.colour;
    }
    return fog;
}

}
