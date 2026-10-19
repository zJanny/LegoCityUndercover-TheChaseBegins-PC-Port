#pragma once
// Which scanner UI storyboards play at which time for the audio scanner view (docs/re/abilities-scanner.md): the zoom meter, the
// reticle of sprites/blend/DRC_Scanner/Reticule_Audio and the scan meter. Rules from the reticle update 0x2e9030, the zoom meter
// update 0x1d99a8 / 0x263400 and the lock update 0x2ef4c8. No SDL dependency.
#include <string>

namespace chase::runtime {

constexpr float kScannerScreenCentre[2] = {200.0f, 120.0f};

struct ScannerReticleInput {
    float zoom = 43.0f, zoomMin = 20.0f, zoomMax = 43.0f;
    int state = 0;  // mode +0x3c
    float lockProgress = 0, scanProgress = 0;
    bool hasTarget = false;
    float targetScreen[2] = {0, 0};   // screen position of the target (+0x48)
    bool hasCurrent = false;
    float currentScreen[2] = {0, 0};  // screen position of the scanned object (+0x44)
    float dt = 1.0f / 30.0f;
};

// A fraction of a storyboard's length (the original multiplies by the length from 0x23caa4).
struct ScannerBoardFraction {
    std::string name;
    float fraction = 0;
};

struct ScannerReticlePose {
    float zoomMeterFraction = 0;  // 0x263400: 1 - (zoom - min) / (max - min), time of the storyboard "Zoom"
    ScannerBoardFraction lockingOn, focus;  // "Ret_LockingOn" and "Ret_Focus" of Reticule_Audio, applied in this order
    float scanMeterFraction = 0;  // time of the storyboard "Scanning" of ScanMeter
    bool scanMeterShown = false;
    float position[2] = {kScannerScreenCentre[0], kScannerScreenCentre[1]};  // mode +0x18 / +0x1c
};

class ScannerReticle {
public:
    ScannerReticlePose update(const ScannerReticleInput& input);
    void reset();

private:
    float position_[2] = {kScannerScreenCentre[0], kScannerScreenCentre[1]};
    float from_[2] = {kScannerScreenCentre[0], kScannerScreenCentre[1]};
};

}
