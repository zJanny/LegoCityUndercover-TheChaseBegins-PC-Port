#pragma once
// The screen fade of the original (block 0x529e30, docs/re/cutscenes.md): one fade shared by FadeScreen 0x1a2e8c, the
// DCamCutScenePlay handler 0x26fa18 and the cutscene mode (enter 0x2fadb8, update 0x2fae68, exit 0x2fac7c). The start
// 0x256a14 sets the timer at +0x11c: the duration (0x114aa4), the start position as a fraction of it (0x1150e0) and the
// rate (0x1140a0). The value 0x252880 is position / duration (0 for a duration of 0), the position (0x1202f8) moves
// with the clock at the rate and stays inside [0, duration]. The value is the brightness: 1 normal, 0 black. Whatever
// starts a fade replaces the one before. No SDL dependency.

namespace chase::runtime {

class ScreenFade {
public:
    // 0x2520e8: start position 1, rate -1 (to black).
    void fadeOut(float seconds) { start(1.0f, -1.0f, seconds); }
    // 0x252068: start position 0, rate +1 (from black).
    void fadeIn(float seconds) { start(0.0f, 1.0f, seconds); }
    void start(float startFraction, float rate, float seconds);
    void update(float dt);
    float brightness() const;
    // 0x25207c: a fade has been started and its value has not reached its end (1 for a positive rate, 0 otherwise).
    bool running() const;

private:
    bool started_ = false;
    float duration_ = 0, position_ = 0, rate_ = 0;
};

}
