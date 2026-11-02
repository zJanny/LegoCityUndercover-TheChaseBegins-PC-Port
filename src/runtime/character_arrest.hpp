#pragma once
// The player's arrest of a level character (docs/re/combat-characters-arrest.md, entry 2026-11-02 "The arrest by A"): the A tap of 0x1fdbd4
// (0x1fdf70..0x1fe08c, 0x1fe774..0x1fe9f8), the arrest test 0x2107e0 with the pair test 0x21e0d4, the player's state ARRESTING 322 (enter 0x26ac14,
// update 0x28ba78) and the target's states ARREST 323 (enter 0x1c7208, update 0x26abcc, exit 0x1b8150) and ARRESTED 324 (enter 0x25ebe8, update
// 0x282e6c, exit 0x1d4af0), registered from the records at 0x357b8c by 0x2b8560. No SDL dependency.
#include "runtime/character_takedown.hpp"
#include "runtime/player_attack.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

inline constexpr std::uint16_t kArrestingState = 322;   // 0x142 ARRESTING, the player
inline constexpr std::uint16_t kArrestState = 323;      // 0x143 ARREST, the target
inline constexpr std::uint16_t kArrestedState = 324;    // 0x144 ARRESTED, the target
// 0x26ac14: 0x21941c(blend 0.1, speed 1.0, player, tablesanimations sheet 1 row 0x13a `cuffing`, mode 0).
inline constexpr int kArrestingClip = 314;
// 0x28bac0: the player turns to the target by at most 0x1770 heading units per update (0x23df04).
inline constexpr std::int32_t kArrestingTurnStep = 0x1770;
// 0x282e84: ARRESTED counts ctrl+0x24c up by the real dt and ends when it passes 1.0.
inline constexpr float kArrestedTime = 1.0f;
// 0x1c7278: ARREST sets the target mode ctrl+0xda & 7 to 4 (nobody); 0x25ecc0: ARRESTED sets it to 1.
inline constexpr std::uint8_t kArrestTargetModeNobody = 4;
inline constexpr std::uint8_t kArrestedTargetMode = 1;
// The clips of the sheet 2 row of the combat record byte +0xe: ARREST plays u16 +6 (0x1c725c), ARRESTED u16 +8 with mode 1 (0x25ec40).
inline constexpr std::size_t kArrestClipColumn = 3;
inline constexpr std::size_t kArrestedClipColumn = 4;

// 0x21e0d4: a character paired with another (fight, grab, struggle states); in the attack states 163 / 164 only for the attack record byte +9
// 4..10.
bool characterPaired(std::uint16_t state, std::int8_t attackKind);

// What 0x2107e0 reads of the target.
struct ArrestCandidate {
    std::uint16_t state = kCharacterStateIdle;  // ctrl+0x6c
    std::int8_t attackKind = 0;                 // the attack record byte +9 (read by 0x21e0d4 in 163 / 164 only)
    std::uint8_t arrestHealth = 1;              // sub +0x122
    bool avoidArrestTrigger = false;            // 0x23cff4(*0x376000, entity): an enabled CharacterAvoidArrest trigger of the character
    std::uint8_t arrestDamage = 0;              // sheet 2 byte +10 of the row of the combat record byte +0xe
};
// 0x2107e0: not paired, arrest health 0, no CharacterAvoidArrest trigger, in a stun state whose enter lowers the arrest health (0x123, 0x125,
// 0x127, 0x129, 0x12b, 0x12d, 0x12f, 0x134, 0x135, 0x13c) and a row with a non-zero arrest damage.
bool arrestTargetPasses(const ArrestCandidate& target);

// 0x1fdbd4 on an A tap (ctrl+0xe bit 0x80, the tap of 0x1bb690 for ctrl+0xc bit 0x80) with nothing carried: the search 0x219c08 with the player's
// range ctrl+0x80 (0x21a1d0), the angle *0x372e4c, the flags 9 and characters only (fifth argument 1); the target it finds is arrested when it
// passes 0x2107e0. The search looks along `heading` (ctrl+6 while the stick is held, else the facing). `arrestState` answers 0x2107e0's facts for a
// candidate name, none when the character has none (not arrestable).
std::optional<std::size_t> chooseArrestTarget(const AttackBody& attacker, const AttackEntity& player, std::int16_t heading,
                                              const std::vector<AttackCandidate>& candidates,
                                              const std::function<std::optional<ArrestCandidate>(const std::string&)>& arrestState);

// 0x23df04(step, from, to): from turns toward to by at most `step` heading units, the difference taken in the short direction.
std::int16_t turnHeadingToward(std::int32_t step, std::int16_t from, std::int16_t to);

struct PlayerArrestStep {
    bool active = false;     // the player was in ARRESTING at the start of this update
    bool finished = false;   // the clip finished: the update requested IDLE (1), entered in this update
};

// The player's ARRESTING: start() is the enter in the update of the A tap (the caller plays `cuffing`, `clipUpdates` long); each later update turns the facing (ctrl+8, also ctrl+0xa) toward the target's node and,
// once the clip is done (slot kind 6), requests IDLE. There is no mover call: the player stands.
class PlayerArrest {
public:
    void start(const std::string& target, unsigned clipUpdates);
    PlayerArrestStep update(LocomotionState& motion, std::optional<std::int16_t> targetHeading);
    bool active() const { return active_; }
    const std::string& target() const { return target_; }

private:
    bool active_ = false;
    unsigned clipUpdates_ = 0;
    unsigned elapsed_ = 0;
    std::string target_;
};

struct ArrestStep {
    std::optional<int> clip;     // tablesanimations sheet 1 row started this update (0x21941c)
    bool clipLoops = false;      // ARRESTED passes mode 1 (STAND-IN COMBAT-S6: played once and held)
    bool unlisted = false;       // the exit of the takedown state or the enter of ARRESTED (0x21e700) removed the character from the list
    bool arrested = false;       // ARRESTED raised CharacterArrested (0x23cee8(*0x375ff8, entity, entity, 255, 0))
    bool done = false;           // ARRESTED ended: entity +0xc bit 4 (destroyed) and +0x10 bit 0 (disabled, 0x2428c0)
};

// The target's ARREST and ARRESTED. start() is the request 0x1fe9e0 makes from the player's update; the next update() takes it over from the
// takedown states (their exit runs) in the original's order: the state update (its request is entered at once), then a pending request (exit,
// enter). ARREST and ARRESTED have no per-update hook.
class CharacterArrestStates {
public:
    void start() { pending_ = kArrestState; }
    ArrestStep update(TakedownCharacter& character, CharacterHitTarget& target, const TakedownTables& tables, ArrestableList& list,
                      CharacterTakedownStates& takedown, const std::string& name, bool clipFinished, float dt);
    std::uint16_t state() const { return state_; }
    bool pending() const { return pending_.has_value(); }
    bool active() const { return state_ != kCharacterStateIdle || pending_; }
    bool done() const { return done_; }

private:
    void enter(TakedownCharacter& character, const TakedownTables& tables, ArrestableList& list, const std::string& name, ArrestStep& step) const;

    std::uint16_t state_ = kCharacterStateIdle;
    std::optional<std::uint16_t> pending_;
    bool done_ = false;
};

}
