#include "runtime/super_brick_display.hpp"

#include <algorithm>
#include <cmath>

namespace chase::runtime {

namespace {
constexpr float kPi = 3.14159265f;
}

void SuperBrickDisplay::show() {
    stamp_ = updates_;
    fader_.trigger();
}

void SuperBrickDisplay::banked(std::uint32_t total) {
    count_ = total;
    bounceStamp_ = updates_;
}

void SuperBrickDisplay::update(float dt) {
    fader_.update(dt);
    ++updates_;
    spin_ += dt * kSuperBrickIconSpinPerSecond;
    if (spin_ > 2.0f * kPi) spin_ -= 2.0f * kPi;
    if (stamp_ == 0) {
        fader_.hide();
        textAlpha_ = 0.0f;
    } else if (updates_ >= stamp_ + kSuperBrickDisplayUpdates) {
        stamp_ = 0;
        bounceStamp_ = 0;
    }
    usedAlpha_ = textAlpha_;
    if (fader_.state() != CameraLockIcon::State::Hidden) textAlpha_ = fader_.alpha();
}

SuperBrickDisplayView SuperBrickDisplay::view() const {
    SuperBrickDisplayView out;
    out.spin = spin_;
    out.count = count_;
    const float alpha = fader_.alpha();
    out.iconVisible = fader_.state() != CameraLockIcon::State::Hidden;
    out.iconAlpha = alpha;
    out.iconX = kSuperBrickAnchorX;
    out.iconY = kSuperBrickAnchorY + kSuperBrickIconOffsetY + kSuperBrickSlide * alpha + kSuperBrickIconSlideBase;
    if (stamp_ != 0) {
        out.textVisible = true;
        out.textX = kSuperBrickAnchorX + kSuperBrickTextOffsetX;
        out.textY = kSuperBrickAnchorY + kSuperBrickTextOffsetY - kSuperBrickSlide + kSuperBrickSlide * usedAlpha_;
        if (updates_ < bounceStamp_ + kSuperBrickBounceUpdates)
            out.textScale = std::sin(float(updates_ - bounceStamp_) * 0.2f * kPi) * 2.0f;
    }
    return out;
}

}
