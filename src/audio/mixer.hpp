#pragma once
// Software mixer for decoded sounds (plan step S4d). No SDL dependency: the
// platform layer pulls interleaved stereo frames from mix().
#include "assets/assets.hpp"
#include <cstddef>
#include <memory>
#include <vector>

namespace chase::audio {
class Mixer {
public:
    explicit Mixer(int outputRate);
    // Starts a voice; returns its id. The sound must outlive the voice.
    int play(std::shared_ptr<const assets::Sound> sound, float gain = 1.0f);
    int play(std::shared_ptr<const assets::Sound> sound, float leftGain, float rightGain);
    // Changes the gains of a running voice; false when the voice has ended.
    bool setGains(int voice, float leftGain, float rightGain);
    // Script voice controls: the source frames per second of the voice and a volume factor on its gains; false when the voice has ended.
    bool setRate(int voice, int sourceRate);
    bool setVolume(int voice, float volume);
    void stop(int voice);
    // Ramps the voice to silence over `seconds` of output and then ends it; a non-positive time stops it at once.
    void fadeOut(int voice, float seconds);
    bool playing(int voice) const;
    std::size_t activeVoices() const;
    // Mixes `frames` stereo frames into `out` (2 * frames floats, overwritten, range about -1..1).
    void mix(float* out, std::size_t frames);
    int outputRate() const { return rate_; }

private:
    struct Voice {
        int id;
        std::shared_ptr<const assets::Sound> sound;
        double position;   // in source frames
        double step;       // source frames per output frame
        float left;
        float right;
        double fade = 1.0;
        float volume = 1.0f;
        double fadeStep = 0.0;
        bool ended = false;
    };
    int rate_;
    int nextId_ = 1;
    std::vector<Voice> voices_;
};
} // namespace chase::audio
