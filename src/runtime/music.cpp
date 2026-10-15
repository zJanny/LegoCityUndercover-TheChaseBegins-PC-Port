#include "runtime/music.hpp"

namespace chase::runtime {

MoodChoice chooseMoodTrack(const assets::MusicSet& set, unsigned mood, unsigned currentTrack) {
    const std::uint8_t entries[2] = {set.ambient, set.action};
    MoodChoice choice;
    choice.mood = mood & 1;
    if (entries[choice.mood] == kNoMusic) {
        choice.stop = true;
        return choice;
    }
    if (entries[choice.mood] == 0) choice.mood ^= 1;
    const unsigned track = entries[choice.mood];
    if (track != currentTrack) choice.track = track;
    return choice;
}

std::optional<assets::MusicSet> musicSetOfLevel(const assets::SoundTables& sounds, std::uint8_t set) {
    if (set >= sounds.musicSets.size()) return std::nullopt;
    return sounds.musicSets[set];
}

LevelMusic levelStartMusic(const assets::SoundTables& sounds, std::uint8_t set, unsigned currentTrack) {
    LevelMusic music;
    music.set = set;
    if (set >= sounds.musicSets.size()) return music;
    music.choice = chooseMoodTrack(sounds.musicSets[set], kLevelStartMood, currentTrack);
    if (music.choice.track && *music.choice.track < sounds.musicNames.size()) music.trackName = sounds.musicNames[*music.choice.track];
    return music;
}

}
