#pragma once
// Streams one music track at a time by its row in tablessoundfx sheet 1 (0x240ec8 / 0x24039c). Evidence:
// docs/re/audio.md. No SDL dependency: the owner pulls samples from the mixer and guards both with one lock.
#include "assets/assets.hpp"
#include "audio/mixer.hpp"
#include <string>
#include <vector>

namespace chase::audio {

// Archive path of a music track name ("music/<name>.bwav", the game opens "Music/<name>.wav", 0x24039c).
std::string musicPath(const std::string& name);

class MusicPlayer {
public:
    MusicPlayer(assets::FuseArchive& archive, Mixer& mixer, std::vector<std::string> trackNames);

    // The music master volume: the option byte / 10, applied as `volume * master` (0x376b3c, 0x1f36ec).
    void setMasterVolume(float volume);
    // Starts the track (stopping the current one first); false for an unknown or unnamed track (the game opens
    // "Music/.wav" and plays nothing). `volume` is the fade target `s0` of 0x24039c.
    bool start(unsigned track, float volume = 1.0f);
    void stop();
    // The stream volume `s0` of 0x220f6c; the music master volume is applied on top.
    void setVolume(float volume);
    // The stream ran to its end (state 4 of 0x220fe0, read by the updater 0x240c3c).
    bool finished() const { return voice_ >= 0 && !mixer_.playing(voice_); }
    // The track id of the playing stream, 0 when none (0x586220).
    unsigned currentTrack() const { return current_; }
    bool playing() const { return voice_ >= 0 && mixer_.playing(voice_); }

private:
    assets::FuseArchive& archive_;
    Mixer& mixer_;
    std::vector<std::string> names_;
    float master_ = 1.0f;
    float volume_ = 1.0f;
    unsigned current_ = 0;
    int voice_ = -1;
};

} // namespace chase::audio
