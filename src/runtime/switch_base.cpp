#include "runtime/switch_base.hpp"
#include "runtime/level_start.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace chase::runtime {

std::int32_t switchIntParam(const assets::LevelObject& object, const assets::Level& cls, const char* name, std::int32_t fallback) {
    const assets::LevelParam* param = effectiveParam(object, cls, assets::nameHash(name));
    if (!param) return fallback;
    if (param->type == 5 || param->type == 7) return std::int32_t(param->value);
    if (param->type != 0) return fallback;
    float value;
    std::memcpy(&value, &param->value, 4);
    if (std::isnan(value)) return 0;
    if (value >= 2147483647.0f) return 2147483647;
    if (value <= -2147483648.0f) return -2147483647 - 1;
    return std::int32_t(value);
}

namespace {

float floatParam(const assets::LevelObject& object, const assets::Level& cls, const char* name, float fallback) {
    const assets::LevelParam* param = effectiveParam(object, cls, assets::nameHash(name));
    if (!param || param->type != 0) return fallback;
    float value;
    std::memcpy(&value, &param->value, 4);
    return value;
}

void add(std::vector<SwitchEffect>& effects, SwitchEffectKind kind, std::uint16_t sound = 0) { effects.push_back({kind, sound}); }

bool onState(SwitchState state) { return state == SwitchState::On || state == SwitchState::Held || state == SwitchState::Resetting; }

}

SwitchParams readSwitchParams(const assets::LevelObject& object, const assets::Level& cls) {
    SwitchParams params;
    params.resetTime = floatParam(object, cls, "ResetTime", 0.0f);
    params.startOn = switchIntParam(object, cls, "StartOn", 0) != 0;
    params.loopedUseAnim = switchIntParam(object, cls, "LoopedUseAnim", 0) != 0;
    params.triggerBeforeAnim = switchIntParam(object, cls, "TriggerBeforeAnim", 0) != 0;
    params.alwaysReset = switchIntParam(object, cls, "AlwaysReset", 0) != 0;
    params.disableAfterUse = switchIntParam(object, cls, "DisableAfterUse", 0) != 0;
    params.sfxOn = std::uint16_t(switchIntParam(object, cls, "ATTR_SFX_ON", 0));
    params.sfxOff = std::uint16_t(switchIntParam(object, cls, "ATTR_SFX_OFF", 0));
    params.sfxUse = std::uint16_t(switchIntParam(object, cls, "ATTR_SFX_USE", 0));
    params.useSoundDelay = floatParam(object, cls, "USE_SoundDelay", 0.0f);
    params.numberOfTurns = floatParam(object, cls, "NumberOfTurns", 1.0f);
    params.dontCutUseSound = switchIntParam(object, cls, "DontCutUseSound", 0) != 0;
    return params;
}

bool switchPostLink(SwitchRecord& record, const SwitchParams& params) {
    if (!params.startOn) return false;
    record.state = record.target = SwitchState::Off;
    record.resetTimer = params.resetTime;
    return true;
}

void switchStableUpdate(SwitchRecord& record, const SwitchParams& params, bool useAnimFinished, std::vector<SwitchEffect>& effects) {
    const SwitchState usedTarget = params.neverReset() ? SwitchState::Held : SwitchState::On;
    switch (record.state) {
    case SwitchState::Off:
        if (record.useRequested) {
            record.target = SwitchState::Used;
            record.useRequested = false;
        }
        break;
    case SwitchState::Used:
        if (!params.hasUseAnim || useAnimFinished || params.triggerBeforeAnim) record.target = usedTarget;
        break;
    case SwitchState::On:
        if (record.useRequested) {
            record.resetTimer = params.resetTime;
            record.useRequested = false;
        } else if (record.resetTimer <= 0.0f) {
            record.target = SwitchState::SwitchingOff;
        }
        break;
    case SwitchState::Held: break;
    case SwitchState::Resetting:
        if (record.resetTimer <= 0.0f) record.target = SwitchState::SwitchingOff;
        break;
    case SwitchState::SwitchingOff:
        if (!params.hasUseAnim) {
            record.target = SwitchState::Off;
        } else if (useAnimFinished) {
            record.target = SwitchState::Off;
            add(effects, SwitchEffectKind::UseAnimToStart);
        }
        break;
    }
    if (params.disableAfterUse && onState(record.state)) add(effects, SwitchEffectKind::DisableAfterUse);
}

