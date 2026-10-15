#pragma once
// Plays sound effects by id, like the play routine 0x23fa54 and the voice set-up 0x23f82c / 0x23f940: volume from the
// table row, positional sounds attenuated by distance and panned by the listener (src/audio/spatial.*). Evidence:
// docs/re/audio.md. No SDL dependency: the owner pulls samples from the mixer and guards both with one lock.
#include "assets/assets.hpp"
#include "audio/effectvoices.hpp"
#include "audio/mixer.hpp"
#include "audio/spatial.hpp"
#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <vector>

namespace chase::audio {

// The master volume of sound effects: the option byte / 10 (0x2c19b8 and 0x2c19cc call 0x1dca5c with the default
// option 8 and then option / 10.0, stored at 0x37636c + 4).
constexpr float volumeFromOption(std::uint8_t option) { return float(option) / 10.0f; }

class SoundPlayer {
public:
    SoundPlayer(assets::FuseArchive& archive, Mixer& mixer);

    void setMasterVolume(float volume) { master_ = volume; }
    float masterVolume() const { return master_; }
    void setListener(const Listener& listener) { listener_ = listener; }
    const Listener& listener() const { return listener_; }

    // 0x23fa54(id, position, ...): returns the mixer voice, or -1 when the id has no sound (id 0, unnamed or unknown)
    // or the sound is silent at its distance. `position` null plays it flat (0x23f82c).
    // `owner` is the u16 at +0x14 of the object that plays the sound (0x23ae84), the key of stop().
    int play(std::uint32_t id, const std::array<float, 3>* position, std::uint16_t owner = 0);

    // 0x23ae50(id, object, -1.0): the voice that `id` started for `owner` ends, with the fade time of the sound's table row
    // (column 16; 0 stops it at once). False when none runs.
    // A fade time of 0 or more replaces the column 16 time (0x23f75c: only a negative argument takes the table's).
    bool stop(std::uint32_t id, std::uint16_t owner, float fadeSeconds = -1.0f);

    // SetSoundVolume 0x219178 and ScaleSoundFrequency 0x22f770 on the voice that `id` started for `owner`.
    // AUDIO-S31: the volume is a factor on the gains and the rate is (int)(scale * source rate); the original's
    // consumers of the voice fields +0x50 and +0x58 are only partly read.
    bool setVolume(std::uint32_t id, std::uint16_t owner, float volume);
    bool scaleFrequency(std::uint32_t id, std::uint16_t owner, float scale);

    // Recomputes the gains of the running positional voices from the current listener (0x107fa0 runs every frame).
    void update();

    const std::vector<assets::SoundEffect>& effects() const { return effects_; }
    const assets::SoundTables& tables() const { return tables_; }

private:
    struct Positional {
        int voice;
        EmitterShape shape;
        std::array<float, 3> position;
    };
    std::shared_ptr<const assets::Sound> sound(const assets::SoundEffect& effect);

    assets::FuseArchive& archive_;
    Mixer& mixer_;
    EffectVoices voices_;
    std::vector<assets::SoundEffect> effects_;
    assets::SoundTables tables_;
    std::map<std::uint32_t, std::shared_ptr<const assets::Sound>> decoded_;
    std::vector<Positional> positional_;
    Listener listener_;
    float master_ = 1.0f;
};

} // namespace chase::audio
