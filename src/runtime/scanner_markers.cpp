#include "runtime/scanner_markers.hpp"
#include <algorithm>
#include <cmath>

namespace chase::runtime {

void ScannerMarkers::reset() {
    slots_ = {};
    pulses_ = {};
    poses_.clear();
}

void ScannerMarkers::startPulses(PulseSlot& slot) const {
    for (std::size_t k = 0; k < 3; ++k) {
        slot.time[k] = 0;
        slot.delay[k] = kScannerPulseDelayUpdates * float(k) * kScannerUpdateSeconds;
    }
}

const std::vector<ScannerMarkerPose>& ScannerMarkers::update(float dt, const std::vector<ScannerMarkerInput>& records) {
    poses_.assign(std::min(records.size(), kScannerMarkerSlots), {});
    std::size_t pulseIndex = 0;
    for (std::size_t i = 0; i < poses_.size(); ++i) {
        Slot& slot = slots_[i];
        ScannerMarkerPose& pose = poses_[i];
        if (!records[i].visible) {
            slot = {};
            continue;
        }
        if (!slot.shown) {
            slot.shown = true;
            slot.fading = true;
            slot.fade = 0;
            slot.centreTime = 0;
        } else if (slot.fading) {
            slot.fade += dt * kScannerMarkerFadeRate;
            if (slot.fade >= 1.0f) {
                slot.fade = 1.0f;
                slot.fading = false;
            }
        }
        const float dx = records[i].screen[0] - kScannerMarkerScreenHalf[0], dy = records[i].screen[1] - kScannerMarkerScreenHalf[1];
        const float distance = std::sqrt(dx * dx + dy * dy);
        float distanceFade = 1.0f;
        if (distance > kScannerMarkerFullDistance)
            distanceFade = 1.0f - std::min(1.0f, (distance - kScannerMarkerFullDistance) / kScannerMarkerFadeDistance);
        pose.shown = true;
        pose.screen[0] = records[i].screen[0];
        pose.screen[1] = records[i].screen[1];
        pose.opacity = slot.fade * distanceFade;
        if (slot.centreTime >= kScannerCentrePulsingSeconds) slot.centreTime = 0;
        pose.centreTime = slot.centreTime;
        slot.centreTime = std::min(kScannerCentrePulsingSeconds, slot.centreTime + dt);

        if (pulseIndex >= kScannerPulseSlots) continue;
        PulseSlot& pulse = pulses_[pulseIndex++];
        pose.pulsesShown = true;
        int finished = 0;
        for (std::size_t k = 0; k < 3; ++k)
            if (pulse.started && pulse.time[k] >= kScannerPulseSeconds) ++finished;
        bool start = !pulse.started;
        if (!start && finished == 3) {
            if (pulse.wait == 0.0f) {
                pulse.wait = kScannerPulseRestartWait;
            } else {
                pulse.wait -= dt;
                if (pulse.wait <= 0.0f) {
                    pulse.wait = 0;
                    start = true;
                }
            }
        }
        if (start) {
            startPulses(pulse);
            pulse.started = true;
        }
        for (std::size_t k = 0; k < 3; ++k) {
            pose.pulseTime[k] = pulse.time[k];
            pose.pulsePlaying[k] = pulse.started && pulse.delay[k] <= 0.0f;
            if (pulse.delay[k] > 0.0f)
                pulse.delay[k] -= dt;
            else if (pulse.time[k] < kScannerPulseSeconds)
                pulse.time[k] = std::min(kScannerPulseSeconds, pulse.time[k] + dt * (1.0f + kScannerPulseSpeedStep * float(k)));
        }
    }
    return poses_;
}

}
