#pragma once
// The takedown of a level character (docs/re/combat-characters.md, entry 2026-11-02 "The takedown states"): the fight move's target state MELEE PUTDOWN
// (227, enter 0x1d6d04, update 0x27726c), the takedown states 0x122..0x13e (registered at 0x279da0 with the update 0x283b10, the enter 0x28cbf8, the
// per-update hook 0x29ad90 and the exit 0x283a0c), the arrest health drop 0x2b900c, the list of arrestable characters 0x5187cc (add 0x192fd4, remove
// 0x21e700) and the events CharacterArrestable / CharacterAvoidArrest. The tables are tables/tablescombat.bxls sheets 1, 2 and 5 (*0x372e28,
// *0x372e34, *0x372e68, loader 0x2f6138). No SDL dependency.
#include "assets/assets.hpp"
#include "runtime/character_hit.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

inline constexpr std::uint16_t kTakedownMeleePutdown = 227;     // 0xe3 MELEE PUTDOWN, the target state of the fight move 0x1d3980
inline constexpr std::uint16_t kTakedownFirstState = 0x122;     // ENERGYGUN HIT
inline constexpr std::uint16_t kTakedownLastState = 0x13e;      // RUN OVER STUN
inline constexpr std::uint16_t kTakedownPutdownStun = 0x135;    // 309 PUTDOWN STUN
inline constexpr std::uint16_t kTakedownAutoArrested = 0x137;   // 311 AUTO ARRESTED
inline constexpr std::uint16_t kTakedownGenericGetUp = 0x139;   // 313 GENERIC GETUP
inline constexpr std::uint16_t kTakedownAvoidArrest = 0x13a;    // 314 AVOID ARREST
inline constexpr std::uint16_t kTakedownBossStun = 0x13c;       // 316 BOSS STUN
inline constexpr std::uint16_t kTakedownRunOverStun = 0x13e;    // 318 RUN OVER STUN

// The record the MELEE PUTDOWN update hands to 0x22e884 when its clip finished (0x2773d4): no move, no sender, damage 1, kind 16.
inline constexpr std::uint8_t kHitKindPutdown = 16;
// 0x28d228: the timer of RUN OVER STUN (literal 0x28d410).
inline constexpr float kRunOverStunTime = 2.0f;
// 0x192fd4: eight slots of 0x40 bytes at 0x5187cc; the slot's timer starts with -1.0 (0x23cc00, literal 0x19301c).
inline constexpr std::size_t kArrestableSlots = 8;
inline constexpr float kArrestableTimerStart = -1.0f;
// 0x1d6d04 and 0x28cbf8 start their clip with 0x21941c(blend 0.1, speed 1.0).
inline constexpr float kTakedownClipBlend = 0.1f;
inline constexpr float kTakedownClipSpeed = 1.0f;

// tables/tablescombat.bxls sheet 2 (*0x372e34, 12-byte rows): five clips (u16 +0 grabbed, +2 stun, +4 get up, +6, +8), the arrest damage byte +10
// (0x28d320) and the stud count byte +11 (the strike loop 0x1d6b20).
struct TakedownMoveRow {
    std::array<std::uint16_t, 5> clips{};
    std::uint8_t arrestDamage = 0;
    std::uint8_t studs = 0;
};
// Sheet 5 (*0x372e68, 60-byte rows by HitTimingStun / HitTimingArrestable): the twelve times in seconds at +0..+44 (the two ints and the float after
// them are not read by the takedown).
struct TakedownTimingRow {
    std::array<float, 12> times{};
};
// Sheet 1 (*0x372e28, the fight moves): byte +0xa is the takedown row of a target, +0xb the row of a target with ability 51 (0x1d6d98).
struct TakedownTables {
    std::vector<std::uint8_t> fightRows, fightRowsAbility51;
    std::vector<TakedownMoveRow> moves;
    std::vector<TakedownTimingRow> timings;
};
TakedownTables takedownTables(const assets::Table& combat);

// `HitTimingStun` (default 0, sub +0x12c) and `HitTimingArrestable` (default 1, +0x12d) of the create 0x236d00, read as integers.
struct TakedownTiming {
    std::uint8_t stun = 0;
    std::uint8_t arrestable = 1;
};
TakedownTiming takedownTimingOf(const assets::LevelObject& object, const assets::Level& cls);

// 0x2345dc: a random float in [0, 1), the low 16 bits of 0x25b2b8 times 2^-16.
float takedownRandom(std::uint32_t word);

// What the takedown reads and writes of the character besides the arrest health (CharacterHitTarget::arrestHealth).
struct TakedownCharacter {
    TakedownTiming timing;
    std::uint8_t selectByte = 0;    // ctrl+0xda: bits 0..2 the target mode, bits 3..5 the mode the takedown restores (0 for the goons: oracle D-1)
    bool grabbed = false;           // entity +0xc bit 21, set by the enters, cleared by the update 0x27726c and the exit 0x283a0c
    float timer = 0;                // ctrl+0x24c, counted down by the real dt (0x23602c)
    float arrestTime = 0;           // sub +0x9c
    std::uint8_t moveRow = 0;       // combat record (*(sub+0)) byte +0xe: the row of sheet 2
    bool ability51 = false;         // 0x23a70c(ctrl, 51)
    bool avoidArrestTrigger = false; // 0x23cff4(*0x376000, entity): an enabled CharacterAvoidArrest trigger owned by the character
    bool autoArrestFlag = false;    // sub +0x131 & 0xc0 (writer not read: STAND-IN COMBAT-S4, always clear)
    bool destroyed = false;         // entity +0xc bit 4
};

