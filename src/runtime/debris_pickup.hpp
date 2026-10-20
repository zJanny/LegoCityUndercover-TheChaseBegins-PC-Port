#pragma once
// The pickup of super debris bricks (docs/re/super-bricks.md, entry 2026-10-20 "The pickup of super debris bricks"): the branches of the record
// update 0x180168 that a brick record (type 6) reaches once 0x2c4f9c marked it pickable (+0x22 bit 2): the pickup test of states 1 and 2, the
// magnet state 3, the collect 0x2277c4 with the hook 0x2c4f78 (the HUD display 0x2144f8), the collected state 4 flying to the HUD sink 2, the
// counter add 0x28a128 and its hook 0x29c090 (the super brick count 0x593430, 0x262778, UI sound 14 or 2) and the object placement 0x2a2cc0 of
// states 3 and 4. The motion of states 1 and 2 is runtime/debris.hpp. No SDL dependency.
#include "runtime/debris.hpp"

#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

namespace chase::runtime {

// Record +0x22: bit 1 a super brick (0x2c8278), bit 2 pickable (set by 0x2c4f9c in the states 1 and 3, 0x2c504c).
inline constexpr std::uint8_t kDebrisFlagSuper = 2, kDebrisFlagPickable = 4;
// Record +0x49 bit 0: no stud multiplier (0x28a1d4); bit 2: collected at the next test (0x180458).
inline constexpr std::uint8_t kDebrisFlagNoMultiplier = 1, kDebrisFlagCollectAtOnce = 4;
// 0x1809ec: the magnet collects within *0x3772b0 x 0.25 of its target.
inline constexpr float kDebrisMagnetReachFactor = 0.25f;
// 0x29d9f8..0x29da30: while the magnet is on, row 6 +0x6c (the magnet radius) is FLT_MAX (literal 0x29daf8) and +0x70 (its life) 1.0;
// +0x68 (the step per update) comes from *0x3746ec, read 0.1 in the oracle (debris.md §2026-10-18T14:46:04).
inline constexpr float kDebrisMagnetRadius = std::numeric_limits<float>::max();
inline constexpr float kDebrisMagnetLife = 1.0f;
inline constexpr float kDebrisMagnetStepMeasured = 0.1f;
// Type row 6 +0x60 (1.0, 0x2ed3e8) is the collect timer; +0x8a the collect sound, 1 as read in the oracle (0 in the data image).
inline constexpr float kDebrisCollectTime = 1.0f;
inline constexpr std::uint16_t kDebrisCollectSoundMeasured = 1;
// 0x2277c4: the collect sound gets the volume 0.7 (literal 0x227abc).
inline constexpr float kDebrisCollectVolume = 0.7f;
// 0x28a128: a type 6 record is not one of the stud types 0..4; the switch's default branch adds 10 to the stud counter (0x28a188, 0x28a1c0).
inline constexpr std::uint64_t kDebrisStudUnits = 10;
// 0x180b0c..0x180b28: the HUD sink of type 6 is +0x5c = 3 (no flight), a pickable record flies to sink 2: the super brick display anchor
// (186, 21) at the depth 5.5 (sink table 0x58ef4c row 2, set at 0x2cee0c).
inline constexpr float kSuperDebrisSinkX = 186.0f, kSuperDebrisSinkY = 21.0f, kSuperDebrisSinkDepth = 5.5f;
struct DebrisPickupInput {
    float frameDelta = 0;              // 0x23602c
    DebrisVec3 pickupPoint{};          // the player's node matrix applied to entity +0x60 (studPickupPoint)
    bool blocked = false;              // the player's entity word +0xc bit 4
    bool magnet = false;               // type row 6 +0x64 = 0 (0x1f87b4): on while the player drives (0x24255c) or with the extra bit 8
    std::optional<DebrisVec3> magnetTarget;  // the player's "chest" joint (0x24f444, 0x227788), else its node translation; none: the target has no node (state 3 waits)
    std::optional<DebrisVec3> flightTarget;  // sink 2 through 0x24c1d0 with the camera 0x22e1f4(0): unprojectTopScreen(186, 21, 5.5)
    std::uint32_t tint = 0xffffffffu;  // *0x3743f8 (bytes r, g, b, a)
};

struct DebrisPickupSound {
    std::uint16_t id = 0;
    DebrisVec3 position{};
    float volume = 1.0f;
};

// The hook 0x29c090 for a type 6 record: the count after the add (0x262778) and the UI sound index of 0x235684.
struct SuperDebrisBanked {
    unsigned record = 0;  // index into DebrisField::records()
    std::int8_t value = 0;
    std::uint32_t total = 0;
    std::uint16_t sound = 0;
};

// The node of the record's object (DebrisField::objectOf) as 0x2a2cc0 leaves it for a record in state 3 or 4.
struct SuperDebrisPlacement {
    unsigned record = 0;
    DebrisVec3 position{};
    float scale = 1.0f;
    std::uint32_t colour = 0xffffffffu;  // r, g, b with alpha 0xff
    std::uint8_t alpha = 255;
};

struct DebrisPickupStep {
    std::vector<DebrisPickupSound> sounds;       // 0x2277c4: the type's +0x8a at the record
    unsigned shown = 0;                          // 0x2144f8 calls (0x2c4f78 for a super record)
    std::vector<SuperDebrisBanked> banked;
    std::uint64_t studUnits = 0;                 // 0x28a128 for each collected type 6 record
    std::vector<unsigned> freedRecords;          // records whose object 0x28a128 / 0x180894 disable (DebrisField::freeRecord); their state is 0
    std::vector<SuperDebrisPlacement> placements;
};

// One game update over the brick records, run before the records' motion (0x180168 tests a record before it moves it). `count` is the u32 at
// 0x593430. A record that this update moved to state 3 or 4 also loses the frame time from its life, as 0x18083c does for it.
DebrisPickupStep updateDebrisPickup(DebrisRecord* records, unsigned recordCount, const DebrisPickupInput& input, std::uint32_t& count);

}
