// The screen fade of the original (src/runtime/screen_fade.cpp): the timer 0x256a14 / 0x252880 and the draw 0x24bda4 with
// the screen gate 0x1087ec (docs/re/cutscenes.md, entry on the fade draw).
#include "runtime/screen_fade.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using chase::runtime::DisplayScreen;
using chase::runtime::FadeScreens;
using chase::runtime::ScreenFade;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b) { return std::fabs(a - b) < 1e-5f; }

// 0x24bda4: opacity |1 - value| (byte [sp + 0x13] = (1 - value) * 255 for value <= 1); nothing is drawn at value 1.
// Provenance: llvm-objdump 0x24be4c..0x24be78 and the linear brightness fall measured in the oracle
// (docs/re/cutscenes.md §2026-10-14T23:52:14, 30 vblanks for 0.5 s).
void opacityFollowsTheValue() {
    ScreenFade fade;
    require(fade.opacityOn(DisplayScreen::Top) == 0.0f, "no fade started: nothing is drawn");
    fade.fadeOut(0.5f);
    require(fade.opacityOn(DisplayScreen::Top) == 0.0f, "value 1: nothing is drawn");
    fade.update(0.125f);
    require(near(fade.opacityOn(DisplayScreen::Top), 0.25f) && near(fade.brightness(), 0.75f), "linear fall");
    fade.update(1.0f);
    require(fade.opacityOn(DisplayScreen::Top) == 1.0f && !fade.running(), "black at the end of the fade out");
    fade.fadeIn(0.5f);
    require(fade.opacityOn(DisplayScreen::Top) == 1.0f && fade.running(), "a fade in starts black");
    fade.update(0.25f);
    require(near(fade.opacityOn(DisplayScreen::Top), 0.5f), "linear rise");
    fade.update(0.25f);
    require(fade.opacityOn(DisplayScreen::Top) == 0.0f && !fade.running(), "clear at the end");
}

// The gate 0x1087ec with both screens rendered (mode byte 0x529e30 + 0x3c4 = 2, read in gameplay-start): screen byte 0 draws
// on both screens, 1 on the top screen only, 2 on the bottom screen only. Provenance: llvm-objdump 0x1087ec..0x108890; the
// byte 1 of the cutscene fades fits the measured end fade that left the bottom screen clear.
void screenByteSelectsTheScreens() {
    ScreenFade all;
    all.fadeOut(1.0f, FadeScreens::All);
    all.update(0.5f);
    require(near(all.opacityOn(DisplayScreen::Top), 0.5f) && near(all.opacityOn(DisplayScreen::Bottom), 0.5f), "byte 0: both screens");
    ScreenFade top;
    top.fadeOut(1.0f, FadeScreens::Top);
    top.update(0.5f);
    require(near(top.opacityOn(DisplayScreen::Top), 0.5f) && top.opacityOn(DisplayScreen::Bottom) == 0.0f, "byte 1: top screen only");
    ScreenFade bottom;
    bottom.fadeOut(1.0f, FadeScreens::Bottom);
    bottom.update(0.5f);
    require(bottom.opacityOn(DisplayScreen::Top) == 0.0f && near(bottom.opacityOn(DisplayScreen::Bottom), 0.5f), "byte 2: bottom screen only");
}

void startReplacesTheFade() {
    ScreenFade fade;
    fade.fadeOut(1.0f, FadeScreens::All);
    fade.update(0.5f);
    fade.fadeIn(1.0f, FadeScreens::Top);
    require(fade.opacityOn(DisplayScreen::Top) == 1.0f && fade.opacityOn(DisplayScreen::Bottom) == 0.0f,
            "a new start replaces the fade, its screen byte too");
    fade.fadeOut(0.0f, FadeScreens::Top);
    require(fade.brightness() == 0.0f && !fade.running(), "a duration of 0 is black at once and not running (0x252880)");
}
}

int main() {
    try {
        opacityFollowsTheValue();
        screenByteSelectsTheScreens();
        startReplacesTheFade();
    } catch (const std::exception& error) {
        std::cerr << "screen-fade test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "screen-fade test passed\n";
    return 0;
}
