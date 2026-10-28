#include "runtime/scanner_bottom.hpp"
#include <cmath>

namespace chase::runtime {

bool scannerBackTapped(bool touching, bool wasTouching, float x, float y) {
    return touching && !wasTouching && std::hypot(x - kScannerBackCentreX, y - kScannerBackCentreY) <= kScannerBackRadius;
}

bool ScannerBottom::touch(const ScannerBottomTouch& input) {
    const bool tapped = scannerBackTapped(input.touching, wasTouching_, input.x, input.y);
    wasTouching_ = input.touching;
    if (!tapped || !input.active || !input.canToggle) return false;
    toggleExit(true);
    return true;
}

void ScannerBottom::follow(ScannerPhase before, ScannerPhase after, float dt) {
    if (after == ScannerPhase::Entering && before != ScannerPhase::Entering) {
        stage_ = ScannerBottomStage::Scanner;
        clock_ = 0;
        if (!scanToggled_) toggleScan(true);
        toggleExit(false);
        exitLook_ = "default";
    } else if (after == ScannerPhase::Exiting && before != ScannerPhase::Exiting) {
        stage_ = ScannerBottomStage::Leaving;
        clock_ = 0;
        toggleScan(false);
        if (!exitToggled_) toggleExit(true);
    } else {
        clock_ += dt;
        if (!exitLookChanged_) exitLookClock_ += dt;
        if (stage_ == ScannerBottomStage::Leaving && clock_ >= kScannerBottomLeaveSeconds) stage_ = ScannerBottomStage::Map;
    }
    exitLookChanged_ = false;
}

ScannerBottomPose ScannerBottom::pose() const {
    ScannerBottomPose pose;
    pose.stage = stage_;
    if (stage_ == ScannerBottomStage::Map) return pose;
    const bool scanner = stage_ == ScannerBottomStage::Scanner;
    pose.chars = {scanner ? "CharsOff" : "CharsOnDelay", clock_, scanner ? "disabled" : "default", 0};
    pose.exit = {scanner ? "ExitOnDelay" : "ExitOff", clock_, exitLook_, exitLookClock_};
    pose.audio = {scanner ? "AudioOnDelay" : "AudioOff", clock_, "disabled", 0};
    return pose;
}

std::vector<int> ScannerBottom::takeSounds() {
    std::vector<int> out;
    out.swap(sounds_);
    return out;
}

void ScannerBottom::toggleScan(bool on) {
    scanToggled_ = on;
    sounds_.push_back(on ? kMapButtonSoundToggleOn : kMapButtonSoundToggleOff);
}

void ScannerBottom::toggleExit(bool on) {
    exitToggled_ = on;
    exitLook_ = on ? "toggleOn" : "toggleOff";
    exitLookClock_ = 0;
    exitLookChanged_ = true;
    sounds_.push_back(on ? kMapButtonSoundToggleOn : kMapButtonSoundToggleOff);
}

}
