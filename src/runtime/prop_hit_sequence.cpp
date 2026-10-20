#include "runtime/prop_hit_sequence.hpp"

#include "runtime/level_start.hpp"

#include <cstring>

namespace chase::runtime {
namespace {

constexpr std::uint8_t kModeHealth = 1;
constexpr std::uint8_t kModeLights = 3;
constexpr std::uint8_t kModeHealthB = 4;
constexpr std::uint8_t kModePropSender = 5;
constexpr std::uint8_t kModeUntypedHit = 7;
constexpr std::uint8_t kModeSequence = 11;
constexpr std::uint8_t kModeCharacterHit = 50;
constexpr std::uint8_t kModeHitKind2 = 52;
constexpr std::uint8_t kPropKind = 67;
constexpr std::uint8_t kCharacterKind = 27;
constexpr std::uint8_t kHitKind2 = 2;
constexpr std::uint8_t kSequenceHit = 255;

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

bool modeReadsHealth(std::uint8_t mode) {
    return mode == kModeHealth || mode == kModeLights || mode == kModeHealthB || mode == kModeUntypedHit || mode == kModeSequence;
}

HitOutcome subtractHealth(HitMachineProp& prop, std::int32_t damage) {
    prop.health = std::int8_t(std::uint8_t(std::uint8_t(prop.health) - std::uint8_t(damage)));
    if (prop.health > 0) return HitOutcome::Damaged;
    prop.health = 0;
    return HitOutcome::Destroyed;
}

HitOutcome hitMachineCase0(HitMachineProp& prop, const HitMessage& message) {
    switch (prop.mode) {
    case kModeUntypedHit:
        if (message.word0 != 0) return HitOutcome::Ignored;
        [[fallthrough]];
    case kModeHealth:
    case kModeHealthB:
        if (prop.projectileType != -1 && message.projectile != std::uint8_t(prop.projectileType)) return HitOutcome::Ignored;
        return subtractHealth(prop, message.damage);
    case kModePropSender:
        if (!message.hasSender || message.senderKind != kPropKind) return HitOutcome::Ignored;
        prop.health = 0;
        return HitOutcome::Destroyed;
    case kModeLights:
        return HitOutcome::NotPorted;
    case kModeSequence:
        return message.damage < 1 ? HitOutcome::Ignored : HitOutcome::NotPorted;
    default:
        return HitOutcome::Ignored;
    }
}

}

HitMachineProp hitMachineProp(const assets::LevelObject& object, const assets::Level& cls) {
    HitMachineProp prop;
    prop.mode = std::uint8_t(intParam(object, cls, "Destructable", 0));
    if (modeReadsHealth(prop.mode)) prop.health = std::int8_t(intParam(object, cls, "Health", 1));
    prop.projectileType = std::int8_t(intParam(object, cls, "ProjectileType", -1));
    prop.ignorePlayerDamage = intParam(object, cls, "IgnorePlayerDamage", 0) != 0;
    prop.abilityDestructable = intParam(object, cls, "AbilityDestructable", 0);
    return prop;
}

HitMessage vehicleSmashMessage() {
    HitMessage message;
    message.damage = 100;
    message.hitKind = 11;
    message.hasSender = true;
    message.senderKind = 183;
    return message;
}

HitOutcome hitMachineMessage0(HitMachineProp& prop, const HitMessage& message) {
    if (message.projectile >= 1 && message.projectile <= 6) return HitOutcome::Ignored;
    if (message.hasSender) {
        if (prop.ignorePlayerDamage && message.senderIsPlayer) return HitOutcome::Ignored;
        if (prop.abilityDestructable != 0) {
            if (!message.senderIsCharacter) return HitOutcome::Ignored;
            return HitOutcome::NotPorted;
        }
        if (message.senderKind == kCharacterKind) {
            if (prop.mode == kModeCharacterHit) return subtractHealth(prop, message.damage);
        } else if (message.hitKind == kHitKind2 && prop.mode == kModeHitKind2) {
            return subtractHealth(prop, message.damage);
        }
    }
    if (prop.mode == kModeSequence && message.sequence == kSequenceHit) return HitOutcome::NotPorted;
    return hitMachineCase0(prop, message);
}

}
