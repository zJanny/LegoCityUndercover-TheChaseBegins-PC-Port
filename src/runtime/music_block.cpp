#include "runtime/music_block.hpp"
#include "runtime/music.hpp"
#include <cstring>

namespace chase::runtime {
namespace {

constexpr std::uint32_t kOneBits = 0x3f800000u;

float clampVolume(float value) {
    std::uint32_t bits;
    std::memcpy(&bits, &value, sizeof bits);
    if (static_cast<std::int32_t>(bits) >= static_cast<std::int32_t>(kOneBits)) return 1.0f;
    if (value <= 0.0f) return 0.0f;
    return value;
}

}

void MusicBlock::startTrack(unsigned track, bool loop, bool crossfade, float fade) {
    if (lock_ != 0) return;
    override_ = true;
    request(track, loop, crossfade, fade);
}

void MusicBlock::request(unsigned track, bool loop, bool crossfade, float fade) {
    if (crossfade && currentTrack_ != 0) {
        ramp_ = true;
        target_ = 0.0f;
        queuedPresent_ = true;
        queuedTrack_ = track;
        queuedLoop_ = loop;
        queuedFade_ = fade;
        queuedFlag_ = true;
        return;
    }
    if (streamOpen_) stopNow();
    if (track == 0) return;
    currentTrack_ = track;
    currentLoop_ = loop;
    target_ = fade;
    queuedPresent_ = false;
    queuedTrack_ = 0;
    streamOpen_ = streams_.open(track, loop);
    if (!streamOpen_) return;
    if (crossfade) {
        streams_.setVolume(0.0f);
        volume_ = 0.0f;
        ramp_ = true;
        target_ = fade;
    } else {
        streams_.setVolume(fade);
        volume_ = fade;
    }
}

void MusicBlock::stopNow() {
    if (streamOpen_) {
        streams_.close();
        streamOpen_ = false;
    }
    currentTrack_ = 0;
}

void MusicBlock::fadeOut(float step) {
    step_ = step <= 0.0f ? kMusicInitialRampStep : step;
    ramp_ = true;
    target_ = 0.0f;
}

void MusicBlock::moodRoute(unsigned mood, bool loop, bool crossfade, float fade) {
    if (lock_ != 0 || override_ || !set_) return;
    const MoodChoice choice = chooseMoodTrack(*set_, mood, currentTrack_);
    mood_ = choice.mood;
    if (choice.stop) {
        if (currentTrack_ != 0) {
            ramp_ = true;
            target_ = 0.0f;
        }
        currentTrack_ = 0;
        return;
    }
    if (!choice.track) return;
    if (!crossfade) stopNow();
    request(*choice.track, loop, crossfade, fade);
}

void MusicBlock::update(float dt, float updatesPerSecond) {
    if (streamOpen_ && streams_.finished()) {
        streams_.close();
        streamOpen_ = false;
        currentTrack_ = 0;
    }
    if (!ramp_) return;
    if (streamOpen_) {
        const float delta = dt >= 0.0f ? (step_ * updatesPerSecond) * dt : step_;
        float next;
        if (target_ >= volume_) {
            next = volume_ + delta;
            if (next > target_) {
                next = target_;
                ramp_ = false;
            }
        } else {
            next = volume_ - delta;
            if (target_ > next) {
                next = target_;
                ramp_ = false;
            }
        }
        next = clampVolume(next);
        streams_.setVolume(next);
        volume_ = next;
        if (!(target_ == 0.0f && next == 0.0f)) return;
    }
    stopNow();
    const unsigned track = queuedTrack_;
    request(track, queuedLoop_, queuedFlag_, queuedFade_);
    queuedPresent_ = false;
    queuedTrack_ = 0;
}

}
