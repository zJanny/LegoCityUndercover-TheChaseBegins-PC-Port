#include "runtime/character_hit.hpp"
#include "runtime/level_start.hpp"

#include <algorithm>
#include <cstring>
#include <iterator>

namespace chase::runtime {
namespace {

constexpr std::uint16_t kPlayerNoHitStates[] = {0x156, 0x157, 0x158};
constexpr std::uint16_t kGuardedStates[] = {14, 15, 16, 17, 225, 322, 323, 324};
constexpr std::uint16_t kAttackStates[] = {0xa3, 0xa4};
constexpr std::uint16_t kRefusingStates[] = {0xf1, 0xf2};

// 0x23d610: the takedown states of 0x22e884 / 0x28cbf8 take no reaction.
bool takedownState(std::uint16_t state) {
    switch (state) {
    case 0x122: case 0x123: case 0x124: case 0x125: case 0x126: case 0x127: case 0x128: case 0x129: case 0x12a: case 0x12b: case 0x12c:
    case 0x12d: case 0x12e: case 0x12f: case 0x134: case 0x135: case 0x139: case 0x13a: case 0x13b: case 0x13c: case 0x13d: case 0x13e:
    case 0x143: case 0x144:
        return true;
    default:
        return false;
    }
}

template <std::size_t N>
bool oneOf(std::uint16_t state, const std::uint16_t (&states)[N]) {
    return std::find(std::begin(states), std::end(states), state) != std::end(states);
}

// 0x219ba0(target, sender, 1) with the sender the player: mode 0 and 3 take the player's hit, mode 2 only from its target ctrl+0xc4 (never the player
// here: STAND-IN COMBAT-S1), mode 4 and the others refuse.
bool selectModeTakesPlayer(std::uint8_t mode) { return mode == 0 || mode == 3; }

constexpr std::uint8_t kRidePigKind = 159;
constexpr std::uint8_t kHitKindRidePigTakes = 4;

// 0x22e884: non-zero turns the damage to 0. A ride pig (kind 159) takes only kind 4; a boss returns 1 (kind 15 requests KNOCK BACK, COMBAT-4); a
// move record of type 1..7, 11, 12 or 13 (byte +0x66) and the kinds 13 to 16 and 18 request takedown states (not ported, COMBAT-2) and return 1;
// without a move a character in a takedown state returns 1.
bool takedownTakesHit(const CharacterHitTarget& target, const CharacterHitMessage& hit) {
    if (target.kind == kRidePigKind && hit.kind != kHitKindRidePigTakes) return true;
    if (target.boss) return true;
    if (hit.move) {
        const std::uint8_t type = *hit.move;
        return (type >= 1 && type <= 7) || type == 11 || type == 12 || type == 13;
    }
    if (takedownState(target.state)) return true;
    return hit.kind == 13 || hit.kind == 14 || hit.kind == 15 || hit.kind == 16 || hit.kind == 18;
}

// 0x2451d0: an integer parameter (types 5 and 7 raw, type 0 truncated), else the default.
std::int32_t intParam(const assets::LevelObject& object, const assets::Level& cls, const char* name, std::int32_t fallback) {
    const assets::LevelParam* value = effectiveParam(object, cls, assets::nameHash(name));
    if (!value) return fallback;
    if (value->type == 5 || value->type == 7) return std::int32_t(value->value);
    if (value->type == 0) {
        float f = 0;
        std::memcpy(&f, &value->value, 4);
        return std::int32_t(f);
    }
    return fallback;
}

constexpr std::int32_t kDefaultHealth = 4;

}

CharacterHitMessage characterHitFromPlayer(std::int32_t damage, std::uint8_t kind) {
    CharacterHitMessage hit;
    hit.sender.present = true;
    hit.sender.player = true;
    hit.sender.character = true;
    hit.damage = damage;
    hit.kind = kind;
    return hit;
}

CharacterHitTarget characterHitTargetOf(const assets::LevelObject& object, const assets::Level& cls) {
    CharacterHitTarget target;
    target.maxHealth = std::uint8_t(intParam(object, cls, "Health", kDefaultHealth));
    target.health = target.maxHealth;
    target.arrestHealth = arrestHealthOf(intParam(object, cls, "ArrestHealth", 0));
    target.boss = intParam(object, cls, "Boss_IsBoss", 0) != 0;
    return target;
}

std::uint8_t arrestHealthOf(std::int32_t arrestHealthParam) {
    const std::int32_t value = arrestHealthParam + 1;
    return std::uint8_t(std::clamp(value, 0, 255));
}

bool changeArrestHealth(CharacterHitTarget& target, int delta) {
    const int value = std::max(0, int(target.arrestHealth) + delta);
    target.arrestHealth = std::uint8_t(value);
    return value == 0;
}

std::optional<std::uint16_t> hitReactionState(const CharacterHitTarget& target, const CharacterHitMessage& hit) {
    if (target.pending || target.reactionLocked) return std::nullopt;
    if ((oneOf(target.state, kAttackStates) && target.attackWindow) || oneOf(target.state, kRefusingStates) || takedownState(target.state)) return std::nullopt;
    const bool special = (hit.kind == 4 && hit.weapon == 2) || (hit.kind == 6 && (hit.weapon == 6 || hit.weapon == 4));
    if (special) {
        const bool thrown = hit.weapon >= 6 && hit.weapon <= 11 && hit.weapon != 10;
        const std::uint16_t state = thrown ? kCharacterStateThrown : kCharacterStateKnockBackCont;
        if (target.state == state) return std::nullopt;
        return state;
    }
    if (hit.kind == kHitKindPunch && hit.sender.present)
        return target.state == kCharacterStatePunched ? kCharacterStatePunchedRepeat : kCharacterStatePunched;
    if (target.state == kCharacterStateKnockBack) return std::nullopt;
    return target.onGround ? kCharacterStateKnockBack : kCharacterStateFall;
}

CharacterHitResult receiveHit(CharacterHitTarget& target, const CharacterHitMessage& hit) {
    CharacterHitResult result;
    if (target.disabled || hit.kind == kHitKindIgnored) return result;
    if (target.player && oneOf(target.state, kPlayerNoHitStates)) return result;
    const bool fromCharacter = hit.sender.present && hit.sender.character;
    if (fromCharacter && hit.sender.player && !selectModeTakesPlayer(target.selectMode)) return result;
    if (!target.player && target.playerSlot) return result;
    if (hit.damage < kHitIgnoresGuards) {
        if (target.spawnCounter < kHitSpawnGuard || target.cooldown != 0 || oneOf(target.state, kGuardedStates)) return result;
    }
    std::uint8_t cooldown = target.player ? kHitCooldownPlayer : hit.kind == kHitKindPunch ? kHitCooldownPunch : kHitCooldownOther;
    std::int32_t damage = hit.damage;
    if (takedownTakesHit(target, hit)) damage = 0;
    const bool punchOnCharacter = hit.sender.present && hit.kind == kHitKindPunch && !target.player;
    if (punchOnCharacter) {
        damage = 0;
    } else if (damage < 0) {
        target.health = std::uint16_t(std::min<int>(target.health - damage, target.maxHealth));
        result.outcome = CharacterHitOutcome::Healed;
        return result;
    } else if (damage > 0) {
        if (target.oneHitKill) damage = 100;
        if (!target.destroyed && !target.reactionLocked && target.health != 0) {
            if (std::uint32_t(damage) < target.health) {
                target.health = std::uint16_t(target.health - damage);
                result.outcome = CharacterHitOutcome::Damaged;
            } else {
                target.health = 0;
                target.destroyed = true;
                result.outcome = CharacterHitOutcome::Destroyed;
            }
            result.damage = damage;
        }
        if (!target.destroyed && !target.reactionLocked && cooldown != 0) {
            result.cooldown = cooldown;
            target.cooldown = cooldown;
        }
    }
    if (hit.damage < kHitReactionLimit) {
        result.reaction = hitReactionState(target, hit);
        // 0x2414b4 refuses every request but DEAD (11) on an entity with the destroyed bit.
        if (target.destroyed) result.reaction.reset();
        if (result.reaction) target.pending = result.reaction;
    }
    if (result.outcome == CharacterHitOutcome::Ignored) result.outcome = CharacterHitOutcome::Reacted;
    return result;
}

void CharacterHitStates::request(std::uint16_t state) { pending_ = state; }

bool CharacterHitStates::active() const {
    return state_ == kCharacterStatePunched || state_ == kCharacterStatePunchedRepeat || state_ == kCharacterStateKnockedDown ||
           state_ == kCharacterStateGetUp;
}

CharacterHitStep CharacterHitStates::update(bool clipFinished, bool boss) {
    CharacterHitStep step;
    if (pending_) {
        state_ = *pending_;
        pending_.reset();
        switch (state_) {
        case kCharacterStatePunched:
        case kCharacterStatePunchedRepeat:
            step.clip = kClipPunched;
            step.facesSender = true;
            break;
        case kCharacterStateKnockedDown:
            step.clip = kClipKnockedDown;
            wait_ = boss ? kKnockedDownWaitBoss : kKnockedDownWait;
            break;
        case kCharacterStateGetUp:
            step.clip = kClipGetUp;
            break;
        default:
            break;
        }
        return step;
    }
    switch (state_) {
    case kCharacterStatePunched:
    case kCharacterStatePunchedRepeat:
    case kCharacterStateGetUp:
        if (clipFinished) request(kCharacterStateIdle), step.request = kCharacterStateIdle;
        break;
    case kCharacterStateKnockedDown:
        if (clipFinished && --wait_ == 0) request(kCharacterStateGetUp), step.request = kCharacterStateGetUp;
        break;
    default:
        break;
    }
    return step;
}

}
