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
        const float progress = fadeOut_ > 0.0f ? std::clamp((now_ - start_) / fadeOut_, 0.0f, 1.0f) : 1.0f;
        state_ = State::FadingIn;
        start_ = now_ - (1.0f - progress) * fadeIn_;
        break;
    }
    }
}

void CameraLockIcon::hide() {
    if (state_ == State::FadingIn) {
        start_ = now_ - (1.0f - alpha()) * fadeOut_;
        state_ = fadeOut_ > 0.0f ? State::FadingOut : State::Hidden;
    } else if (state_ == State::Shown) {
        start_ = now_;
        state_ = fadeOut_ > 0.0f ? State::FadingOut : State::Hidden;
    }
}

void CameraLockIcon::update(float dt) {
    now_ += dt;
    if (state_ == State::FadingIn && now_ - start_ >= fadeIn_) {
        state_ = State::Shown;
        start_ = now_;
    } else if (state_ == State::Shown && hold_ >= 0.0f && now_ - start_ >= hold_) {
        state_ = State::FadingOut;
        start_ = now_;
    } else if (state_ == State::FadingOut && now_ - start_ >= fadeOut_) {
        state_ = State::Hidden;
    }
}

float CameraLockIcon::alpha() const {
    switch (state_) {
    case State::Hidden: return 0.0f;
    case State::FadingIn: return fadeIn_ > 0.0f ? std::clamp((now_ - start_) / fadeIn_, 0.0f, 1.0f) : 1.0f;
    case State::Shown: return 1.0f;
    case State::FadingOut: return fadeOut_ > 0.0f ? 1.0f - std::clamp((now_ - start_) / fadeOut_, 0.0f, 1.0f) : 0.0f;
    }
    return 0.0f;
}

}
