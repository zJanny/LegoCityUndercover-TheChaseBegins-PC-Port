#include "audio/mixer.hpp"
#include <algorithm>
#include <stdexcept>

namespace chase::audio {
Mixer::Mixer(int outputRate) : rate_(outputRate) {
    if (outputRate <= 0) throw std::runtime_error("invalid mixer rate");
}

int Mixer::play(std::shared_ptr<const assets::Sound> sound, float gain) {
    if (!sound || sound->channels.empty() || sound->rate <= 0 || !sound->frames)
        throw std::runtime_error("cannot play an empty sound");
    if (sound->loop && sound->loopStart >= sound->frames) throw std::runtime_error("loop start beyond sound end");
    int id = nextId_++;
    voices_.push_back({id, std::move(sound), 0.0, 0.0, gain});
    voices_.back().step = double(voices_.back().sound->rate) / rate_;
    return id;
}

void Mixer::stop(int voice) {
    voices_.erase(std::remove_if(voices_.begin(), voices_.end(), [&](const Voice& v) { return v.id == voice; }),
                  voices_.end());
}

bool Mixer::playing(int voice) const {
    return std::any_of(voices_.begin(), voices_.end(), [&](const Voice& v) { return v.id == voice; });
}

std::size_t Mixer::activeVoices() const { return voices_.size(); }

void Mixer::mix(float* out, std::size_t frames) {
    std::fill(out, out + 2 * frames, 0.0f);
    for (auto& voice : voices_) {
        const auto& sound = *voice.sound;
        double end = sound.frames;
        for (std::size_t f = 0; f < frames; ++f) {
            if (voice.position >= end) {
                if (!sound.loop) break;
                voice.position = sound.loopStart + (voice.position - end);
            }
            auto at = std::size_t(voice.position);
            double frac = voice.position - double(at);
            // Next sample for interpolation wraps to the loop start (or holds at the end).
            std::size_t next = at + 1 < sound.frames ? at + 1 : (sound.loop ? sound.loopStart : at);
            for (int side = 0; side < 2; ++side) {
                const auto& channel = sound.channels[std::min<std::size_t>(std::size_t(side), sound.channels.size() - 1)];
                double s = channel[at] + (channel[next] - channel[at]) * frac;
                out[2 * f + std::size_t(side)] += float(s / 32768.0) * voice.gain;
            }
            voice.position += voice.step;
        }
    }
    voices_.erase(std::remove_if(voices_.begin(), voices_.end(),
                                 [](const Voice& v) { return !v.sound->loop && v.position >= v.sound->frames; }),
                  voices_.end());
}
} // namespace chase::audio
