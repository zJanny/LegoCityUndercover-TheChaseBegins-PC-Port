#pragma once
// The screen fade of the original (block 0x529e30, docs/re/cutscenes.md): one fade shared by FadeScreen 0x1a2e8c, the
// DCamCutScenePlay handler 0x26fa18 and the cutscene mode (enter 0x2fadb8, update 0x2fae68, exit 0x2fac7c). The start
// 0x256a14 sets the timer at +0x11c: the duration (0x114aa4), the start position as a fraction of it (0x1150e0) and the
// rate (0x1140a0), and stores the screen byte r1 at +0x118. The value 0x252880 is position / duration (0 for a duration
// of 0), the position (0x1202f8) moves with the clock at the rate and stays inside [0, duration]. The value is the
// brightness: 1 normal, 0 black. Whatever starts a fade replaces the one before. The draw 0x24bda4 (thunk 0x1166bc) does
// nothing at value 1 and otherwise draws a black quad over the whole screen with the alpha byte |1 - value| * 255 (record
// byte 0x13), blended source alpha / one minus source alpha with equation add (record bytes 8 = 4, 9 = 5, 7 = 0; GPU blend
// register 0x101 written by 0x1292fc through the factor table 0x37628c) (docs/re/cutscenes.md, docs/re/script-text.md);
// the screen gate 0x1087ec decides from the screen byte which screens get it. No SDL dependency.

namespace chase::runtime {

enum class FadeScreens : unsigned char { All = 0, Top = 1, Bottom = 2 };
enum class DisplayScreen { Top, Bottom };

class ScreenFade {
public:
    // 0x2520e8: start position 1, rate -1 (to black).
    void fadeOut(float seconds, FadeScreens screens = FadeScreens::All) { start(1.0f, -1.0f, seconds, screens); }
    // 0x252068: start position 0, rate +1 (from black).
    void fadeIn(float seconds, FadeScreens screens = FadeScreens::All) { start(0.0f, 1.0f, seconds, screens); }
    void start(float startFraction, float rate, float seconds, FadeScreens screens = FadeScreens::All);
    void update(float dt);
    float brightness() const;
    // 0x25207c: a fade has been started and its value has not reached its end (1 for a positive rate, 0 otherwise).
    bool running() const;
    // 0x1087ec with both screens rendered (render mode byte 0x529e30 + 0x3c4 = 2, measured in gameplay): byte 0 draws on
    // both screens, 1 on the top screen only, 2 on the bottom screen only.
    bool coversScreen(DisplayScreen screen) const;
    // 0x24bda4: 0 at value 1, else the alpha byte (int)(|1 - value| * 255) (0x24be58 / 0x24be70, truncated) / 255; the quad is black.
    float opacityOn(DisplayScreen screen) const;

private:
    bool started_ = false;
    float duration_ = 0, position_ = 0, rate_ = 0;
    FadeScreens screens_ = FadeScreens::All;
};

}
