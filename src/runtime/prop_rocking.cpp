#include "runtime/prop_rocking.hpp"

#include "runtime/debris.hpp"
#include "runtime/level_start.hpp"
#include "runtime/player_intent.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>

namespace chase::runtime {
namespace {

constexpr float kTwoPi = 6.2831854820251465f;  // literal 0x40c90fdb at 0x28a870

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

std::int32_t intParam(const assets::LevelObject& object, const assets::Level& cls, const char* name, std::int32_t fallback) {
    const assets::LevelParam* param = effectiveParam(object, cls, assets::nameHash(name));
    if (!param) return fallback;
    if (param->type == 0) {
        float value = 0;
        std::memcpy(&value, &param->value, 4);
        return std::int32_t(value);
    }
    return std::int32_t(param->value);
}

float waveFraction(std::uint32_t clock, std::uint32_t phase, std::uint32_t period) {
    if (period == 0) return 0.0f;
    return float((clock + phase) % period) / float(period);
}

}

bool propRockingOf(const assets::LevelObject& object, const assets::Level& cls, PropRocking& out) {
    if (assets::objectKind(object, cls) != kRockingPropKind) return false;
    if ((intParam(object, cls, "Rocking", 0) & 1) == 0) return false;
    out.instance = lower(object.instance);
    out.rocking = true;
    out.period1 = std::uint32_t(intParam(object, cls, "WaveBob1", kWaveBob1Default)) * kRockingRate;
    out.period2 = std::uint32_t(intParam(object, cls, "WaveBob2", kWaveBob2Default)) * kRockingRate;
    out.phase = std::uint32_t(intParam(object, cls, "WaveBobOffset", kWaveBobOffsetDefault));
    return true;
}

std::array<float, 9> rockingRotation(const PropRocking& prop, std::uint32_t clock) {
    const float pitch = kRockingPitch * gameSin(waveFraction(clock, prop.phase, prop.period1) * kTwoPi);
    const float roll = gameCos(waveFraction(clock, prop.phase, prop.period2) * kTwoPi) * kRockingRoll;
    const std::array<float, 9> rock = debrisRotation(roll, 0.0f, pitch);
    const std::array<float, 9> base = debrisRotation(prop.baseAngles[0], prop.baseAngles[1], prop.baseAngles[2]);
    std::array<float, 9> out{};
    for (int row = 0; row < 3; ++row)
        for (int column = 0; column < 3; ++column)
            out[row * 3 + column] = rock[row * 3] * base[column] + rock[row * 3 + 1] * base[3 + column] + rock[row * 3 + 2] * base[6 + column];
    return out;
}

std::array<float, 16> rockingMatrix(const PropRocking& prop, std::uint32_t clock) {
    const std::array<float, 9> rotation = rockingRotation(prop, clock);
    std::array<float, 16> matrix = prop.placement;
    for (int row = 0; row < 3; ++row)
        for (int column = 0; column < 3; ++column) matrix[row * 4 + column] = rotation[row * 3 + column];
    return matrix;
}

void PropRockingSet::load(const assets::Level& level, const assets::ClassLookup& classOf) {
    props_.clear();
    for (const assets::LevelObject& object : level.objects) {
        const assets::Level* cls = classOf(object.className);
        PropRocking prop;
        if (!cls || !propRockingOf(object, *cls, prop)) continue;
        if (const auto start = assets::objectStart(level, object.instance)) {
            prop.placement = assets::placementMatrix(*start);
            prop.baseAngles = assets::matrixEulerAngles(prop.placement);
        }
        props_.push_back(std::move(prop));
    }
}

bool PropRockingSet::setRocking(const std::string& lowerInstance, float value) {
    for (PropRocking& prop : props_)
        if (prop.instance == lowerInstance) {
            prop.rocking = static_cast<std::int32_t>(value) == 1;
            return true;
        }
    return false;
}

const PropRocking* PropRockingSet::find(const std::string& lowerInstance) const {
    for (const PropRocking& prop : props_)
        if (prop.instance == lowerInstance) return &prop;
    return nullptr;
}

}
