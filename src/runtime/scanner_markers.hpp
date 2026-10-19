#pragma once
// The per-object markers of the audio scanner view (docs/re/abilities-scanner.md): every scannable on screen gets a SoundCentre
// pip and one of 15 triplets of SoundPulse rings. Rules from the reticle update 0x2e9030. No SDL dependency.
// STAND-INS (SCAN-S7): the start delay of pulse k is 3 * k updates of 1/30 s (the unit of the delay argument of 0x245b40 is not
// read), the marker slot of a record is its index in the record list (the original sorts with 0x26eec4), and the centre pip starts
// at time 0 when its marker is first shown.
#include <array>
#include <cstddef>
#include <vector>

namespace chase::runtime {

constexpr std::size_t kScannerMarkerSlots = 50;
constexpr std::size_t kScannerPulseSlots = 15;
constexpr float kScannerMarkerScreenHalf[2] = {200.0f, 120.0f};
constexpr float kScannerMarkerFadeRate = 2.0f;
constexpr float kScannerMarkerFullDistance = 20.0f;
constexpr float kScannerMarkerFadeDistance = 180.0f;
constexpr float kScannerPulseRestartWait = 0.3f;
constexpr float kScannerCentrePulsingSeconds = 0.459f;
constexpr float kScannerPulseSeconds = 1.1f;
constexpr float kScannerPulseSpeedStep = 0.2f;
constexpr float kScannerPulseDelayUpdates = 3.0f;
constexpr float kScannerUpdateSeconds = 1.0f / 30.0f;

struct ScannerMarkerInput {
    bool visible = false;
    float screen[2] = {0, 0};
};

struct ScannerMarkerPose {
    bool shown = false;
    float screen[2] = {0, 0};
    float opacity = 0;
    float centreTime = 0;
    bool pulsesShown = false;
    std::array<float, 3> pulseTime{};
    std::array<bool, 3> pulsePlaying{};
};

class ScannerMarkers {
public:
    const std::vector<ScannerMarkerPose>& update(float dt, const std::vector<ScannerMarkerInput>& records);
    void reset();

private:
    struct Slot {
        bool shown = false, fading = false;
        float fade = 0, centreTime = 0;
    };
    struct PulseSlot {
        bool started = false;
        float wait = 0;
        std::array<float, 3> time{}, delay{};
    };
    void startPulses(PulseSlot& slot) const;

    std::array<Slot, kScannerMarkerSlots> slots_;
    std::array<PulseSlot, kScannerPulseSlots> pulses_;
    std::vector<ScannerMarkerPose> poses_;
};

}
