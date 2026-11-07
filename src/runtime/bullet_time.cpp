#include "runtime/bullet_time.hpp"

namespace chase::runtime {

bool BulletTimeSystem::set(float scale, float duration, bool flag, std::optional<std::uint8_t> shotRecordMode) {
    if (active_ ? (scale_ == 1.0f && scale == 1.0f) : scale == 1.0f) return false;
    const bool slowing = scale < 1.0f;
    base_ = timeScale_;
    scale_ = scale;
    duration_ = duration;
    start_ = clock_;
    active_ = true;
    reached_ = false;
    if (slowing) {
        mode_ = bulletTimeSlowingMode(flag, true, shotRecordMode);
        cameraShot_ = flag;
        sounds_.push_back({false, 0, 0.0f, BulletTimeVoices::Pause});
        sounds_.push_back({false, kBulletTimeSoundStart, 0.0f});
        return true;
    }
    modeCall_ = bulletTimeModeCall(mode_);
    restoreObject_ = true;
    sounds_.push_back({true, kBulletTimeSoundStart, duration});
    sounds_.push_back({true, kBulletTimeSoundLoop, duration});
    sounds_.push_back({false, 0, 0.0f, BulletTimeVoices::Resume});
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

BulletTimeModeCall BulletTimeSystem::takeModeCall() {
    const BulletTimeModeCall call = modeCall_;
    modeCall_ = BulletTimeModeCall::None;
    return call;
}

bool BulletTimeSystem::takeRestoreObjectRequest() {
    const bool request = restoreObject_;
    restoreObject_ = false;
    return request;
}

std::vector<BulletTimeSound> BulletTimeSystem::takeSounds() {
    std::vector<BulletTimeSound> taken;
    taken.swap(sounds_);
    return taken;
}

}
