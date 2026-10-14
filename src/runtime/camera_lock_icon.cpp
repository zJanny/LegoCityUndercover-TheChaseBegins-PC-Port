#include "runtime/camera_lock_icon.hpp"
#include <algorithm>

namespace chase::runtime {

void CameraLockIcon::trigger() {
    switch (state_) {
    case State::Hidden:
        state_ = State::FadingIn;
        start_ = now_;
        break;
    case State::FadingIn: break;
    case State::Shown: start_ = now_; break;
    case State::FadingOut: {
        const float progress = std::clamp((now_ - start_) / kFadeSeconds, 0.0f, 1.0f);
        state_ = State::FadingIn;
        start_ = now_ - (1.0f - progress) * kFadeSeconds;
        break;
    }
    }
}

void CameraLockIcon::update(float dt) {
    now_ += dt;
    if (state_ == State::FadingIn && now_ - start_ >= kFadeSeconds) {
        state_ = State::Shown;
        start_ = now_;
    } else if (state_ == State::Shown && now_ - start_ >= kHoldSeconds) {
        state_ = State::FadingOut;
        start_ = now_;
    } else if (state_ == State::FadingOut && now_ - start_ >= kFadeSeconds) {
        state_ = State::Hidden;
    }
}

float CameraLockIcon::alpha() const {
    switch (state_) {
    case State::Hidden: return 0.0f;
    case State::FadingIn: return std::clamp((now_ - start_) / kFadeSeconds, 0.0f, 1.0f);
    case State::Shown: return 1.0f;
    case State::FadingOut: return 1.0f - std::clamp((now_ - start_) / kFadeSeconds, 0.0f, 1.0f);
    }
    return 0.0f;
}

}
