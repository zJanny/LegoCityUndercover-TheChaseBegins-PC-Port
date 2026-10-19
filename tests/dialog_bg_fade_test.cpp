// The panel alpha of the bottom screen (src/runtime/dialog_bg_fade.cpp) and the screen byte of FadeScreen. Values come from
// llvm-objdump on game.arm.elf (docs/re/script-text.md, entry on the dialogue background draw 0x2d9410): the HUD update
// 0x2de264..0x2de2a0 steps the float 0x51a2d4 by 0.2 (literal 0x2de44c) per update, up while the tip state byte is not 0
// (0x1ee790) or the BlackoutMap byte 0x519e24 + 0x484 is set (0x19464c, handler 0x1b23f4), clamped to 0..1; the draw
// 0x2d9410 returns at alpha 0 and packs (int)(alpha * 255) (0x2d94a4). FadeScreen 0x1a2e8c passes 0x1d9584() != 0, the
// byte 0x51a2bd that ShowDialogBG writes through 0x1a21dc, as the screen byte (screen gate 0x1087ec: 1 = top screen only).
#include "assets/assets.hpp"
#include "runtime/dialog_bg_fade.hpp"
#include "runtime/script_runner.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;
using runtime::DialogBackgroundFade;
using runtime::DisplayScreen;
using runtime::ScriptRunner;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b) { return std::fabs(a - b) < 1e-5f; }

void add(assets::Level& level, const std::string& name, std::vector<assets::ScriptCall> calls) {
    assets::LevelScript script;
    script.nameHash = assets::nameHash(name);
    script.calls = std::move(calls);
    level.scripts.push_back(std::move(script));
}

void alphaStepsPerUpdate() {
    DialogBackgroundFade fade;
    require(!fade.drawn() && fade.alpha() == 0.0f, "nothing drawn before a tip");
    fade.update(true, false);
    require(near(fade.alpha(), 0.2f) && fade.drawn(), "0.2 up per update while the tip is shown");
    for (int i = 0; i < 10; ++i) fade.update(true, false);
    require(fade.alpha() == 1.0f, "clamped to 1");
    fade.update(false, false);
    require(near(fade.alpha(), 0.8f), "0.2 down per update when neither condition holds");
    for (int i = 0; i < 10; ++i) fade.update(false, false);
    require(fade.alpha() == 0.0f && !fade.drawn(), "clamped to 0, the draw returns");
    fade.update(false, true);
    require(near(fade.alpha(), 0.2f), "the BlackoutMap byte raises the alpha without a tip");
    for (int i = 0; i < 4; ++i) fade.update(false, true);
    require(fade.alpha() == 1.0f, "BlackoutMap holds the background at 1");
    for (int i = 0; i < 5; ++i) fade.update(false, false);
    require(fade.alpha() > 0.0f && fade.drawn() && fade.drawnAlpha() == 0.0f,
            "five steps down leave a float residue (2.4e-8, RPC poll in script-text.md): the draw runs with alpha byte 0");
}

// The tip draw 0x15b518 reads the panel alpha through 0x1b1fe0 (REA xrefs 0x51a2d4: only 0x1b1fe4), so a tip that starts while
// BlackoutMap holds the alpha at 1 is drawn at 1 at once.
void tipUsesThePanelAlpha() {
    runtime::ScriptTextState text;
    for (int i = 0; i < 5; ++i) text.update(1.0f / 30, true);
    require(text.dialogueBackground().alpha() == 1.0f, "BlackoutMap: panel alpha 1");
    text.showTip(1, 1.0f);
    require(text.tip().alpha == 1.0f, "the tip starts at the panel alpha");
    text.update(1.0f / 30, true);
    require(text.tip().alpha == 1.0f && text.dialogueBackground().alpha() == 1.0f, "one value for both");
    for (int i = 0; i < 80 && text.tip().active; ++i) text.update(1.0f / 30, false);
    require(!text.tip().active && text.dialogueBackground().alpha() == 0.0f, "both end together without BlackoutMap");
}

void blackoutMapHoldsTheBackground() {
    assets::Level level;
    add(level, "b", {{"BlackoutMap", {"1"}}, {"Wait", {"1"}}, {"BlackoutMap", {"0"}}});
    assets::PlacedObjectState state;
    ScriptRunner runner(level, state);
    require(runner.start("b"), "start");
    for (int i = 0; i < 5; ++i) runner.update(1.0f / 30);
    require(runner.objectives().mapBlackout() && runner.text().dialogueBackground().alpha() == 1.0f, "BlackoutMap 1 covers the map");
    for (int i = 0; i < 40 && runner.objectives().mapBlackout(); ++i) runner.update(1.0f / 30);
    require(!runner.objectives().mapBlackout(), "BlackoutMap 0 clears the byte");
    for (int i = 0; i < 5; ++i) runner.update(1.0f / 30);
    require(runner.text().dialogueBackground().drawnAlpha() == 0.0f, "the background is invisible after five updates");
    runner.update(1.0f / 30);
    require(runner.text().dialogueBackground().alpha() == 0.0f, "the sixth update clamps the residue to 0");
}

void fadeScreenUsesTheDialogBgFlag() {
    assets::Level level;
    add(level, "plain", {{"FadeScreen", {"0"}}});
    add(level, "dialog", {{"ShowDialogBG", {"1"}}, {"FadeScreen", {"0"}}});
    assets::PlacedObjectState state;
    ScriptRunner plain(level, state);
    plain.start("plain");
    plain.update(0.25f);
    require(near(plain.screenFade().opacityOn(DisplayScreen::Top), 127.0f / 255.0f) && near(plain.screenFade().opacityOn(DisplayScreen::Bottom), 127.0f / 255.0f),
            "byte 0: both screens fade");
    assets::PlacedObjectState dialogState;
    ScriptRunner dialog(level, dialogState);
    dialog.start("dialog");
    dialog.update(0.25f);
    require(near(dialog.screenFade().opacityOn(DisplayScreen::Top), 127.0f / 255.0f) && dialog.screenFade().opacityOn(DisplayScreen::Bottom) == 0.0f,
            "ShowDialogBG 1: byte 1, the top screen only");
}
}

int main() {
    try {
        alphaStepsPerUpdate();
        tipUsesThePanelAlpha();
        blackoutMapHoldsTheBackground();
        fadeScreenUsesTheDialogBgFlag();
    } catch (const std::exception& error) {
        std::cerr << "dialog-bg-fade test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "dialog-bg-fade test passed\n";
    return 0;
}
