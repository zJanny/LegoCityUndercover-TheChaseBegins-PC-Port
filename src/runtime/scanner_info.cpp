#include "runtime/scanner_info.hpp"
#include <algorithm>

namespace chase::runtime {

int scannerInfoIcon(int iconByte, bool hasAbility) { return iconByte == kScannerInfoAlienIcon && !hasAbility ? 0 : iconByte; }

void ScannerInfo::reset() { *this = ScannerInfo(); }

void ScannerInfo::select(float reticleY, int state) {
    const float hysteresis = state == 6 ? kScannerInfoHysteresis : kScannerInfoIdleHysteresis;
    const float line = placement_ == 1 ? kScannerInfoSwitchLine - hysteresis : kScannerInfoSwitchLine + hysteresis;
    if (reticleY > line) {
        box_ = ScannerInfoBox::Top;
        placement_ = 1;
    } else {
        box_ = ScannerInfoBox::Bottom;
        placement_ = 2;
    }
}

ScannerInfoPose ScannerInfo::update(const ScannerInfoInput& input) {
    if (playing_) time_ += input.dt;
    const float length = leaving_ ? kScannerInfoOffSeconds : kScannerInfoOnSeconds;
    const bool ended = playing_ && time_ >= length;

    if (input.state == 6) {
        if (input.current) {
            record_ = *input.current;
            hasRecord_ = true;
        } else {
            hasRecord_ = false;
        }
    }
    if (hasRecord_) {
        const float d = record_.distanceSquared;
        opacity_ = d < kScannerInfoNearSquared ? 1.0f : std::max(0.0f, 1.0f - (d - kScannerInfoNearSquared) / kScannerInfoFadeSquared);
    }

    switch (phase_) {
    case 0:
        if (input.state == 6 && previousState_ != 6) phase_ = 1;
        break;
    case 1:
        if (input.target) {
            record_ = *input.target;
            hasRecord_ = true;
        }
        select(input.reticleY, input.state);
        leaving_ = false;
        playing_ = true;
        time_ = 0;
        icon_ = scannerInfoIcon(record_.icon, false);
        phase_ = 2;
        break;
    case 2:
        if (ended) phase_ = 3;
        break;
    case 3:
        select(input.reticleY, input.state);
        if (input.state != 6) phase_ = 4;
        break;
    case 4:
        leaving_ = true;
        playing_ = true;
        time_ = 0;
        phase_ = 5;
        break;
    case 5:
        if (ended) {
            box_ = ScannerInfoBox::None;
            placement_ = 0;
            hasRecord_ = false;
            playing_ = false;
            leaving_ = false;
            phase_ = 0;
        }
        break;
    }
    previousState_ = input.state;

    ScannerInfoPose pose;
    pose.box = box_;
    pose.leaving = leaving_;
    pose.time = playing_ ? std::min(time_, leaving_ ? kScannerInfoOffSeconds : kScannerInfoOnSeconds) : (leaving_ ? kScannerInfoOffSeconds : kScannerInfoOnSeconds);
    pose.opacity = opacity_;
    pose.icon = icon_;
    if (hasRecord_) {
        pose.offset[0] = record_.screen[0] - kScannerInfoCentre[0];
        pose.offset[1] = record_.screen[1] - kScannerInfoCentre[1];
    }
    return pose;
}

}
