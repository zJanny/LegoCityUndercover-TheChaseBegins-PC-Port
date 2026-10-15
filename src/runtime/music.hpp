#pragma once
// Which music track a level plays: the mood rule of 0x206c40 over the music set of the level record (set at level
// load by 0x2f6c24 / 0x2e6524 from record byte +0x39 and the 14 sets of tablessoundfx sheet 2). Evidence:
// docs/re/audio.md. No SDL dependency.
#include "assets/assets.hpp"
#include <optional>

namespace chase::runtime {

constexpr unsigned kMoodAmbient = 0, kMoodAction = 1;
constexpr std::uint8_t kNoMusic = 255;   // set entry: stop the music for this mood
constexpr unsigned kLevelStartMood = kMoodAmbient;   // 0x2f71b0 calls 0x206c40 with mood 0
constexpr float kLevelStartMusicFade = 1.0f;         // s0 of that call: literal 0x3f800000 at 0x2f705c, loop 1, crossfade 1

// The set record the level start hands to the block (+0x24 of 0x376b30); none when the set index is 14 or more (0x2f7160).
std::optional<assets::MusicSet> musicSetOfLevel(const assets::SoundTables& sounds, std::uint8_t set);

struct MoodChoice {
    bool stop = false;                   // entry 255: the current stream is stopped
    std::optional<unsigned> track;       // track row to start; empty when the current track already plays it
    unsigned mood = 0;                   // the mood after the 0 -> other mood swap (DAT_00376b33)
};

// 0x206c40: entry 255 stops the music; entry 0 swaps to the other mood; the entry is the track row of
// tablessoundfx sheet 1, started unless it equals `currentTrack` (0x586220, 0 when no stream plays).
MoodChoice chooseMoodTrack(const assets::MusicSet& set, unsigned mood, unsigned currentTrack);

// The music set of a level record, the track a level start (mood 0) asks for, and its name ("" when the row is
// empty: the game then opens "Music/.wav" and plays nothing).
struct LevelMusic {
    unsigned set = 0;
    MoodChoice choice;
    std::string trackName;
};
LevelMusic levelStartMusic(const assets::SoundTables& sounds, std::uint8_t set, unsigned currentTrack);

}
