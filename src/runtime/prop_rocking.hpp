#pragma once
// The rocking of the ObjectType 67 props (docs/re/props-rocking.md): the create 0x16a268 reads `Rocking` into bit 6 of record +0xde and the
// periods `WaveBob1`, `WaveBob2`, `WaveBobOffset`; the post-link 0x16aeb4 keeps the node's Euler angles (0x247660) at record +0x5c; each
// entity update 0x16b06c runs 0x28a758 while the bit is set, and the script native SetPropRocking 0x1da6a8 sets or clears it. No SDL dependency.
#include "assets/assets.hpp"
#include "assets/object_classes.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr std::uint8_t kRockingPropKind = 67;
constexpr std::uint32_t kRockingRate = 30;                 // 0x23ca4c: 30 without a rate object
constexpr std::int32_t kWaveBob1Default = 2;               // 0x16a864
constexpr std::int32_t kWaveBob2Default = 3;               // 0x16a888
constexpr std::int32_t kWaveBobOffsetDefault = 0;          // 0x16a8b0
constexpr float kRockingPitch = 0.03490658849477768f;      // literal 0x3d0efa36 at 0x28a874 (2 degrees)
constexpr float kRockingRoll = 0.04363323375582695f;       // literal 0x3d32b8c3 at 0x28a878 (2.5 degrees)
// STAND-IN SCRIPT-S297: the clock 0x241098 the rocking reads advances 60 units per second here (2 per update at 30 updates per second, as
// measured for the use markers, SCRIPT-S247); its multiplier and its value at the level start are not read.
constexpr std::uint32_t kRockingClockUnitsPerUpdateStandIn = 2;

struct PropRocking {
    std::string instance;                  // lower case
    bool rocking = false;                  // record +0xde bit 6
    std::uint32_t period1 = 0;             // record +0xb4: WaveBob1 * 0x23ca4c
    std::uint32_t period2 = 0;             // record +0xb8: WaveBob2 * 0x23ca4c
    std::uint32_t phase = 0;               // record +0xbc: WaveBobOffset
    std::array<float, 3> baseAngles{};     // record +0x5c: 0x247660 of the node at the post-link
    std::array<float, 16> placement{};     // the node matrix at the level start
};

// The record fields of 0x16a268 for a kind 67 object whose `Rocking` parameter has bit 0 set; nothing for the others. STAND-IN SCRIPT-S297:
// the create keeps a record (and reads the WaveBob fields) for more kind 67 objects than these (rule 0x16a334..0x16a3a0, not read), so a
// SetPropRocking(x, 1) on a kind 67 prop with `Rocking` 0 does nothing here.
bool propRockingOf(const assets::LevelObject& object, const assets::Level& cls, PropRocking& out);
// 0x28a758: the node's 3x3 rotation rows, Rx..z(roll, 0, pitch) (0x237c48) times the rotation of the base angles (0x25898c), with
// pitch = 2 degrees * sin(2 pi ((clock + phase) mod period1) / period1) and roll = 2.5 degrees * cos(2 pi ((clock + phase) mod period2) / period2).
std::array<float, 9> rockingRotation(const PropRocking& prop, std::uint32_t clock);
// The node matrix: the rotation rows of rockingRotation, the translation of the placement (0x232788 copies three rows only).
std::array<float, 16> rockingMatrix(const PropRocking& prop, std::uint32_t clock);

class PropRockingSet {
public:
    void load(const assets::Level& level, const assets::ClassLookup& classOf);
    // SetPropRocking 0x1da6a8: int(value) == 1 sets bit 6, any other value clears it; an entity without a record is left alone. False when the
    // instance has no rocking record here.
    bool setRocking(const std::string& lowerInstance, float value);
    const PropRocking* find(const std::string& lowerInstance) const;
    const std::vector<PropRocking>& props() const { return props_; }

private:
    std::vector<PropRocking> props_;
};

}
