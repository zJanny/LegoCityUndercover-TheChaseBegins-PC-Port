#include "runtime/scanner_reticle.hpp"
#include <algorithm>

namespace chase::runtime {

void ScannerReticle::reset() {
    for (int a = 0; a < 2; ++a) position_[a] = from_[a] = kScannerScreenCentre[a];
}

ScannerReticlePose ScannerReticle::update(const ScannerReticleInput& input) {
    ScannerReticlePose pose;
    pose.zoomMeterFraction = 1.0f - (input.zoom - input.zoomMin) / (input.zoomMax - input.zoomMin);
    switch (input.state) {
    case 1:
        if (input.hasTarget) {
            if (input.lockProgress < 1.0f)
                for (int a = 0; a < 2; ++a) position_[a] = from_[a] + (input.targetScreen[a] - from_[a]) * input.lockProgress;
            else
                for (int a = 0; a < 2; ++a) position_[a] = from_[a] = input.targetScreen[a];
        }
        break;
    case 2:
        if (input.hasTarget)
            for (int a = 0; a < 2; ++a) position_[a] = from_[a] = input.targetScreen[a];
        break;
    case 5:
    case 6:
    case 7:
    case 8:
        for (int a = 0; a < 2; ++a) position_[a] = from_[a] = input.hasCurrent ? input.currentScreen[a] : kScannerScreenCentre[a];
        break;
    default:
        break;
    }
    std::copy(position_, position_ + 2, pose.position);
    switch (input.state) {
    case 5:
    case 7:
        pose.lockingOn = {"Ret_LockingOn", input.scanProgress};
        pose.scanMeterFraction = input.scanProgress;
        pose.scanMeterShown = true;
        break;
    case 6:
        pose.lockingOn = {"Ret_LockingOn", 1.0f};
        pose.scanMeterFraction = 1.0f;
        pose.scanMeterShown = true;
        break;
    case 8:
        pose.focus = {"Ret_Focus", input.scanProgress};
        pose.scanMeterShown = false;
        break;
    default:
        pose.lockingOn = {"Ret_LockingOn", 0.0f};
        pose.focus = {"Ret_Focus", input.lockProgress};
        break;
    }
    return pose;
}

}
