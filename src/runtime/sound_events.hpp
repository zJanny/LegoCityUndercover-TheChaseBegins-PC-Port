#pragma once
// Sound calls of the player and of the stud pickups: which effect id sounds when, and at which position. The player
// plays character sounds from a row of tablessoundfx sheet 3 (selected by the character's sound category); studs
// play the sound of their type table. Evidence and traces: docs/re/audio.md. No SDL dependency.
#include "assets/assets.hpp"
#include "runtime/studs.hpp"
#include <array>
#include <cstdint>
#include <vector>

namespace chase::runtime {

// A call of the play routine 0x23fa54(id, position, ...), made through 0x23ae84 (the position is the node position of
// the object) or directly with a position.
struct SoundCall {
    std::uint16_t id = 0;
    std::array<float, 3> position{};
    std::uint16_t owner = 0;  // the u16 at +0x14 of the playing object (0x23ae84), the key of the stop 0x23ae50
    bool onlyIfIdle = false;  // 0x2c0384 for a persistent sound: no new voice while (id, owner) still runs (0x21b5a8)
};

// A call of the stop 0x23ae50 / 0x235028 with the table fade (column 16).
struct SoundStop {
    std::uint16_t id = 0;
    std::uint16_t owner = 0;
};

// Slots of a character sound row (u16 at category * 0x44 + slot * 2 of the table loaded to 0x375f7c).
constexpr std::size_t kSlotFootstep = 5;         // event 50 of 0x19efbc: footstep pulse of the animation
constexpr std::size_t kSlotJump = 6;             // 0x200e70, the take-off state
constexpr std::size_t kSlotLandFromHeight = 7;   // 0x1abc5c: fall distance above 1.0
constexpr std::size_t kSlotLandWater = 9;        // 0x1abc5c: ctrl+0x317 bit 3
constexpr std::size_t kSlotLandOnObject11 = 13;  // 0x1abc5c: ground object of type 11
constexpr std::size_t kSlotLandHard = 8;         // 0x25f460: after the literal sound below
constexpr std::uint16_t kHardLandingSound = 285; // literal at 0x25f570: movement/characters/big_fall

// The pulse track of the locomotion animations (walk, run, sprint): name hash of the footstep event track.
constexpr std::uint32_t kFootstepTrackHash = 0x68cc6917;
// Event 50 is posted when the weighted track value rose by more than this since the last update (0x19f128 compares with 0x3f000000).
constexpr float kFootstepRise = 0.5f;
// Fall distance (ctrl+0x2b4, the accumulated fall speed) above which the landing plays the slot 7 sound first (0x1abcd8).
constexpr float kLandingImpactDistance = 1.0f;

// Value of the footstep track at the normalised animation time (0 when the animation has no such track). The track is
// sampled as a step function (the value of the last key at or before the time): the Bezier evaluation of the other
// tracks reproduces none of the recorded rises, the step sampling reproduces all walk footsteps (docs/re/audio.md).
float footstepTrackValue(const assets::Animation& animation, float normalisedTime);

// Rising-edge detector of the footstep track of one animation slot.
class FootstepDetector {
public:
    // One update with the track value and the weight of the slot: true when 0x19f10c..0x19f128 posts event 50.
    bool update(float value, float weight = 1.0f);
    // A new animation slot starts: the previous sample is the current one, so the first update cannot fire.
    void restart(float value, float weight = 1.0f);

private:
    float previousValue_ = 0, previousWeight_ = 1;
};

class CharacterSounds {
public:
    CharacterSounds() = default;
    explicit CharacterSounds(const std::array<std::uint16_t, assets::kCharacterSoundSlots>& slots) : slots_(slots) {}
    std::uint16_t id(std::size_t slot) const { return slots_[slot]; }

    SoundCall footstep(const std::array<float, 3>& position) const { return {id(kSlotFootstep), position}; }
    SoundCall jump(const std::array<float, 3>& position) const { return {id(kSlotJump), position}; }
    // 0x25f460, the landing after a fall of kHardLandingDistance or more: big_fall, then the slot 8 sound.
    std::vector<SoundCall> hardLanding(const std::array<float, 3>& position) const {
        return {{kHardLandingSound, position}, {id(kSlotLandHard), position}};
    }
    // 0x1abc5c: the impact sound when the fall was longer than kLandingImpactDistance, then the surface sound
    // (water flag, object type 11 or the default).
    std::vector<SoundCall> landing(float fallDistance, bool water, bool objectType11, const std::array<float, 3>& position) const;

private:
    std::array<std::uint16_t, assets::kCharacterSoundSlots> slots_{};
};

// 0x2277c4: the collect sound of the stud's type at its centre.
SoundCall studCollectCall(const StudCollected& collected, const std::array<float, 3>& centre);

} // namespace chase::runtime
