#pragma once
// The two info boxes (info_top, info_bot) of the audio scanner view (docs/re/abilities-scanner-overlay.md): state machine 0x2d2e14, top / bottom
// choice 0x1f3494, icon choice 0x2ad730. No SDL dependency.
// STAND-INS (SCAN-S11, SCAN-S12): the box offset is the record's screen position minus the screen centre in pixels (measured from an oracle
// picture, the units of the original's +0x14 / +0x18 are not read); the storyboards InfoOn / InfoOff advance with the scanner update and
// last 0.7 s / 0.2 s (layout data); the icon byte is the class value InfoIcon; the previous scan state (+0x29) is the state seen by the
// previous call; ability bit 28 (0x23a87c) is treated as absent; outside scan state 6 the box keeps a copy of the last record (the original keeps
// a pointer into the record list and follows it).
#include <cstdint>

namespace chase::runtime {

constexpr float kScannerInfoScreenHeight = 240.0f;
constexpr float kScannerInfoSwitchLine = kScannerInfoScreenHeight * 0.5f - 15.0f;
constexpr float kScannerInfoHysteresis = 20.0f;
constexpr float kScannerInfoIdleHysteresis = 1000.0f;
constexpr float kScannerInfoNearSquared = 625.0f;
constexpr float kScannerInfoFadeSquared = 2975.0f;
constexpr float kScannerInfoOnSeconds = 0.7f;
constexpr float kScannerInfoOffSeconds = 0.2f;
constexpr int kScannerInfoAlienIcon = 5;
constexpr float kScannerInfoCentre[2] = {200.0f, 120.0f};

struct ScannerInfoRecord {
    float screen[2] = {0, 0};
    float distanceSquared = 0;
    int icon = 0;
};

struct ScannerInfoInput {
    int state = 0;
    const ScannerInfoRecord* current = nullptr;
    const ScannerInfoRecord* target = nullptr;
    float reticleY = kScannerInfoCentre[1];
    float dt = 1.0f / 30.0f;
};

enum class ScannerInfoBox { None, Top, Bottom };

struct ScannerInfoPose {
    ScannerInfoBox box = ScannerInfoBox::None;
    bool leaving = false;
    float time = 0;
    float opacity = 0;
    float offset[2] = {0, 0};
    int icon = 0;
};

int scannerInfoIcon(int iconByte, bool hasAbility);

class ScannerInfo {
public:
    ScannerInfoPose update(const ScannerInfoInput& input);
    void reset();
    int phase() const { return phase_; }

private:
    void select(float reticleY, int state);

    int phase_ = 0;
    int previousState_ = 0;
    int placement_ = 0;
    bool hasRecord_ = false;
    ScannerInfoRecord record_;
    float opacity_ = 0;
    float time_ = 0;
    bool playing_ = false;
    bool leaving_ = false;
    ScannerInfoBox box_ = ScannerInfoBox::None;
    int icon_ = 0;
};

}
