#include "runtime/attack_targets.hpp"

#include <cmath>

namespace chase::runtime {
namespace {
constexpr std::uint8_t kKindDestructible = 100;
constexpr std::uint8_t kKindNever = 158;
constexpr std::uint8_t kKindPathed = 19;
constexpr int kStateDropped0 = 0xb;
constexpr int kStateDropped1 = 0x52;
constexpr int kStateGrabbed0 = 0x143;
constexpr int kStateGrabbed1 = 0x144;
constexpr int kState19 = 0x13;
constexpr int kStateFightWait = 0xdb;
constexpr unsigned kAbilityFightWait = 0x2b;

bool sameSide(const AttackEntity& a, const AttackEntity& b) { return (a.word & kEntityWordBadGuy) == (b.word & kEntityWordBadGuy); }

bool passesNonCharacter(const AttackEntity& candidate) {
    switch (candidate.kind) {
    case kKindDestructible:
    case 155:
    case 85:
    case kEntityKindWobblyProp:
    case 214:
        return true;
    case kKindPathed:
        return candidate.pathRecord;
    case kKindNever:
    default:
        return false;
    }
}
}

bool hasAbility(const AbilitySet& set, unsigned ability) { return ability < 64 && (set[ability >> 3] & (1u << (ability & 7))) != 0; }

bool hasAnyAbility(const AbilitySet& set, const AbilitySet& wanted) {
    for (std::size_t i = 0; i < set.size(); ++i)
        if (set[i] & wanted[i]) return true;
    return false;
}

bool isAttackCharacter(const AttackEntity& entity) {
    return entity.kind == kEntityKindCharacter || entity.kind == kEntityKindBossCharacter || entity.listedKind || (entity.word & kEntityWordBoss) != 0;
}

bool targetModeAllows(const AttackEntity& candidate, const AttackEntity& attacker) {
    switch (candidate.character.targetMode) {
    case 0:
        return true;
    case 2:
        return candidate.character.modeTargetIsAttacker;
    case 3:
        return attacker.player;
    default:
        return false;
    }
}

bool attackCandidatePasses(const AttackEntity& candidate, const AttackEntity& attacker, unsigned flags) {
    if (candidate.flags & kEntityFlagGone) return false;
    if (!(candidate.flags & kEntityFlagCollision)) return false;
    if ((flags & kAttackGateVisible) && candidate.hidden) return false;
    if (!isAttackCharacter(candidate)) return passesNonCharacter(candidate);
    const int state = candidate.character.state;
    if (state == kStateDropped0 || state == kStateDropped1 || state == kStateGrabbed0 || state == kStateGrabbed1) return false;
    if ((flags & kAttackGateNotState19) && state == kState19) return false;
    const bool same = sameSide(candidate, attacker);
    if (flags & kAttackGatePlayerSides) {
        if (!attacker.player && same) return false;
    } else if (same ? (flags & kAttackGateSameSide) : (flags & kAttackGateOtherSide)) {
        return false;
    }
    return targetModeAllows(candidate, attacker);
}

float attackWeight(const AttackEntity& candidate) {
    if (candidate.hidden) return 1.0f;
    if (candidate.word & kEntityWordBoss) return 0.3f;
    if (candidate.player) return 0.25f;
    if (candidate.kind == kKindDestructible) return 0.4f;
    if (!isAttackCharacter(candidate)) return 1.0f;
    if (candidate.character.state == kState19) return 1.0f;
    if (candidate.word & kEntityWordNoHitReset) return 1.0f;
    if (!(candidate.word & kEntityWordBadGuy)) return 1.0f;
    return 0.4f;
}

float attackSearchAngle(const AttackEntity& candidate, float angle, bool allAround) {
    return (candidate.word & kEntityWordBadGuy) && allAround ? kBadGuyAngle : angle;
}

bool grabStateAllows(int state) {
    if (state >= 0x122 && state <= 0x12f) return false;
    switch (state) {
    case 0x134:
    case 0x135:
    case 0x139:
    case 0x13a:
    case 0x13b:
    case 0x13c:
    case 0x13d:
    case 0x13e:
    case kStateGrabbed0:
    case kStateGrabbed1:
        return false;
    default:
        return true;
    }
}

bool fightRefused(const AttackCharacter& character) {
    return character.classFlag0 || (character.state == kStateFightWait && hasAbility(character.abilities, kAbilityFightWait));
}

std::vector<FightMove> fightMoves(const assets::Table& combat) {
    std::vector<FightMove> moves;
    if (combat.sheets.size() < 2) return moves;
    const assets::TableSheet& sheet = combat.sheets[1];
    for (std::size_t row = 0; row < kFightMoveCount && row < sheet.rows; ++row) {
        FightMove move;
        move.pairA = assets::tableValue(sheet, row, 1).asFloat();
        move.pairB = assets::tableValue(sheet, row, 2).asFloat();
        move.clip = int(assets::tableValue(sheet, row, 3).bits);
        move.ability = std::uint8_t(assets::tableValue(sheet, row, 6).bits);
        moves.push_back(move);
    }
    return moves;
}

AbilitySet fightMoveAbilities(const std::vector<FightMove>& moves) {
    AbilitySet set{};
    for (const FightMove& move : moves)
        if (move.ability < 64) set[move.ability >> 3] |= std::uint8_t(1u << (move.ability & 7));
    return set;
}

std::optional<int> chooseFightMove(int last, const AbilitySet& player, const std::vector<FightMove>& moves, GameRandom& random) {
    std::vector<int> allowed;
    for (std::size_t i = 0; i < moves.size() && i < kFightMoveCount; ++i)
        if (int(i) != last && hasAbility(player, moves[i].ability)) allowed.push_back(int(i));
    if (!allowed.empty()) return allowed[random.next() % allowed.size()];
    if (last >= 0 && std::size_t(last) < moves.size() && hasAbility(player, moves[std::size_t(last)].ability)) return last;
    return std::nullopt;
}

bool fightTargetPasses(const AttackEntity& target, const AttackEntity& player, const AbilitySet& fightMoveAbilities, bool clearLine) {
    if (!isAttackCharacter(target)) return false;
    if (!targetModeAllows(target, player)) return false;
    if (fightRefused(target.character)) return false;
    if (!player.character.onGround || !target.character.onGround) return false;
    if (!(std::fabs(player.character.groundY - target.character.groundY) <= kFightGroundGap)) return false;
    if (target.word & kEntityWordBoss) return false;
    if (target.character.heldByOther) return false;
    if (!hasAnyAbility(player.character.abilities, fightMoveAbilities)) return false;
    return clearLine;
}

}
