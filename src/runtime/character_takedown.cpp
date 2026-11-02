#include "runtime/character_takedown.hpp"
#include "runtime/level_start.hpp"

#include <algorithm>
#include <cfloat>
#include <cstring>

namespace chase::runtime {
namespace {

constexpr std::size_t kSheetFightMoves = 1, kSheetTakedownMoves = 2, kSheetTimings = 5;
constexpr std::size_t kFightRowColumn = 4, kFightRowAbility51Column = 5;
constexpr std::size_t kTimingCount = 12;
constexpr std::uint8_t kTargetModeMask = 7;
constexpr std::uint8_t kTargetModeNobody = 1;
constexpr std::uint8_t kRidePigKind = 159;
constexpr std::uint16_t kAiStateNoPursuit = 82;

// What the enter 0x28cbf8 does for each takedown state (jump table 0x28cc38): which clip of the move row it starts and with which mode, which time
// of the timing rows it takes (byte offset / 4, -1 for none, kTimeRandom for the PUTDOWN STUN draw between +4 and +8), and whether its HIT variant
// falls when in the air (r7 at 0x28ccac).
constexpr int kNoClip = -1, kNoTime = -1, kTimeRandom = -2, kTimeFixed = -3;
struct EnterRule {
    int clipColumn = kNoClip;
    bool loops = false;
    int time = kNoTime;
    bool airFall = false;
};

EnterRule enterRule(std::uint16_t state) {
    switch (state) {
    case 0x122: case 0x124: case 0x126: case 0x128: case 0x12a: return {0, false, kNoTime, true};
    case 0x12c: case 0x12e: case 0x133: case 0x136: case 0x13b: case 0x13d: return {0, false, kNoTime, false};
    case 0x123: return {1, true, 3, false};
    case 0x125: return {1, true, 5, false};
    case 0x127: return {1, true, 8, false};
    case 0x129: return {1, true, 4, false};
    case 0x12b: return {1, true, 6, false};
    case 0x12d: return {1, true, 9, false};
    case 0x12f: return {1, true, 7, false};
    case 0x134: return {1, true, 0, false};
    case 0x135: return {1, true, kTimeRandom, false};
    case 0x13c: return {1, true, kNoTime, false};
    case 0x13e: return {1, true, kTimeFixed, false};
    case 0x139: case 0x13a: return {2, false, kNoTime, false};
    default: return {};
    }
}

// 0x28d2a0..0x28d300: the stun states whose enter lowers the arrest health.
bool dropsArrestHealth(std::uint16_t state) {
    switch (state) {
    case 0x123: case 0x125: case 0x127: case 0x129: case 0x12b: case 0x12d: case 0x12f: case 0x134: case 0x135: case 0x13c: return true;
    default: return false;
    }
}

// 0x283a3c..0x283ab4: the exits that remove the character from the arrestable list.
bool exitUnlists(std::uint16_t state) {
    switch (state) {
    case 0x123: case 0x125: case 0x127: case 0x129: case 0x12b: case 0x12d: case 0x12f: case 0x134: case 0x135: return true;
    default: return false;
    }
}

// 0x22ea0c: without a move record 0x22e884 returns 1 in these states (jump table 0x22ea24) before it looks at the kind.
bool takesNoPutdown(std::uint16_t state) {
    if (state < 0x122 || state > 0x144) return false;
    return !(state >= 0x130 && state <= 0x133) && state != 0x136 && state != 0x137 && state != 0x138 && !(state >= 0x13f && state <= 0x142);
}

bool registeredTakedownState(std::uint16_t state) {
    if (state < kTakedownFirstState || state > kTakedownLastState) return false;
    return !(state >= 0x130 && state <= 0x132) && state != 0x137 && state != 0x138;
}

std::uint8_t restoredSelect(std::uint8_t select) { return std::uint8_t((select & ~kTargetModeMask) | ((select >> 3) & kTargetModeMask)); }
std::uint8_t lockedSelect(std::uint8_t select) { return std::uint8_t((select & ~kTargetModeMask) | kTargetModeNobody); }

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

const TakedownTimingRow* timingRow(const TakedownTables& tables, std::uint8_t row) { return row < tables.timings.size() ? &tables.timings[row] : nullptr; }

float lerpTime(const TakedownTimingRow* row, std::uint32_t word) {
    if (!row) return 0;
    const float a = row->times[1], b = row->times[2];
    return a + (b - a) * takedownRandom(word);
}

}

TakedownTables takedownTables(const assets::Table& combat) {
    TakedownTables tables;
    if (combat.sheets.size() > kSheetFightMoves) {
        const assets::TableSheet& sheet = combat.sheets[kSheetFightMoves];
        for (std::size_t row = 0; row < sheet.rows; ++row) {
            tables.fightRows.push_back(std::uint8_t(assets::tableValue(sheet, row, kFightRowColumn).bits));
            tables.fightRowsAbility51.push_back(std::uint8_t(assets::tableValue(sheet, row, kFightRowAbility51Column).bits));
        }
    }
    if (combat.sheets.size() > kSheetTakedownMoves) {
        const assets::TableSheet& sheet = combat.sheets[kSheetTakedownMoves];
        for (std::size_t row = 0; row < sheet.rows; ++row) {
            TakedownMoveRow move;
            for (std::size_t i = 0; i < move.clips.size(); ++i) move.clips[i] = std::uint16_t(assets::tableValue(sheet, row, 1 + i).bits);
            move.arrestDamage = std::uint8_t(assets::tableValue(sheet, row, 6).bits);
            move.studs = std::uint8_t(assets::tableValue(sheet, row, 7).bits);
            tables.moves.push_back(move);
        }
    }
    if (combat.sheets.size() > kSheetTimings) {
        const assets::TableSheet& sheet = combat.sheets[kSheetTimings];
        for (std::size_t row = 0; row < sheet.rows; ++row) {
            TakedownTimingRow timing;
            for (std::size_t i = 0; i < kTimingCount; ++i) timing.times[i] = assets::tableValue(sheet, row, 1 + i).asFloat();
            tables.timings.push_back(timing);
        }
    }
    return tables;
}

TakedownTiming takedownTimingOf(const assets::LevelObject& object, const assets::Level& cls) {
    TakedownTiming timing;
    timing.stun = std::uint8_t(intParam(object, cls, "HitTimingStun", 0));
    timing.arrestable = std::uint8_t(intParam(object, cls, "HitTimingArrestable", 1));
    return timing;
}

float takedownRandom(std::uint32_t word) { return float(word & 0xffffu) * (1.0f / 65536.0f); }

bool ArrestableList::add(const std::string& character) {
    for (std::size_t i = 0; i < slots_.size(); ++i)
        if (slots_[i].empty()) {
            slots_[i] = character;
            timers_[i] = kArrestableTimerStart;
            return true;
        }
    return false;
}

void ArrestableList::remove(const std::string& character) {
    for (std::string& slot : slots_)
        if (slot == character) {
            slot.clear();
            return;
        }
}

bool ArrestableList::contains(const std::string& character) const {
    return std::find(slots_.begin(), slots_.end(), character) != slots_.end();
}

std::size_t ArrestableList::size() const {
    return std::size_t(std::count_if(slots_.begin(), slots_.end(), [](const std::string& s) { return !s.empty(); }));
}

std::optional<std::uint16_t> putdownRequest(const CharacterHitTarget& target, const TakedownCharacter& character) {
    if (target.kind == kRidePigKind || target.boss) return std::nullopt;
    if (takesNoPutdown(target.state)) return std::nullopt;
    return character.autoArrestFlag ? kTakedownAutoArrested : kTakedownPutdownStun;
}

void CharacterTakedownStates::startFightMove(int move) {
    fightMove_ = move;
    pending_ = kTakedownMeleePutdown;
}

TakedownStep CharacterTakedownStates::update(TakedownCharacter& character, CharacterHitTarget& target, const TakedownTables& tables,
                                             ArrestableList& list, const std::string& name, bool clipFinished, float dt,
                                             const std::function<std::uint32_t()>& random) {
    TakedownStep step;
    if (state_ == kTakedownMeleePutdown && clipFinished && !character.destroyed) {
        character.selectByte = restoredSelect(character.selectByte);
        character.grabbed = false;
        if (character.ability51) {
            pending_ = kCharacterStateIdle;
        } else {
            CharacterHitTarget sample = target;
            sample.state = state_;
            if (const std::optional<std::uint16_t> next = putdownRequest(sample, character)) pending_ = next;
        }
    }
    if (pending_) {
        const std::uint16_t next = *pending_;
        pending_.reset();
        exit(character, list, name, step);
        state_ = next;
        target.state = next;
        enter(character, target, tables, list, name, random, step);
    }
    hook(character, target, step.clip ? false : clipFinished, dt, step);
    if (step.request) pending_ = step.request;
    return step;
}

void CharacterTakedownStates::exit(TakedownCharacter& character, ArrestableList& list, const std::string& name, TakedownStep& step) const {
    if (!registeredTakedownState(state_)) return;
    character.selectByte = restoredSelect(character.selectByte);
    character.grabbed = false;
    if (exitUnlists(state_) && list.contains(name)) {
        list.remove(name);
        step.unlisted = true;
    }
    if (state_ == kTakedownAvoidArrest) step.avoidArrest = true;
}

void CharacterTakedownStates::enter(TakedownCharacter& character, CharacterHitTarget& target, const TakedownTables& tables, ArrestableList& list,
                                    const std::string& name, const std::function<std::uint32_t()>& random, TakedownStep& step) const {
    const auto moveRow = [&]() -> const TakedownMoveRow* { return character.moveRow < tables.moves.size() ? &tables.moves[character.moveRow] : nullptr; };
    if (state_ == kTakedownMeleePutdown) {
        const std::vector<std::uint8_t>& rows = character.ability51 ? tables.fightRowsAbility51 : tables.fightRows;
        if (fightMove_ >= 0 && std::size_t(fightMove_) < rows.size()) character.moveRow = rows[std::size_t(fightMove_)];
        if (const TakedownMoveRow* row = moveRow()) step.clip = row->clips[0];
        character.selectByte = lockedSelect(character.selectByte);
        character.grabbed = true;
        return;
    }
    if (!registeredTakedownState(state_)) {
        step.notPorted = state_ != kCharacterStateIdle;
        return;
    }
    const EnterRule rule = enterRule(state_);
    if (rule.clipColumn != kNoClip)
        if (const TakedownMoveRow* row = moveRow()) step.clip = row->clips[std::size_t(rule.clipColumn)], step.clipLoops = rule.loops;
    float stunTime = 0, arrestTime = 0;
    const TakedownTimingRow* stunRow = timingRow(tables, character.timing.stun);
    const TakedownTimingRow* arrestRow = timingRow(tables, character.timing.arrestable);
    if (rule.time == kTimeRandom) {
        stunTime = lerpTime(stunRow, random());
        arrestTime = lerpTime(arrestRow, random());
    } else if (rule.time >= 0) {
        stunTime = stunRow ? stunRow->times[std::size_t(rule.time)] : 0;
        arrestTime = arrestRow ? arrestRow->times[std::size_t(rule.time)] : 0;
    } else if (rule.time == kTimeFixed) {
        character.timer = kRunOverStunTime;
    }
    character.selectByte = lockedSelect(character.selectByte);
    character.grabbed = true;
    if (!dropsArrestHealth(state_)) return;
    character.timer = stunTime;
    const TakedownMoveRow* row = moveRow();
    if (!row || row->arrestDamage == 0) return;
    step.arrestHealthBefore = target.arrestHealth;
    step.effect265 = target.arrestHealth != 0;
    changeArrestHealth(target, -int(row->arrestDamage));
    if (target.arrestHealth != 0 || character.avoidArrestTrigger) return;
    character.arrestTime = arrestTime;
    character.timer = arrestTime;
    if (!list.add(name)) step.listFull = true;
    character.selectByte = restoredSelect(character.selectByte);
    step.arrestable = true;
}

void CharacterTakedownStates::hook(TakedownCharacter& character, const CharacterHitTarget& target, bool clipFinished, float dt, TakedownStep& step) const {
    if (!registeredTakedownState(state_)) return;
    const auto nextOnClip = [&](std::uint16_t next) {
        if (clipFinished) step.request = next;
    };
    switch (state_) {
    case 0x122: case 0x124: case 0x126: case 0x128: case 0x12a: case 0x12c: case 0x12e: case 0x13b: case 0x13d:
        nextOnClip(std::uint16_t(state_ + 1));
        break;
    case 0x133: case 0x136:
        nextOnClip(character.autoArrestFlag ? kTakedownAutoArrested : std::uint16_t(state_ + 1));
        break;
    case 0x123: case 0x125: case 0x127: case 0x129: case 0x12b: case 0x12d: case 0x12f: case 0x134: case 0x135:
        if (target.arrestHealth == 0 && character.avoidArrestTrigger) {
            step.request = kTakedownAvoidArrest;
            break;
        }
        character.timer -= dt;
        if (character.timer <= 0) step.request = kTakedownGenericGetUp;
        break;
    case kTakedownBossStun:
        break;
    case kTakedownRunOverStun:
        character.timer -= dt;
        if (character.timer <= 0) step.request = kTakedownGenericGetUp;
        break;
    default:
        nextOnClip(kCharacterStateIdle);
        break;
    }
}

AggroResult aggroOnHit(const AggroTarget& target, const AggroSender& sender) {
    AggroResult result;
    if (target.badGuy == sender.badGuy) return result;
    if (!target.pursuitActive) {
        result.pursuit = target.state != kAiStateNoPursuit;
        return result;
    }
    if (target.ignorePlayer) return result;
    const bool targetIsSender = target.target && *target.target == sender.name;
    if (targetIsSender || target.counter != 0) {
        if (targetIsSender && sender.player) result.counter = kAggroCounterUpdates;
        return result;
    }
    float distance = FLT_MAX;
    if (target.targetIsPlayer) distance = target.boss && !sender.player ? 0.0f : target.targetDistance;
    if (!(target.range < distance)) return result;
    result.switchTarget = true;
    result.counter = kAggroCounterUpdates;
    return result;
}

}
