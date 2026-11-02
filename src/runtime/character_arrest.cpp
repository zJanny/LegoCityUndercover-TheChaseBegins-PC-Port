#include "runtime/character_arrest.hpp"
#include "runtime/player_intent.hpp"
#include "runtime/player_locomotion.hpp"

namespace chase::runtime {
namespace {

constexpr std::uint8_t kTargetModeMask = 7;

std::uint8_t withTargetMode(std::uint8_t select, std::uint8_t mode) { return std::uint8_t((select & ~kTargetModeMask) | mode); }
std::uint8_t restoredTargetMode(std::uint8_t select) { return std::uint8_t((select & ~kTargetModeMask) | ((select >> 3) & kTargetModeMask)); }

bool arrestableStunState(std::uint16_t state) {
    switch (state) {
    case 0x123: case 0x125: case 0x127: case 0x129: case 0x12b: case 0x12d: case 0x12f: case 0x134: case 0x135: case 0x13c: return true;
    default: return false;
    }
}

const TakedownMoveRow* moveRowOf(const TakedownCharacter& character, const TakedownTables& tables) {
    return character.moveRow < tables.moves.size() ? &tables.moves[character.moveRow] : nullptr;
}

}

bool characterPaired(std::uint16_t state, std::int8_t attackKind) {
    if (state == 163 || state == 164) return attackKind >= 4 && attackKind <= 10;
    if (state == 227 || (state >= 228 && state <= 231) || (state >= 254 && state <= 260)) return true;
    if (state >= 262 && state <= 273) return true;
    return state == 419 || state == 420;
}

bool arrestTargetPasses(const ArrestCandidate& target) {
    if (characterPaired(target.state, target.attackKind) || target.arrestHealth != 0 || target.avoidArrestTrigger) return false;
    return arrestableStunState(target.state) && target.arrestDamage != 0;
}

std::optional<std::size_t> chooseArrestTarget(const AttackBody& attacker, const AttackEntity& player, std::int16_t heading,
                                              const std::vector<AttackCandidate>& candidates,
                                              const std::function<std::optional<ArrestCandidate>(const std::string&)>& arrestState) {
    AttackSearch search{kPlayerMeleeRangeStandIn, kGrabSearchAngle, kAttackSearchFlags, true};
    search.charactersOnly = true;
    const float angle = headingRadians(heading);
    const std::array<float, 3> direction{gameSin(angle), 0.0f, gameCos(angle)};
    const std::optional<std::size_t> found = findAttackTarget(search, player, attacker, direction, candidates);
    if (!found || !arrestState) return std::nullopt;
    const std::optional<ArrestCandidate> target = arrestState(candidates[*found].name);
    if (!target || !arrestTargetPasses(*target)) return std::nullopt;
    return found;
}

std::int16_t turnHeadingToward(std::int32_t step, std::int16_t from, std::int16_t to) {
    const std::int32_t start = std::uint16_t(from);
    const std::int32_t goal = std::uint16_t(to);
    if (start == goal) return from;
    std::int32_t difference = goal - start;
    if (difference < -32768) difference += 65536;
    if (difference > 32768) difference -= 65536;
    if (difference < -step) difference = -step;
    else if (step < difference) difference = step;
    return std::int16_t(std::uint16_t(start + difference));
}

void PlayerArrest::start(const std::string& target, unsigned clipUpdates) {
    active_ = true;
    clipUpdates_ = clipUpdates;
    elapsed_ = 1;
    target_ = target;
}

PlayerArrestStep PlayerArrest::update(LocomotionState& motion, std::optional<std::int16_t> targetHeading) {
    PlayerArrestStep step;
    if (!active_) return step;
    step.active = true;
    if (targetHeading) {
        motion.facing = turnHeadingToward(kArrestingTurnStep, motion.facing, *targetHeading);
        motion.lastIntent = motion.facing;
    }
    if (elapsed_ >= clipUpdates_) {
        step.finished = true;
        active_ = false;
        target_.clear();
        return step;
    }
    ++elapsed_;
    return step;
}

ArrestStep CharacterArrestStates::update(TakedownCharacter& character, CharacterHitTarget& target, const TakedownTables& tables, ArrestableList& list,
                                         CharacterTakedownStates& takedown, const std::string& name, bool clipFinished, float dt) {
    ArrestStep step;
    if (done_) return step;
    if (state_ == kArrestState && clipFinished) pending_ = kArrestedState;
    if (state_ == kArrestedState) {
        character.timer += dt;
        if (kArrestedTime < character.timer) {
            done_ = true;
            character.destroyed = true;
            target.destroyed = true;
            step.done = true;
            return step;
        }
    }
    if (!pending_) return step;
    const std::uint16_t next = *pending_;
    pending_.reset();
    if (state_ == kCharacterStateIdle) {
        step.unlisted = takedown.handOver(next, character, target, list, name).unlisted;
    } else {
        character.selectByte = restoredTargetMode(character.selectByte);
        takedown.handOver(next, character, target, list, name);
    }
    state_ = next;
    target.state = next;
    enter(character, tables, list, name, step);
    return step;
}

void CharacterArrestStates::enter(TakedownCharacter& character, const TakedownTables& tables, ArrestableList& list, const std::string& name,
                                  ArrestStep& step) const {
    const TakedownMoveRow* row = moveRowOf(character, tables);
    character.timer = 0;
    if (state_ == kArrestState) {
        if (row) step.clip = row->clips[kArrestClipColumn];
        character.selectByte = withTargetMode(character.selectByte, kArrestTargetModeNobody);
        return;
    }
    if (row) step.clip = row->clips[kArrestedClipColumn], step.clipLoops = true;
    if (list.contains(name)) {
        list.remove(name);
        step.unlisted = true;
    }
    step.arrested = true;
    character.selectByte = withTargetMode(character.selectByte, kArrestedTargetMode);
}

}
