#include "runtime/lighting.hpp"

#include "runtime/scene_start.hpp"

#include <algorithm>
#include <cmath>

namespace chase::runtime {
namespace {

constexpr std::uint8_t kLightMasks[16] = {0, 4, 0xf, 0, 4, 0xf, 0xf, 0xf, 0, 0xf, 0, 0, 0, 0, 0, 0};
constexpr float kDistributionExponent = 20.0f;

std::uint8_t channel(float value) { return std::uint8_t(std::clamp(int(value), 0, 255)); }

std::array<std::uint8_t, 3> lightColour(const std::array<float, 3>& light, const std::uint8_t material[4], float intensity) {
    std::array<std::uint8_t, 3> out{};
    for (std::size_t c = 0; c < 3; ++c) out[c] = channel(light[c] * float(material[c]) * intensity + 0.5f);
    return out;
}

std::array<std::uint8_t, 3> halved(std::array<std::uint8_t, 3> colour) {
    for (auto& c : colour) c = std::uint8_t(c >> 1);
    return colour;
}

std::uint32_t lutEntry(float current, float next) {
    const int value = std::clamp(int(std::floor(current * 4096.0f)), 0, 0xfff);
    const int difference = std::clamp(int(std::floor((next - current) * 2048.0f)), 0, 0x7ff);
    return std::uint32_t(value) | std::uint32_t(difference) << 12;
}

template <typename F>
LightLut sampledLut(F f) {
    LightLut lut{};
    const float step = 1.0f / float(kLightLutEntries - 1);
    float current = f(0.0f);
    for (std::size_t i = 0; i < kLightLutEntries; ++i) {
        const float next = f(float(i + 1) * step);
        lut[i] = lutEntry(current, next);
        current = next;
    }
    return lut;
}

}

Light lightFromObject(const assets::LightObject& object) {
    Light light;
    light.type = object.type;
    for (std::size_t c = 0; c < 3; ++c) {
        light.colour[c] = object.colour[c] * (1.0f / 255.0f);
        light.specular[c] = object.specular[c] * (1.0f / 255.0f);
    }
    light.intensity = object.intensity;
    light.maxRange = object.maxRange;
    const std::array<float, 16> m = assets::placementMatrix(object.placement);
    if (light.type == 1)
        light.vector = {-m[8], -m[9], -m[10]};
    else
        light.vector = {m[12], m[13], m[14]};
    return light;
}

LightSet levelLights(const std::vector<assets::LightObject>& objects, int currentRoom) {
    LightSet set;
    for (const assets::LightObject& object : objects) {
        const int room = roomNumber(object.room);
        if (room >= 0 && room != currentRoom) continue;
        const Light light = lightFromObject(object);
        if (light.type == 0) {
            for (std::size_t c = 0; c < 3; ++c) set.globalAmbient[c] += light.colour[c] * light.intensity;
        } else if ((light.type == 1 || light.type == 2) && set.list.size() < kMaxLights) {
            set.list.push_back(light);
        }
    }
    return set;
}

unsigned lightMask(unsigned lightingKey) { return lightingKey < 16 ? kLightMasks[lightingKey] : 0; }

FragmentLighting fragmentLighting(const LightSet& lights, unsigned lightingKey, const std::uint8_t colour[4],
                                  const std::uint8_t specular[4], bool doubled) {
    FragmentLighting out;
    const unsigned mask = lightMask(lightingKey);
    if (!mask) return out;
    out.enabled = true;
    if (mask & 1) out.globalAmbient = lightColour(lights.globalAmbient, colour, 1.0f);
    for (const Light& light : lights.list) {
        if (!(mask & (1u << light.type))) continue;
        LightSlot slot;
        const std::array<std::uint8_t, 3> lit = lightColour(light.colour, colour, light.intensity);
        const std::array<std::uint8_t, 3> shine = lightColour(light.specular, specular, light.intensity);
        slot.vector = light.vector;
        if (light.type == 2) {
            slot.ambient = doubled ? halved(lit) : lit;
            slot.specular = doubled ? halved(shine) : shine;
            slot.twoSided = true;
            slot.spotAttenuation = true;
            if (light.maxRange > 0.0f) {
                slot.distanceAttenuation = true;
                slot.distanceScale = 1.0f / light.maxRange;
            }
        } else {
            slot.diffuse = lit;
            slot.specular = shine;
            slot.directional = true;
        }
        out.slots.push_back(slot);
    }
    return out;
}

LightLut distributionLut() {
    return sampledLut([](float x) { return std::pow(x, kDistributionExponent); });
}

LightLut distanceLut() {
    return sampledLut([](float x) { return 1.0f - x; });
}

LightLut spotLut() {
    LightLut lut{};
    const float step = 1.0f / 127.0f;
    const auto f = [](float x) { return 0.5f + x * 0.5f; };
    for (std::size_t half = 0; half < 2; ++half) {
        const float start = half ? -1.0f : 0.0f;
        float current = f(start);
        for (std::size_t i = 0; i < 128; ++i) {
            const float next = f(start + float(i + 1) * step);
            lut[half * 128 + i] = lutEntry(current, next);
            current = next;
        }
    }
    return lut;
}

}
