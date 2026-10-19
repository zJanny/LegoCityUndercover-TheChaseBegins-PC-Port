#pragma once
// The camera shake of the camera manager (state at 0x376870, vector at 0x56d47c), recovered from the start 0x22c500
// (ShakeScreen 0x1b2690 calls it) and the manager update 0x23f174 (docs/re/dcam.md). No SDL dependency.
//
// 0x22c500(ampX, ampY, ampZ, damp, lock, seconds): ignored while a shake runs (time M+4 not 0) and the lock byte M+0 is set;
// otherwise M+0 = lock, M+4 = seconds, the vector = amp * 0.1 per axis (bytes 0..255), M+0x34 = seconds when damp else 0.
// The update 0x23f174, when no override routine (M+0x18) is set and M+4 > 0, draws three values 0x2345dc in the order
// z, y, x, takes (r - 0.5) * vector per axis, multiplies by M+4 / M+0x34 when M+0x34 is not 0, adds the result to the
// translation of the camera matrix and subtracts the update time from M+4. M+4 is not clamped at 0.
// Not recovered: the override routine M+0x18, and the callers other than ShakeScreen that pass the lock byte.
#include "runtime/game_random.hpp"

namespace chase::runtime {

class ScreenShake {
public:
    // ShakeScreen passes the same byte for all three axes and the lock byte 0.
    void start(unsigned ampX, unsigned ampY, unsigned ampZ, bool damp, bool lock, float seconds);
    // One camera update: the offset to add to the eye (zero when no shake runs) and the time step.
    void update(float dt, GameRandom& random, float offset[3]);
    float secondsLeft() const { return seconds_; }

private:
    bool lock_ = false;
    float seconds_ = 0.0f;
    float damp_ = 0.0f;
    float amplitude_[3] = {0, 0, 0};
};

}
