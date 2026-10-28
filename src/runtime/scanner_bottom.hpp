#pragma once
// The bottom screen of the scanner view (docs/re/abilities-scanner-overlay.md): the map buttons that the enter of state 0x157 (0x1bf2d0 ->
// 0x262d94) and of state 0x158 (0x1bf478 -> 0x26307c) switch, and the back button of the map buttons update 0x2d9f84. Only the audio zone
// (type 201, kind 1) is ported: its audio button stays in the disabled look. No SDL dependency.
// STAND-INS (SCAN-S13): the slide storyboards and button looks advance by the scanner update (1/30 s) from the update that enters the state;
// of the looks started in one update the last one is shown; the map button sounds 0x14 / 0x11 of the bank *0x375f44 are played as the UI
// sounds of those ids; the gates of 0x2d9f84 around the hit test (0x1ee790, 0x21d744, 0x236008, 0x236104, the map page byte +0x498,
// 0x375fc4 + 0x24, and state 6 of the storyboard +0x3d4 (0x245f84) that keeps a hit) are not checked.
// STAND-IN (SCAN-S14): the back button's touch circle is its layout rectangle (0, 176, 64 x 64) with the radius half its width; the
// original reads the reference's resolved position (0x23ca28), which equals it once ExitOnDelay has ended.
#include "runtime/scanner.hpp"
#include <vector>

namespace chase::runtime {

constexpr float kScannerBackCentreX = 32.0f;
constexpr float kScannerBackCentreY = 208.0f;
constexpr float kScannerBackRadius = 32.0f;
// STAND-IN (SCAN-S13): the original's enter and exit (0x262d94, 0x26307c) act on the one map buttons object 0x519e24; the port keeps
// this pose apart from runtime/map_buttons.hpp and hands the lower screen back to it when CharsOnDelay (0.533 s, the longest exit slide of
// mapbuttons.bxaml, layout decoder) has ended.
constexpr float kScannerBottomLeaveSeconds = 0.533f;
constexpr int kMapButtonSoundToggleOn = 0x14;
constexpr int kMapButtonSoundToggleOff = 0x11;

enum class ScannerBottomStage { Map, Scanner, Leaving };

struct ScannerBottomButton {
    const char* slide = nullptr;  // storyboard of mapbuttons.bxaml that moves the button
    float slideTime = 0;
    const char* look = "default";  // storyboard of button.bxaml
    float lookTime = 0;
};

struct ScannerBottomPose {
    ScannerBottomStage stage = ScannerBottomStage::Map;
    ScannerBottomButton chars, exit, audio;
};

struct ScannerBottomTouch {
    bool touching = false;
    float x = 0, y = 0;  // bottom-screen pixels
    bool active = false;     // 0x246128: the scanner is on (0x375ee8)
    bool canToggle = false;  // 0x375eb6, part of 0x1ee4f0
};

bool scannerBackTapped(bool touching, bool wasTouching, float x, float y);

class ScannerBottom {
public:
    // Before the scanner update: true when the back button asks for the exit (0x2249e0(1, 0), as B does).
    bool touch(const ScannerBottomTouch& input);
    // After the scanner update, with its phase before and after.
    void follow(ScannerPhase before, ScannerPhase after, float dt);
    ScannerBottomPose pose() const;
    std::vector<int> takeSounds();

private:
    void toggleScan(bool on);
    void toggleExit(bool on);

    ScannerBottomStage stage_ = ScannerBottomStage::Map;
    float clock_ = 0;
    const char* exitLook_ = "default";
    float exitLookClock_ = 0;
    bool scanToggled_ = false;
    bool exitToggled_ = false;
    bool exitLookChanged_ = false;
    bool wasTouching_ = false;
    std::vector<int> sounds_;
};

}
