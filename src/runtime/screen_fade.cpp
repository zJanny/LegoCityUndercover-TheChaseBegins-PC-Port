#include "runtime/screen_fade.hpp"

#include <algorithm>

namespace chase::runtime {

void ScreenFade::start(float startFraction, float rate, float seconds) {
    started_ = true;
    duration_ = std::max(seconds, 0.0f);
    position_ = startFraction * duration_;
    rate_ = rate;
}

void ScreenFade::update(float dt) {
    if (!started_) return;
    position_ = std::clamp(position_ + dt * rate_, 0.0f, duration_);
}

float ScreenFade::brightness() const {
    if (!started_) return 1.0f;
    if (duration_ <= 0) return 0.0f;
    return position_ / duration_;
}

bool ScreenFade::running() const {
    if (!started_) return false;
    const float value = brightness();
    return rate_ >= 0 ? value < 1.0f : value > 0.0f;
}

}
