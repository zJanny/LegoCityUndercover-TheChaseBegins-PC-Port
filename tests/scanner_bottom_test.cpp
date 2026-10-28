// Rules of the scanner bottom screen (docs/re/abilities-scanner-overlay.md, round 20261028-uiport-D, REA pseudocode and llvm-objdump of
// 0x262d94 (end of the state 0x157 enter 0x1bf2d0), 0x26307c (end of the state 0x158 enter 0x1bf478), 0x2d9f84 (map buttons update),
// 0x1ee520 (button hit test), 0x217d0c (toggle with sounds 0x14 / 0x11), 0x2134c8 (default / disabled look)). The back button is the
// buttonExit reference of mapbuttons.bxaml at 0, 176, 64 x 64 (layout decoder), its touch radius half its width (button +0x24 = 0).
#include "runtime/scanner_bottom.hpp"
#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace chase::runtime;

namespace {
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
bool same(const char* a, const char* b) { return a && b && std::strcmp(a, b) == 0; }
constexpr float kDt = 1.0f / 30.0f;

ScannerBottomTouch touchAt(float x, float y, bool active = true, bool canToggle = true) {
    ScannerBottomTouch t;
    t.touching = true;
    t.x = x;
    t.y = y;
    t.active = active;
    t.canToggle = canToggle;
    return t;
}

void slideIn() {
    ScannerBottom bottom;
    require(bottom.pose().stage == ScannerBottomStage::Map, "the map screen before the scanner");
    bottom.follow(ScannerPhase::Idle, ScannerPhase::TakingOut, kDt);
    require(bottom.pose().stage == ScannerBottomStage::Map && bottom.takeSounds().empty(), "taking the scanner out keeps the map buttons");
    bottom.follow(ScannerPhase::TakingOut, ScannerPhase::Entering, kDt);
    ScannerBottomPose pose = bottom.pose();
    require(pose.stage == ScannerBottomStage::Scanner, "the enter of state 0x157 switches the buttons");
    require(same(pose.exit.slide, "ExitOnDelay") && pose.exit.slideTime == 0.0f, "ExitOnDelay starts at time 0");
    require(same(pose.audio.slide, "AudioOnDelay") && same(pose.audio.look, "disabled"), "the audio button slides in, disabled (type 201)");
    require(same(pose.chars.slide, "CharsOff") && same(pose.chars.look, "disabled"), "the characters button slides out, disabled");
    require(same(pose.exit.look, "default"), "the exit button ends in the default look (0x2134c8(exit, 0) after the toggle off)");
    const std::vector<int> sounds = bottom.takeSounds();
    require(sounds.size() == 2 && sounds[0] == kMapButtonSoundToggleOn && sounds[1] == kMapButtonSoundToggleOff,
            "scan toggled on (0x14), then exit toggled off (0x11)");
    for (int i = 0; i < 16; ++i) bottom.follow(ScannerPhase::Entering, ScannerPhase::Entering, kDt);
    require(std::abs(bottom.pose().exit.slideTime - 16 * kDt) < 1e-5f, "the slide clock advances by the update");
}

void backButton() {
    ScannerBottom bottom;
    bottom.follow(ScannerPhase::TakingOut, ScannerPhase::Entering, kDt);
    bottom.takeSounds();
    require(!bottom.touch(touchAt(32, 208, true, false)), "no exit while the toggle is not allowed (0x375eb6 = 0, view entering)");
    require(!bottom.touch(touchAt(32, 208)), "a touch held from before does not act (touch timer +0x460 not 0)");
    bottom.touch(ScannerBottomTouch{});
    require(!bottom.touch(touchAt(32, 208, false, true)), "no exit while the scanner is off (0x246128)");
    bottom.touch(ScannerBottomTouch{});
    require(!bottom.touch(touchAt(66, 208)), "outside the 32 pixel radius");
    bottom.touch(ScannerBottomTouch{});
    require(bottom.touch(touchAt(32 + 22, 208 + 22)), "inside the circle (31.1 pixels from the centre)");
    ScannerBottomPose pose = bottom.pose();
    require(same(pose.exit.look, "toggleOn") && pose.exit.lookTime == 0.0f, "the tap toggles the exit button on");
    std::vector<int> sounds = bottom.takeSounds();
    require(sounds.size() == 1 && sounds[0] == kMapButtonSoundToggleOn, "the tap plays 0x14");
    bottom.follow(ScannerPhase::Active, ScannerPhase::Exiting, kDt);
    pose = bottom.pose();
    require(pose.stage == ScannerBottomStage::Leaving, "the enter of state 0x158 sends the buttons back");
    require(same(pose.exit.slide, "ExitOff") && same(pose.audio.slide, "AudioOff") && same(pose.chars.slide, "CharsOnDelay"),
            "ExitOff, AudioOff and CharsOnDelay");
    require(same(pose.exit.look, "toggleOn") && pose.exit.lookTime == 0.0f && same(pose.chars.look, "default"),
            "the exit button stays toggled on, the characters button is enabled");
    sounds = bottom.takeSounds();
    require(sounds.size() == 1 && sounds[0] == kMapButtonSoundToggleOff, "only the scan toggle off (0x11): exit was already on");
    bottom.follow(ScannerPhase::Exiting, ScannerPhase::Exiting, kDt);
    require(std::abs(bottom.pose().exit.lookTime - kDt) < 1e-6f, "the look clock advances after the tap's update");
    for (int i = 0; i < 14; ++i) bottom.follow(ScannerPhase::Exiting, ScannerPhase::Exiting, kDt);
    require(bottom.pose().stage == ScannerBottomStage::Leaving, "15 updates after the exit CharsOnDelay (0.533 s) still runs");
    bottom.follow(ScannerPhase::Exiting, ScannerPhase::Exiting, kDt);
    require(bottom.pose().stage == ScannerBottomStage::Map, "after 16 updates the map buttons are back as before the scanner");
}

void exitByB() {
    ScannerBottom bottom;
    bottom.follow(ScannerPhase::TakingOut, ScannerPhase::Entering, kDt);
    bottom.follow(ScannerPhase::Active, ScannerPhase::Exiting, kDt);
    bottom.takeSounds();
    bottom.follow(ScannerPhase::Exiting, ScannerPhase::Idle, kDt);
    bottom.follow(ScannerPhase::Idle, ScannerPhase::TakingOut, kDt);
    bottom.follow(ScannerPhase::TakingOut, ScannerPhase::Entering, kDt);
    const std::vector<int> sounds = bottom.takeSounds();
    require(sounds.size() == 2 && sounds[0] == kMapButtonSoundToggleOn, "a second entry toggles scan on again after the exit turned it off");
    ScannerBottom other;
    other.follow(ScannerPhase::TakingOut, ScannerPhase::Entering, kDt);
    other.takeSounds();
    other.follow(ScannerPhase::Active, ScannerPhase::Exiting, kDt);
    const std::vector<int> leaving = other.takeSounds();
    require(leaving.size() == 2 && leaving[0] == kMapButtonSoundToggleOff && leaving[1] == kMapButtonSoundToggleOn,
            "an exit by B toggles scan off and the exit button on (0x11, 0x14)");
    require(same(other.pose().exit.look, "toggleOn"), "the exit button turns on when the exit came from B");
}
}

int main() {
    try {
        require(scannerBackTapped(true, false, 32, 208) && !scannerBackTapped(true, true, 32, 208) && !scannerBackTapped(false, false, 32, 208),
                "the back button acts on the first update of a touch");
        require(scannerBackTapped(true, false, 0, 208) && !scannerBackTapped(true, false, 32, 175), "radius 32 around 32, 208");
        slideIn();
        backButton();
        exitByB();
    } catch (const std::exception& error) {
        std::cerr << "scanner-bottom-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "scanner-bottom-test: ok\n";
    return 0;
}
