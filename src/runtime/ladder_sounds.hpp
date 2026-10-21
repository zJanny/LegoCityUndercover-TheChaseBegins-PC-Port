#pragma once
// Sound events carried by animation clips: the ladder climbs (ladder_up / ladder_down, ladder_dismount) play theirs this way. A clip's
// sub-animations are listed with an aspect id; the aspect 0x6c22818f holds a sound pulse track (hash 0x0ffe43cf, handler 0x2c0384 called
// from 0x287ca4) with a sound record, the aspect 0x3a01a706 a character-sound track (hash 0x13e3511c, branch 0x19f168 of 0x19efbc) with a
// slot record. Evidence: docs/re/player-traversal-ladders.md. No SDL dependency.
#include "assets/assets.hpp"
#include "runtime/game_random.hpp"
#include "runtime/sound_events.hpp"
#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace chase::runtime {

constexpr std::uint32_t kAspectSoundPulse = 0x6c22818f;
constexpr std::uint32_t kAspectCharacterSound = 0x3a01a706;
constexpr std::uint32_t kSoundPulseTrackHash = 0x0ffe43cf;
constexpr std::uint32_t kCharacterSoundTrackHash = 0x13e3511c;
// 0x2c0384 compares the weighted rise with 0x34000001 as an integer (a float above 2^-23); 0x19f128 compares with 0x3f000000.
constexpr float kSoundPulseEdge = 1.1920929e-7f;
constexpr float kCharacterSoundEdge = 0.5f;
// TRAV-S36: the original keys the voice by an instance word of the animation slot (+0x114 of 0x2c0384's slot); the port uses one owner per sub-animation.
constexpr std::uint16_t kClipSoundOwnerStandIn = 0x7000;
// TRAV-S37: the chance draws of the clip sounds come from the rig's own generator, not the game's shared stream 0x25b2b8.
constexpr std::uint32_t kClipSoundSeedStandIn = 1;
constexpr std::uint8_t kSoundFlagLoop = 1;  // bit 0 of record byte +6 (tablessoundfx column 3), tested by 0x219244

// The 20-byte data block of a sound pulse sub-animation (read at [slot + 8] by 0x2c0384): sound id, volume, pitch, chance (below 1.0 a
// draw of 0x2345dc must stay under it) and a flag byte (0: the volume and pitch factors are set on the new voice, 0x219178 / 0x22f770).
struct SoundPulseRecord {
    std::uint32_t id = 0;
    float volume = 1.0f, pitch = 1.0f, chance = 1.0f;
    std::uint8_t flag = 0;
};
std::optional<SoundPulseRecord> soundPulseRecord(const assets::Bytes& data);

// The 16-byte data block of a character-sound sub-animation (0x19f1d0): character sound slot, chance, volume factor and a flag byte.
struct CharacterSoundRecord {
    std::uint32_t slot = 0;
    float chance = 1.0f, volume = 1.0f;
    std::uint8_t flag = 0;
};
std::optional<CharacterSoundRecord> characterSoundRecord(const assets::Bytes& data);

// Value of the first track `hash` of a sub-animation at the normalised time, sampled as a step function (see footstepTrackValue).
float stepTrackValue(const std::vector<assets::AnimTrack>& tracks, std::uint32_t hash, float normalisedTime);

// The sound events of the clip a character plays, one detector per sub-animation. `looping[id]` is the loop flag of the sound id.
class ClipSounds {
public:
    ClipSounds() = default;
    explicit ClipSounds(std::vector<bool> looping) : looping_(std::move(looping)) {}

    // A new clip starts: the previous samples are the current ones, so the first update cannot fire (AUDIO-S3).
    void restart(const assets::Animation& animation, float normalisedTime);
    // One update: appends the plays and stops of the rising and falling edges of this update.
    void update(const assets::Animation& animation, float normalisedTime, const std::array<float, 3>& position,
                const CharacterSounds& character, GameRandom& random, std::vector<SoundCall>& plays, std::vector<SoundStop>& stops);

private:
    bool looping(std::uint32_t id) const { return id < looping_.size() && looping_[id]; }
    void sample(const assets::Animation& animation, float normalisedTime);

    std::vector<bool> looping_;
    std::vector<float> previous_;
};

}
