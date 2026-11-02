#include "runtime/bullet_time.hpp"

namespace chase::runtime {

bool BulletTimeSystem::set(float scale, float duration) {
    if (active_ ? (scale_ == 1.0f && scale == 1.0f) : scale == 1.0f) return false;
    const bool slowing = scale < 1.0f;
    base_ = worldScale_;
    scale_ = scale;
    duration_ = duration;
    start_ = clock_;
    active_ = true;
    if (slowing) {
        mode_ = kBulletTimeSlowMode;
        messages_.push_back({kBulletTimeMessageStart, std::nullopt});
        return true;
    }
    messages_.push_back({kBulletTimeMessageStart, duration});
    messages_.push_back({kBulletTimeMessageRestoring, duration});
    messages_.push_back({kBulletTimeMessageEnd, std::nullopt});
    return true;
}

void BulletTimeSystem::update(float dt) {
    clock_ += dt;
    if (!active_) return;
    float progress = (clock_ - start_) / duration_;
    if (progress > 1.0f) {
        progress = 1.0f;
        if (scale_ == 1.0f) active_ = false;
    }
    worldScale_ = base_ + progress * (scale_ - base_);
}

void BulletTimeSystem::stop() {
    worldScale_ = 1.0f;
    active_ = false;
}

std::vector<BulletTimeMessage> BulletTimeSystem::takeMessages() {
    std::vector<BulletTimeMessage> taken;
    taken.swap(messages_);
    return taken;
}

}