// 0x192fd4 / 0x21e700: the first free of eight slots takes the character; removal clears its slot.
class ArrestableList {
public:
    bool add(const std::string& character);
    void remove(const std::string& character);
    bool contains(const std::string& character) const;
    std::size_t size() const;

private:
    std::array<std::string, kArrestableSlots> slots_;
    std::array<float, kArrestableSlots> timers_{};
};

struct TakedownStep {
    std::optional<int> clip;                   // tablesanimations sheet 1 row started this update (0x21941c)
    bool clipLoops = false;                    // the stun clips pass mode 1 to 0x21941c (STAND-IN COMBAT-S4: played once and held)
    std::optional<std::uint16_t> request;      // 0x2414b4 of the hook, taken by the next update
    std::optional<int> arrestHealthBefore;     // set when the enter lowered the arrest health (0x2b900c)
    bool effect265 = false;                    // 0x2b900c starts effect 265 0.5 above a named joint (not ported, COMBAT-8)
    bool arrestable = false;                   // listed (0x192fd4) and CharacterArrestable raised (0x23cee8(*0x376004, entity, entity, 255, 0))
    bool listFull = false;                     // the list had no free slot
    bool unlisted = false;                     // the exit removed the character from the list (0x21e700)
    bool avoidArrest = false;                  // the exit of AVOID ARREST raised CharacterAvoidArrest (0x23cee8(*0x376000, ...))
    bool notPorted = false;                    // a request of 0x22e884 / the takedown table this port does not run (logged)
};

// The takedown states of one character. A request of request() or of the hook is entered by the next update().
class CharacterTakedownStates {
public:
    // The fight move 0x1d3980 asked the character for MELEE PUTDOWN with fight move `move` (pair record +0x478).
    void startFightMove(int move);
    void request(std::uint16_t state) { pending_ = state; }
    // One update in the original's order (oracle 20261102-mission14-B-1): the update of MELEE PUTDOWN (0x27726c, its request is taken at once),
    // then the exit and enter of a requested state, then the per-update hook 0x29ad90 of the state (a request it makes is taken by the next
    // update). `clipFinished` is slot kind 6 of 0x22f560 for the clip playing before this update (a clip started in it is not finished), `dt` the
    // real update time, `random` the next word of the game generator (0x25b2b8).
    TakedownStep update(TakedownCharacter& character, CharacterHitTarget& target, const TakedownTables& tables, ArrestableList& list,
                        const std::string& name, bool clipFinished, float dt, const std::function<std::uint32_t()>& random);
    std::uint16_t state() const { return state_; }
    std::optional<std::uint16_t> pending() const { return pending_; }
    bool active() const { return state_ != kCharacterStateIdle; }
    int fightMove() const { return fightMove_; }

private:
    void exit(TakedownCharacter& character, ArrestableList& list, const std::string& name, TakedownStep& step) const;
    void enter(TakedownCharacter& character, CharacterHitTarget& target, const TakedownTables& tables, ArrestableList& list, const std::string& name,
               const std::function<std::uint32_t()>& random, TakedownStep& step) const;
    void hook(TakedownCharacter& character, const CharacterHitTarget& target, bool clipFinished, float dt, TakedownStep& step) const;

    std::uint16_t state_ = kCharacterStateIdle;
    std::optional<std::uint16_t> pending_;
    int fightMove_ = -1;
};

// 0x22e884 without a move record for the kinds the takedown sends (16 from the MELEE PUTDOWN update): PUTDOWN STUN, or AUTO ARRESTED with sub
// +0x131 & 0xc0; none for a ride pig (kind 159), a boss or a character already in a takedown state.
std::optional<std::uint16_t> putdownRequest(const CharacterHitTarget& target, const TakedownCharacter& character);

// The aggro 0x1d1170(target, sender) the receiver 0x1b8570 calls for a character sender (CharacterHitResult::aggro).
struct AggroTarget {
    bool badGuy = false;            // entity +0xc bit 2
    bool boss = false;              // entity +0xc bit 14
    bool pursuitActive = false;     // ctrl+0xd8 bit 5 (meaning not read; 0x21102c sets the pursuit only with it)
    bool ignorePlayer = false;      // ctrl+0xd9 bit 0 (AIIgnorePlayer)
    std::uint16_t state = kCharacterStateIdle;  // ctrl+0x6c
    std::optional<std::string> target;          // ctrl+0xc4
    bool targetIsPlayer = false;
    float targetDistance = 0;       // 0x2507dc between the node positions of the character and ctrl+0xc4
    float range = 0;                // ctrl+0x80
    std::uint8_t counter = 0;       // ctrl+0x72
};
struct AggroSender {
    std::string name;
    bool badGuy = false;
    bool player = false;
};
struct AggroResult {
    // 0x21102c was called (ctrl+0xdc bits 3..4 = 2, the path record 0x206d18; the AI request 2 at ctrl+0x15 and ctrl+0xc4 = sender only when
    // ctrl+0xd8 bit 5 is set afterwards, which 0x1d1170 then also needs to go on: STAND-IN COMBAT-S5 takes it as still clear).
    bool pursuit = false;
    bool switchTarget = false;      // 0x2080a0(target, sender)
    std::optional<std::uint8_t> counter;  // ctrl+0x72 = 3 x 0x23ca4c() (the update rate 30)
};
inline constexpr std::uint8_t kAggroCounterUpdates = 3 * 30;
AggroResult aggroOnHit(const AggroTarget& target, const AggroSender& sender);

}
