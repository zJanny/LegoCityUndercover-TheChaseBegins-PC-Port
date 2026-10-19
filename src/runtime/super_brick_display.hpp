#pragma once
// The super brick count display of the HUD: the part of the HUD draw callback 0x193604 that starts at 0x2a5f80 (the number and the spinning brick
// icon), the fader 0x5198b0 (generic fade object of 0x23cb90 / 0x23cc00 / 0x2459d8 / 0x235e2c, draw callback 0x27b3e4) and the fields at
// 0x51920c + 0x2a0..0x2ac. Docs: docs/re/super-bricks.md. No SDL dependency.
//
// A pickup shows the display (0x2144f8: stamp = the update count, show the fader with hold -1). The display lasts 80 updates from the stamp; the
// destroy of the brick changes the shown number to the new count (0x262778) and restarts the number's bounce. After the 80 updates the stamp is cleared
// and the next draw hides the fader, which fades out in 0.1 s.
#include "runtime/camera_lock_icon.hpp"

#include <cstdint>

namespace chase::runtime {

constexpr float kSuperBrickFadeIn = 0.25f;           // 0x290b1c: 0x1fa43c(0.25, 0.1, ...) stores the fade-in +0x14 and fade-out +0x18 of the fader
constexpr float kSuperBrickFadeOut = 0.1f;
constexpr unsigned kSuperBrickDisplayUpdates = 80;   // 0x2a60dc: the update count against stamp + 0x50
constexpr unsigned kSuperBrickBounceUpdates = 5;     // 0x2a6140: stamp B + 5
constexpr float kSuperBrickSlide = 32.0f;            // 0x27b3e4 and 0x2a6180: 32 x the fader's fade value
constexpr float kSuperBrickAnchorX = 186.0f;         // sink 2 of 0x23ca00, set at 0x2cee0c: (0x3584d8 = 186.0, 21.0, 5.5)
constexpr float kSuperBrickAnchorY = 21.0f;
constexpr float kSuperBrickIconOffsetY = 4.0f;       // 0x2a60ec: the icon position is the anchor + (0, 4)
constexpr float kSuperBrickIconSlideBase = -28.0f;   // 0x27b3e4: y + 32 alpha - 28
constexpr float kSuperBrickTextOffsetX = 16.0f;      // 0x2a6130: the number at the anchor + (16, -10), 32 above it before the slide
constexpr float kSuperBrickTextOffsetY = -10.0f;
constexpr float kSuperBrickIconSpinPerSecond = 3.14159265f;  // 0x2a6038: the spin angle +0x38 grows by dt * pi and wraps above 2 pi
constexpr float kSuperBrickIconTilt = 1.4f;              // 0x27b3e4: 0x2354b4(1.4) after the spin rotation 0x237bc4(-spin)
constexpr std::uint32_t kSuperBrickIconTint = 0xffdd9744;  // 0x290b1c: 0x23e110(hud_platebrickmodel, 0xffdd9744, 0xffffffff, 1), ABGR
constexpr std::uint32_t kSuperBrickTextColour = 0xff00ffff;  // 0x2a61b4: the colour argument of the number, ABGR (yellow)

struct SuperBrickDisplayView {
    bool iconVisible = false;  // the fader is not idle: the icon is drawn (0x27b3e4)
    float iconAlpha = 0;       // the fader's fade value, it only moves the icon (it is not a blend alpha)
    float iconX = 0, iconY = 0;
    float spin = 0;            // the angle at 0x375890, 0..2 pi
    bool textVisible = false;
    float textX = 0, textY = 0;
    float textScale = 1;
    std::uint32_t count = 0;
};

class SuperBrickDisplay {
public:
    // The count the display shows before any pickup (the u32 at 0x51920c + 0x2a8, which the port takes from the super brick count).
    void setCount(std::uint32_t count) { count_ = count; }
    // 0x2144f8: stamp +0x2a0 = the update count, the fader is shown with hold -1 (0x23cc00).
    void show();
    // 0x262778 at the destroy: +0x2a8 = the new count, +0x2a4 = the update count (the bounce).
    void banked(std::uint32_t total);
    // One game update (the draw callback runs once per update): spin, expiry, fader.
    void update(float dt);
    SuperBrickDisplayView view() const;
    std::uint32_t count() const { return count_; }
    unsigned updates() const { return updates_; }

private:
    // The fader 0x5198b0: the state machine of 0x23cb90 / 0x23cc00 / 0x2459d8 / 0x235e2c shared with the camera lock icon and the head icon.
    CameraLockIcon fader_{kSuperBrickFadeIn, kSuperBrickFadeOut, -1.0f};

    unsigned updates_ = 1000;  // 0x241098 is far above 0 in play; a stamp of 0 means no display
    unsigned stamp_ = 0, bounceStamp_ = 0;
    std::uint32_t count_ = 0;
    float spin_ = 0;
    float usedAlpha_ = 0;
    float textAlpha_ = 0;  // 0x375898: the fader's value as of its last draw, used by the number of the next draw callback
};

}
