#include "runtime/prop_bouncer.hpp"

#include "runtime/level_start.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>

namespace chase::runtime {
namespace {

float floatParam(const assets::LevelObject& object, const assets::Level& cls, const char* name, float fallback) {
    const assets::LevelParam* param = effectiveParam(object, cls, assets::nameHash(name));
    if (!param) return fallback;
    if (param->type == 0) {
        float value = 0;
        std::memcpy(&value, &param->value, 4);
        return value;
    }
    if (param->type == 5 || param->type == 7) return float(std::int32_t(param->value));
    return fallback;
}

bool flagParam(const assets::LevelObject& object, const assets::Level& cls, const char* name) {
    const assets::LevelParam* param = effectiveParam(object, cls, assets::nameHash(name));
    if (!param) return false;
    if (param->type == 0) {
        float value = 0;
        std::memcpy(&value, &param->value, 4);
        return std::int32_t(value) != 0;
    }
    return std::int32_t(param->value) != 0;
}

std::uint16_t idParam(const assets::LevelObject& object, const assets::Level& cls, const char* name) {
    const assets::LevelParam* param = effectiveParam(object, cls, assets::nameHash(name));
    return param ? std::uint16_t(param->value) : std::uint16_t(0);
}

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

}

BouncerParams bouncerParams(const assets::LevelObject& object, const assets::Level& cls) {
    BouncerParams params;
    params.instance = lower(object.instance);
    params.highestTakeOffSpeed = std::fabs(floatParam(object, cls, "HighestTakeOffSpeed", 1.0f));
    params.lowestTakeOffSpeed = std::fabs(floatParam(object, cls, "LowestTakeOffSpeed", 0.3f));
    params.jumpSpeedChange = floatParam(object, cls, "JumpSpeedChange", 0.1f);
    params.fallSpeedLoss = floatParam(object, cls, "FallSpeedLoss", 1.0f);
    params.autoBounce = flagParam(object, cls, "AutoBounce");
    params.trickBounce = flagParam(object, cls, "TrickBounce");
    params.sound = idParam(object, cls, "ATTR_SFX");
    params.soundHit = idParam(object, cls, "ATTR_SFXHit");
    params.particle = idParam(object, cls, "ATTR_ParticleNum");
    return params;
}

void BouncerSet::load(const assets::Level& level, const assets::ClassLookup& classOf) {
    bouncers_.clear();
    for (const assets::LevelObject& object : level.objects) {
        if (object.className != kBouncerClass) continue;
        if (const assets::Level* cls = classOf(object.className)) bouncers_.push_back(bouncerParams(object, *cls));
    }
}

const BouncerParams* BouncerSet::find(const std::string& lowerInstance) const {
    for (const BouncerParams& bouncer : bouncers_)
        if (bouncer.instance == lowerInstance) return &bouncer;
    return nullptr;
}

BounceLaunch bounceLaunch(const BouncerParams& bouncer, BounceMemory& memory, float fallSpeed, bool jumpHeld) {
    if (bouncer.lowestTakeOffSpeed > memory.speed) {
        const float scaled = fallSpeed * (1.0f - bouncer.fallSpeedLoss);
        float speed = bouncer.lowestTakeOffSpeed;
        if (scaled >= bouncer.highestTakeOffSpeed) speed = bouncer.highestTakeOffSpeed;
        else if (scaled > bouncer.lowestTakeOffSpeed) speed = scaled;
        memory.speed = speed;
    }
    if (bouncer.fallSpeedLoss < 1.0f) memory.field24c = 1.0f;

    float speed = memory.speed;
    if (jumpHeld || memory.field24c == 1.0f) speed += bouncer.jumpSpeedChange;
    else speed -= bouncer.jumpSpeedChange * 2.0f;
    memory.field24c = 0;
    if (speed >= bouncer.highestTakeOffSpeed) speed = bouncer.highestTakeOffSpeed;
    else if (!(speed > bouncer.lowestTakeOffSpeed)) speed = bouncer.lowestTakeOffSpeed;
    memory.speed = speed;

    BounceLaunch launch;
    launch.speed = speed;
    launch.fallSpeed = (speed > 0 ? -speed : speed) + kBounceRiseStep;
    launch.rise = speed;
    return launch;
}

void startBounceFlight(VerticalState& state, const BounceLaunch& launch) {
    state.posture = Posture::Falling;
    state.y += launch.rise;
    state.bouncing = true;
    state.bounceSettling = false;
    state.fallSpeed = launch.fallSpeed;
    state.fallDistance = 0;
    state.jump = nullptr;
    state.slideFall = false;
    state.hardLanding = false;
}

}
