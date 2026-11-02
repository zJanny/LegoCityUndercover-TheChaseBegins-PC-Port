#include "runtime/bullet_time.hpp"

namespace chase::runtime {

bool BulletTimeSystem::set(float scale, float duration) {
    if (active_ ? (scale_ == 1.0f && scale == 1.0f) : scale == 1.0f) return false;
    const bool slowing = scale < 1.0f;
    base_ = timeScale_;
    scale_ = scale;
    duration_ = duration;
    start_ = clock_;
    active_ = true;
    reached_ = false;
    if (slowing) {
        mode_ = kBulletTimeSlowMode;
        sounds_.push_back({false, kBulletTimeSoundStart, 0.0f});
        return true;
    }
    sounds_.push_back({true, kBulletTimeSoundStart, duration});
    sounds_.push_back({true, kBulletTimeSoundLoop, duration});
    sounds_.push_back({false, kBulletTimeSoundEnd, 0.0f});
    return true;
}

bool BulletTimeSystem::update(float realDt, bool loopSoundIdle) {
    clock_ += realDt;
    if (!active_) return false;
    if (loopSoundIdle && scale_ != 1.0f) sounds_.push_back({false, kBulletTimeSoundLoop, 0.0f});
    float progress = (clock_ - start_) / duration_;
    if (progress > 1.0f) {
        progress = 1.0f;
        if (scale_ == 1.0f) active_ = false;
    }
    timeScale_ = base_ + progress * (scale_ - base_);
    const bool reachedNow = progress == 1.0f && !reached_;
    if (reachedNow) reached_ = true;
    return reachedNow;
}

void BulletTimeSystem::stop() {
    timeScale_ = 1.0f;
    active_ = false;
}

std::vector<BulletTimeSound> BulletTimeSystem::takeSounds() {
    std::vector<BulletTimeSound> taken;
    taken.swap(sounds_);
    return taken;
}

}
