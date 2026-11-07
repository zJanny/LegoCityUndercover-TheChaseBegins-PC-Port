#include "audio/mixer.hpp"
#include <algorithm>
#include <stdexcept>

namespace chase::audio {
Mixer::Mixer(int outputRate) : rate_(outputRate) {
    if (outputRate <= 0) throw std::runtime_error("invalid mixer rate");
}

int Mixer::play(std::shared_ptr<const assets::Sound> sound, float gain) { return play(std::move(sound), gain, gain); }

bool Mixer::setGains(int voice, float leftGain, float rightGain) {
    for (Voice& v : voices_)
        if (v.id == voice) {
            v.left = leftGain;
            v.right = rightGain;
            return true;
        }
    return false;
}

bool Mixer::setPitch(int voice, float pitch) {
    for (Voice& v : voices_)
        if (v.id == voice) {
            v.step = double(pitch) * double(v.sound->rate) / rate_;
            return true;
        }
    return false;
}

int Mixer::play(std::shared_ptr<const assets::Sound> sound, float leftGain, float rightGain) {
    if (!sound || sound->channels.empty() || sound->rate <= 0 || !sound->frames)
        throw std::runtime_error("cannot play an empty sound");
    if (sound->loop && sound->loopStart >= sound->frames) throw std::runtime_error("loop start beyond sound end");
    int id = nextId_++;
    voices_.push_back({id, std::move(sound), 0.0, 0.0, leftGain, rightGain});
    voices_.back().step = double(voices_.back().sound->rate) / rate_;
    return id;
}

void Mixer::stop(int voice) {
    voices_.erase(std::remove_if(voices_.begin(), voices_.end(), [&](const Voice& v) { return v.id == voice; }),
                  voices_.end());
}

void Mixer::fadeOut(int voice, float seconds) {
    if (seconds <= 0.0f) {
        stop(voice);
        return;
    }
    for (Voice& v : voices_)
        if (v.id == voice) v.fadeStep = 1.0 / (double(seconds) * rate_);
}

bool Mixer::playing(int voice) const {
    return std::any_of(voices_.begin(), voices_.end(), [&](const Voice& v) { return v.id == voice; });
}

void Mixer::pauseAll() {
    for (Voice& v : voices_) v.paused = true;
}

void Mixer::resumeAll() {
    for (Voice& v : voices_) v.paused = false;
}

bool Mixer::paused(int voice) const {
    return std::any_of(voices_.begin(), voices_.end(), [&](const Voice& v) { return v.id == voice && v.paused; });
}

std::size_t Mixer::activeVoices() const { return voices_.size(); }

void Mixer::mix(float* out, std::size_t frames) {
    std::fill(out, out + 2 * frames, 0.0f);
    for (auto& voice : voices_) {
        if (voice.paused) continue;
        const auto& sound = *voice.sound;
        double end = sound.frames;
        for (std::size_t f = 0; f < frames; ++f) {
            if (voice.ended) break;
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
                out[2 * f + std::size_t(side)] += float(s / 32768.0) * (side == 0 ? voice.left : voice.right) * float(voice.fade);
            }
            voice.position += voice.step;
            if (voice.fadeStep > 0.0) {
                voice.fade -= voice.fadeStep;
                if (voice.fade <= 0.0) voice.ended = true;
            }
        }
    }
    voices_.erase(std::remove_if(voices_.begin(), voices_.end(),
                                 [](const Voice& v) { return v.ended || (!v.sound->loop && v.position >= v.sound->frames); }),
                  voices_.end());
}
} // namespace chase::audio
