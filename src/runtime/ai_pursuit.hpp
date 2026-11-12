#pragma once
// The fight states of an AI character (docs/re/ai-states.md, entry of 2026-11-12): the decisions of AI state 2 (0x2c4708 -> 0x2bb558: hold near the
// target or hand over to the attack), 3 (0x2b82ec -> 0x2a92a4 with the attack routine 0x2896d4), 4 and 5 (0x2c5d3c: the run of the move front end
// 0x22d85c and of the chase 0x2a136c) and 31 (0x2c9d24, the wind-up of the strike), with the helpers they share: the attack ticket 0x1f6b40, the
// weapon mode 0x1f6d90, the hand-over 0x2080a0, the range bound keeper 0x1faa6c and the per-side attack cool-down 0x26aa1c. The decisions run on
// the entities of the idle conduct (goon_conduct.hpp), whose update calls them through the state table below. No SDL dependency.
#include "runtime/goon_conduct.hpp"

#include <array>
#include <cstdint>

namespace chase::runtime {

constexpr std::uint8_t kAiPursueState = 2;        // 0x21102c
constexpr std::uint8_t kAiAttackState = 3;        // 0x2080a0
constexpr std::uint8_t kAiMoveState = 4;          // 0x22d85c
constexpr std::uint8_t kAiChaseState = 5;         // 0x2a9708
constexpr std::uint8_t kAiTauntState = 16;        // 0x2bb874
constexpr std::uint8_t kAiWindUpState = 31;       // 0x2b842c
constexpr std::uint8_t kAiChargeState = 33;       // 0x2b83a4

constexpr int kAiAbilityMelee = 19;               // 0x2bc998 with the row byte +0x17
constexpr int kAiAbilityRanged = 20;              // 0x21e7a4 with the row byte +0x18
constexpr int kAiAbilityStrike = 54;              // 0x2b83c0
constexpr int kAiAbilityCharge = 63;              // 0x2b830c
constexpr std::size_t kCharacterRowMeleeWeapon = 0x17;
constexpr std::size_t kCharacterRowRangedWeapon = 0x18;
constexpr std::size_t kWeaponRowAbility = 22;     // u16 +0x16 of the 24-byte weapon row (0x21e7e4)

constexpr float kAiHoldFactor = 3.0f;             // literal 0x2bba28: without a ticket the character holds at 3 x AIMeleeFireDistance
constexpr float kAiTicketApproachDivisor = 2.0f;  // literal 0x2bba2c: with a ticket and a target outside the bound, range / (2 x distance)
constexpr float kAiChaseFactor = 0.8f;            // literal 0x2a14ec: the chase ends 0.8 x range from the target
constexpr float kAiChaseFallbackRange = 3.0f;     // literal 0x2a14f4: the second path request needs a range above 3
constexpr float kAiRepathDistance = 5.0f;         // literal 0x2c6170: a target at least max(range, 5) from the destination asks for a new path
constexpr float kAiStrikeReach = 3.0f;            // literal 0x2b84dc, the reach 0x1f5114 takes from the decision of state 3
constexpr float kAiWindUpReach = 4.5f;            // literal 0x2c9edc, the reach of the wind-up states 304 and 305
constexpr float kAiMeleeModeFactor = 1.5f;        // literal 0x1f6e3c
constexpr std::uint32_t kAiRetryBase = 10;        // 0x2bb968: ctrl+0x84 = 10 + random % 10
constexpr std::uint32_t kAiRetrySpan = 10;
constexpr std::uint32_t kAiMoveCountdown = 5;     // 0x22da08: ctrl+0x84 = 5
constexpr std::uint32_t kAiTauntChance = 5;       // 0x2bb794: 0x235498(5) answers 0
constexpr std::uint32_t kAiIdleAttackBase = 30;   // 0x289a24: weapon mode 0 waits 30 + random % 30
constexpr std::uint32_t kAiIdleAttackSpan = 30;
constexpr std::uint16_t kAiSideCooldown = 10;     // 0x289910: the words 0x3771da / 0x3771dc
constexpr std::int32_t kAiFacingWindow = 0x13fe;  // literal 0x289b64: facing - turn + window / 2 must not exceed the window
constexpr unsigned kAiMeleeAttackersLimit = 1;    // word 0x3771e0 (initial value, no store found)
constexpr unsigned kAiRangedAttackersLimit = 2;   // word 0x3771e4
constexpr std::uint16_t kAiPressMelee = 0x20;     // ctrl+0xe bits the attack routine sets (0x2899a0, 0x2899cc, 0x2899fc)
constexpr std::uint16_t kAiPressRanged = 0x40;
constexpr std::uint16_t kAiPressOther = 0x80;
constexpr std::uint8_t kAiWeaponRanged = 1;       // ctrl+0x2cd bits 0x2106d8 tests: 0 the ranged weapon, 1 the melee weapon, 2 the third
constexpr std::uint8_t kAiWeaponMelee = 2;
constexpr std::uint8_t kAiWeaponThird = 4;

// STAND-IN SCRIPT-S341: inputs of the fight decisions the port does not track. The byte 0x377540 (anded with 0x3771d7 in the mill-about branches of
// 0x22d85c, 0x2a92a4 and 0x2c5d3c) reads 0 as in the image; the character's own ctrl+0x316 bit 5, without which state 2 never taunts, is clear; the
// ray of `AILineOfSight` (0x22b108) finds nothing in the way; the turn ctrl+8 has reached the facing ctrl+10 when the attack routine tests it.
constexpr bool kAiGlobalMillAboutModeStandIn = false;
constexpr bool kAiTauntBitStandIn = false;
constexpr bool kAiSightRayBlockedStandIn = false;

// 0x2bcea0: `AICombatBehaviour` & 7 adjusted by what the character can do. 0 becomes 2 with melee only, 1 with ranged only and 3 with neither;
// 1 without ranged and 2 without melee become 3.
std::uint8_t aiCombatBehaviour(std::uint8_t raw, bool melee, bool ranged);

// 0x1f6d90(entity, ctrl) with the distance to the target: the weapon mode the character wants (1 ranged, 2 melee, 3 none, 4 the third weapon).
std::uint8_t aiWeaponMode(std::uint8_t behaviour, bool melee, bool ranged, float meleeDistance, float fireDistance, float distance);

// What the AI set-up 0x2bc934 reads of the character row (`CharacterEnum`, tables/tablescharacters.bxls sheet 0) and tables/tablesweapons.bxls
// sheet 0: ctrl+0xdb bit 0 (ability 19 and a melee weapon row), bit 1 (0x21e798: ability 20, a ranged weapon row and that row's ability), and the
// abilities 54 and 63 the decision of state 3 asks for.
struct AiCombatFacts {
    bool melee = false;
    bool ranged = false;
    bool strike = false;
    bool charge = false;
};
AiCombatFacts aiCombatFacts(const assets::Table& characters, const assets::Table& weapons, unsigned characterRow);
// Fills the fields of `params` that come from the facts and adjusts its combat behaviour.
void applyAiCombatFacts(AiConductParams& params, const AiCombatFacts& facts);

// target + (from - target) x (reach / |from - target|); `from` itself when both are at one point.
std::array<float, 3> aiPointNear(const std::array<float, 3>& target, const std::array<float, 3>& from, float reach);

// One record of the state table *0x3771f0 the port runs here: the decision (word +0) and the enter (word +4, null when the table has none).
struct AiFightState {
    std::uint8_t state;
    void (*decision)(AiConduct&, AiConductEntity&, const AiConductBoundTest&);
    void (*enter)(AiConduct&, AiConductEntity&);
    const char* name;
};
const AiFightState* aiFightState(std::uint8_t state);
const char* aiFightStateName(std::uint8_t state);

// 0x1f6b40(entity): 1 unless a boss asks; 0 when another bad guy with its melee weapon out attacks the same target (limit 0x3771e0), or, for a
// character with its ranged weapon out, when two with theirs out do (limit 0x3771e4).
bool aiAttackTicket(const AiConduct& conduct, const AiConductEntity& self);
// 0x2080a0(entity, target): AI state 3 on the target, ctrl+0xb8 = the own position, ctrl+0xd9 bit 2 cleared.
void aiAttackTarget(AiConduct& conduct, AiConductEntity& self, const AiConductEntity& target);
// 0x22d85c(entity, destination, avoid, walk, 0): the path request and AI state 4; false when the destination and the character are both outside
// the range bound, no slot is free or the path finder cannot get there.
bool aiMoveTo(AiConduct& conduct, AiConductEntity& self, const std::array<float, 3>& destination, bool avoid, bool walk, const AiConductBoundTest& inside);
// 0x2a136c(entity, 0, walk, range): the run to 0.8 x range from the target; false when the character is that near already.
bool aiChaseTarget(AiConduct& conduct, AiConductEntity& self, const AiConductEntity& target, float range, bool walk, const AiConductBoundTest& inside);
// 0x1faa6c(entity): keeps the character inside its range bound.
void aiKeepInBound(AiConduct& conduct, AiConductEntity& self, const AiConductBoundTest& inside);
// 0x26aa1c, once per update: both side cool-downs count down to 0.
void aiSideCooldownStep(std::array<std::uint16_t, 2>& cooldowns);

}