void switchStateChange(SwitchRecord& record, const SwitchParams& params, float dt, const std::function<void(bool)>& reference,
                       std::vector<SwitchEffect>& effects) {
    if (record.state != record.target) {
        if (record.state == SwitchState::Used && params.sfxUse != 0 && !params.dontCutUseSound) add(effects, SwitchEffectKind::StopUseSound, params.sfxUse);
        const SwitchState target = record.target;
        if (target == SwitchState::Used) {
            if (params.hasUseAnim && params.numberOfTurns == 1.0f) add(effects, SwitchEffectKind::UseAnimForward);
            record.useSoundTimer = params.useSoundDelay;
            if (record.useSoundTimer == 0.0f && params.sfxUse != 0) add(effects, SwitchEffectKind::UseSound, params.sfxUse);
        } else if (target == SwitchState::On || target == SwitchState::Held) {
            record.resetTimer = params.resetTime;
            add(effects, SwitchEffectKind::LightsOn);
            if (record.state != SwitchState::On) reference(true);
            if (params.sfxOn != 0) add(effects, SwitchEffectKind::SoundOn, params.sfxOn);
            add(effects, SwitchEffectKind::StudDrop);
        } else if (target == SwitchState::SwitchingOff) {
            reference(false);
            add(effects, SwitchEffectKind::LightsOff);
            if (params.hasUseAnim && !params.loopedUseAnim) add(effects, SwitchEffectKind::UseAnimBackward);
            if (params.sfxOff != 0) add(effects, SwitchEffectKind::SoundOff, params.sfxOff);
            record.useRequested = record.usedByOther = false;
        }
        record.state = record.target;
    }
    if (record.useSoundTimer != 0.0f) {
        record.useSoundTimer -= dt;
        if (record.useSoundTimer <= 0.0f) {
            if (params.sfxUse != 0) add(effects, SwitchEffectKind::UseSound, params.sfxUse);
            record.useSoundTimer = 0.0f;
        }
    }
    if (record.resetTimer > 0.0f) record.resetTimer = std::max(0.0f, record.resetTimer - dt);
}

SwitchMessageResult switchBaseMessage(SwitchRecord& record, const SwitchParams& params, std::uint8_t message, bool fromPlayer, float argument,
                                      std::uint8_t referenceKind) {
    SwitchMessageResult result;
    switch (message) {
    case kSwitchMessageState: result.returned = std::uint32_t(record.state); break;
    case kSwitchMessageRestorePose:
        if (onState(record.state) && params.hasUseAnim) add(result.effects, SwitchEffectKind::UseAnimAtEnd);
        break;
    case kSwitchMessageReset:
        if (referenceKind == 1 || referenceKind == 2) record.target = SwitchState::SwitchingOff;
        break;
    case kSwitchMessageUse:
        if (record.target == SwitchState::Off && argument == 1.0f) {
            record.useRequested = true;
            record.usedByOther = !fromPlayer;
        }
        break;
    case kSwitchMessageOnAtOnce:
        record.state = record.target = SwitchState::Held;
        result.setRegistered = true;
        if (params.hasUseAnim) add(result.effects, SwitchEffectKind::UseAnimAtEnd);
        break;
    case kSwitchMessageOff:
        if (onState(record.target)) record.target = SwitchState::SwitchingOff;
        break;
    case kSwitchMessageOn:
        if (record.target == SwitchState::Off) {
            record.useRequested = true;
            record.usedByOther = false;
        } else if (record.target == SwitchState::On) {
            if (!params.alwaysReset) {
                record.target = SwitchState::Held;
            } else {
                record.target = SwitchState::Resetting;
                record.resetTimer = params.resetTime;
            }
        }
        break;
    default: break;
    }
    return result;
}

}
