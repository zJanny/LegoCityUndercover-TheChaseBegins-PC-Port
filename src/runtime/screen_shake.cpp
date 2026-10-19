#include "runtime/screen_shake.hpp"

namespace chase::runtime {

void ScreenShake::start(unsigned ampX, unsigned ampY, unsigned ampZ, bool damp, bool lock, float seconds) {
    if (seconds_ != 0.0f && lock_) return;
    lock_ = lock;
    seconds_ = seconds;
    amplitude_[0] = float(ampX & 255u) * 0.1f;
    amplitude_[1] = float(ampY & 255u) * 0.1f;
    amplitude_[2] = float(ampZ & 255u) * 0.1f;
    damp_ = damp ? seconds : 0.0f;
}

void ScreenShake::update(float dt, GameRandom& random, float offset[3]) {
    offset[0] = offset[1] = offset[2] = 0.0f;
    if (!(seconds_ > 0.0f)) return;
    offset[2] = (gameRandomUnit(random) - 0.5f) * amplitude_[2];
    offset[1] = (gameRandomUnit(random) - 0.5f) * amplitude_[1];
    offset[0] = (gameRandomUnit(random) - 0.5f) * amplitude_[0];
    if (damp_ != 0.0f) {
        const float scale = seconds_ / damp_;
        for (int a = 0; a < 3; ++a) offset[a] *= scale;
    }
    seconds_ -= dt;
}

}
