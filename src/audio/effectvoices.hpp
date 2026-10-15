#pragma once
// The voices of running sound effects by (sound id, owner id), like the instance array of a sound bank searched by 0x23fcac:
// the stop 0x23ae50 -> 0x23f75c finds the voice that sound `id` started for the object whose u16 at +0x14 is `owner`.
// Evidence: docs/re/audio.md. No SDL dependency.
#include "audio/mixer.hpp"
#include <cstdint>
#include <vector>

namespace chase::audio {

class EffectVoices {
public:
    explicit EffectVoices(Mixer& mixer) : mixer_(mixer) {}

    void add(int voice, std::uint32_t id, std::uint16_t owner);

    // 0x23f75c: the oldest running voice of (id, owner) ends at once when `fadeSeconds` is 0 (rec +0xc of the sound
    // record, tablessoundfx column 16) and fades out over that time otherwise. False when no such voice runs.
    bool stop(std::uint32_t id, std::uint16_t owner, float fadeSeconds);

    // The oldest running voice of (id, owner), the instance search 0x23fcac; -1 when none.
    int find(std::uint32_t id, std::uint16_t owner);

private:
    struct Entry {
        int voice;
        std::uint32_t id;
        std::uint16_t owner;
    };
    void prune();

    Mixer& mixer_;
    std::vector<Entry> entries_;
};

} // namespace chase::audio
