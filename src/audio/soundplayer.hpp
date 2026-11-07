#pragma once
// Plays sound effects by id, like the play routine 0x23fa54 and the voice set-up 0x23f82c / 0x23f940: volume from the
// table row, positional sounds attenuated by distance and panned by the listener (src/audio/spatial.*). Evidence:
// docs/re/audio.md. No SDL dependency: the owner pulls samples from the mixer and guards both with one lock.
#include "assets/assets.hpp"
#include "audio/effectvoices.hpp"
#include "audio/mixer.hpp"
#include "audio/spatial.hpp"
#include "audio/voice_params.hpp"
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

    // 0x1dca5c: the master volume reaches every running voice through its dirty bit.
    void setMasterVolume(float volume);
    // AUDIO-S34: the byte 0x376ce6 that enables the rate boost of sounds with record bit 3; the only code that writes it
    // clears it and no sound effect row has the bit, so it is off.
    void setRateBoostByte(bool on) { rateBoostByteStandIn_ = on; }
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
    // 0x2247f0: a voice that `id` started for `owner` still runs.
    bool playing(std::uint32_t id, std::uint16_t owner) { return voices_.find(id, owner) >= 0; }

    // SetSoundVolume 0x219178 -> 0x227ff4 and ScaleSoundFrequency 0x22f770 -> 0x227f4c on the voice that `id` started for
    // `owner`. The volume replaces the voice's base volume (table percent / 100); the distance factor and the master volume
    // multiply it (0x107fa0, 0x25c388). The rate is (int)(scale * source rate), stored as the voice rate; the pitch is
    // rate * Doppler factor / source rate (docs/re/audio.md).
    bool setVolume(std::uint32_t id, std::uint16_t owner, float volume);
    bool scaleFrequency(std::uint32_t id, std::uint16_t owner, float scale);

    // 0x1cef9c(1) / 0x1cef9c(0) of BulletTime: every running voice is held, or continues.
    void pauseVoices() { mixer_.pauseAll(); }
    void resumeVoices() { mixer_.resumeAll(); }

    // Recomputes the gains of the running positional voices from the current listener (0x107fa0 runs every frame).
    void update();

    const std::vector<assets::SoundEffect>& effects() const { return effects_; }
    const assets::SoundTables& tables() const { return tables_; }

private:
    struct Live {
        int voice;
        EmitterShape shape;
        bool positional;
        std::array<float, 3> position;
        std::uint32_t sourceRate;
        std::int32_t rate;
        bool boost;
    };
    std::shared_ptr<const assets::Sound> sound(const assets::SoundEffect& effect);
    Live* live(int voice);
    StereoGains gainsOf(const Live& voice) const;
    float dopplerOf(const Live& voice) const;
    void applyGains(const Live& voice);
    void applyPitch(const Live& voice, float doppler);
    void prune();

    assets::FuseArchive& archive_;
    Mixer& mixer_;
    EffectVoices voices_;
    std::vector<assets::SoundEffect> effects_;
    assets::SoundTables tables_;
    std::map<std::uint32_t, std::shared_ptr<const assets::Sound>> decoded_;
    std::vector<Live> live_;
    Listener listener_;
    float master_ = 1.0f;
    bool rateBoostByteStandIn_ = false;
};

} // namespace chase::audio
