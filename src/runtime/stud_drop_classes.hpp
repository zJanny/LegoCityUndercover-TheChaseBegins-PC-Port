#pragma once
// Which classes drop studs and the requests of their callers (docs/re/studs.md, entry "Which classes drop studs"): the generic state machine 0x22c6d0
// (ten kinds, the drop of 0x22c8a0 -> 0x22ca88), the player's hurt 0x20bd18, the player's death 0x26d1f0, the hit-sequence slots 0x2b7894 and the
// reward sites of the ability props that call 0x23a6b0 with `StudValue` (default 25). Nothing here runs a class: the kinds' own state machines are not
// ported, so the glue has no call yet (STUD-26). No SDL dependency.
#include "assets/assets.hpp"
#include "runtime/stud_callers.hpp"
#include "runtime/stud_drop.hpp"

#include <array>
#include <cstdint>
#include <optional>

namespace chase::runtime {

// The kinds whose update (descriptor slot +0xc) ends in 0x22c6d0: 38 lightonoff (0x1de740), 58 switch levers, ability props, key pads and class lockers
// (0x175338), 100 energy switches (0x18166c), 125 (0x1ac47c), 138 fire (0x15a2f4 -> 0x184724), 139 switchfloor (0x19e268), 141 (0x260d3c), 185 axe doors,
// break-and-enter, electric repair and the other ability props (0x18dd6c -> 0x1d4040), 186 colour swappers (0x260068 -> 0x1b58f0), 222 freeze targets (0x1adcf8).
inline constexpr std::array<std::uint8_t, 10> kGenericStudStateKinds = {38, 58, 100, 125, 138, 139, 141, 185, 186, 222};
bool runsGenericStudStateMachine(std::uint8_t kind);

// 0x22c6d0 changes state when the halfword +4 (target) differs from +2: a target of 2 or 3 (0x22c754: cmp #2, cmpne #3) runs 0x22c8a0.
bool genericStateChangeDropsStuds(std::uint16_t targetState);

// 0x2451d0(entity, name, fallback): a float parameter (type 0) is cast with vcvt.s32.f32 (saturating, towards zero), the types 5 and 7 return the stored
// word, every other type and a missing parameter the fallback.
std::int32_t studValueParameter(const assets::LevelObject& object, const assets::Level& cls, std::int32_t fallback);

// 0x22c8a0: 0x22ca88(entity, forward 1, force 0, mode flag 0, 1): count `StudValue` (fallback 0), 0x2149ac(entity, SpawnLoc, count, 1, 0), so mode 1 and the
// SpawnLoc node when the object has one, else `node`. A count of 0 drops nothing; a negative count is passed on by the original and dropped here
// (STAND-IN STUD-S14).
std::optional<StudDropRequest> genericStateStudRequest(const StudSpawnSource& source, std::int32_t studValue, const std::array<float, 16>& node);

// The sites that call 0x23a6b0(entity, `StudValue` with fallback 25, forward, 0, 1): mode 1 like the generic machine.
struct StudRewardSite {
    std::uint8_t kind;
    std::uint32_t call;   // the bl 0x23a6b0
    bool forward;
};
inline constexpr std::array<StudRewardSite, 5> kStudRewardSites = {{
    {191, 0x186158, true},   // props/ability_safecrack*: state change to 3 of the slot +0xc function 0x186078
    {195, 0x1b062c, true},   // props/ability_wiretap: state 4, once (record byte +0x35 bit 1)
    {150, 0x2bd814, false},  // props/ability_jetpackpad*: the used flag (byte +0x131 bit 3) of the character record, 0x2bd750
    {66, 0x319fe0, false},   // projectiles: the target of kind 124 (0x319f3c), StudValue of the target object
    {183, 0x28d8e4, false},  // vehicles: the countdown +0x24c of 0x28d81c
}};
inline constexpr std::int32_t kStudRewardFallback = 25;
const StudRewardSite* studRewardSite(std::uint8_t kind);
std::optional<StudDropRequest> studRewardRequest(const StudRewardSite& site, const StudSpawnSource& source, std::int32_t studValue, const std::array<float, 16>& node);

// 0x20bd18 (called every update of the player, byte +0x12e bit 0 of the character record, cleared after the call): 0x21e2e4(mode 0, count 30, the node's
// translation, direction none, s0 = s1 = 1.0, stack 1, 0, 0, 30, 0, 0): the same request as the non-player death's 30 studs.
inline constexpr unsigned kPlayerHurtStudCount = 30;
StudDropRequest playerHurtStudRequest(const DebrisVec3& nodePosition);

// 0x26d1f0 (the player's death, called for the camera focus *0x3772b8): n = the stud counter 0x593418 divided by 10, at most 110, lowered while the digit sum
// of n is above 10 (by 1, by 10 when its last digit is 0); 0x21e2e4(mode 0, count n, the node's translation, no direction, stack 1, 0, 0, 30, 0, 0) with the byte
// 0x377278 set to 1 during the call, then the counter minus 10 n. A count of 0 still clears the byte. STAND-IN STUD-S15: the notification 0x22ddf4(0x519760, 4)
// that follows a non-zero loss is not ported.
inline constexpr unsigned kPlayerDeathStudLimit = 110, kPlayerDeathDigitSumLimit = 10, kPlayerDeathUnitValue = 10;
struct PlayerDeathStudLoss {
    unsigned count = 0;
    std::uint64_t counterAfter = 0;
    std::optional<StudDropRequest> request;
    bool noMultiplier = true;   // byte 0x377278 = 1 for the call (record +0x49 bit 0)
};
PlayerDeathStudLoss playerDeathStudLoss(std::uint64_t studCounter, const DebrisVec3& nodePosition);

// 0x2b7894(entity, record, finish): the studs of the hit-sequence slots. Slot i has the byte +0x16 + 0x10 i `StudSpawnValue`, +0x17 `StudSpawnFrame` (255 none),
// +0x18 the sound frame, the halfword +0x14 the sound; the counter +0xd9 selects per field the last slot in use at or below it.
struct HitSequenceSlot {
    std::uint16_t sound = 0;
    std::uint8_t value = 0;
    std::uint8_t frame = 255;
    std::uint8_t soundFrame = 255;
};
struct HitSequenceStudInput {
    std::array<HitSequenceSlot, 4> slots{};
    std::uint8_t counter = 0;       // +0xd9
    std::uint8_t health = 0;        // +0xd4, unsigned
    bool hasClip = true;            // 0x23ad24(entity + 0x40) is not null
    float clipFrame = 0.0f;         // 0x213140(clip, 0)
    float boxHalfX = 0.0f;          // entity +0x6c
    float boxHalfZ = 0.0f;          // entity +0x74
};
// The studs fire when the slot's frame lies in [frame, frame + 1) of the clip, the health is not 1 and the slot's value is not 0: 0x23a6b0(entity, value,
// forward = half x > 2 half z, 1, 1), so mode 0.
std::optional<StudDropRequest> hitSequenceStudRequest(const HitSequenceStudInput& input, const StudSpawnSource& source, const std::array<float, 16>& node);

}
