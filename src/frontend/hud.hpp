#pragma once
// In-game HUD of the top screen (docs/re/hud.md). No SDL dependency.
// Evidence: page init 0x2cedb8 / 0x17bb14, draw callback 0x193604, heart switch 0x2316b4, stud bar 0x15b2f0 / 0x166e08 /
// 0x22ddf4 / 0x240fc0.
#include "frontend/canvas.hpp"
#include "frontend/layout.hpp"
#include "frontend/resources.hpp"
#include <array>
#include <cstdint>
#include <string>

namespace chase::frontend {

constexpr const char* kHudLayout = "sprites/ui_hud/blend/main.bxaml";
constexpr const char* kHeartControlLayout = "sprites/ui_hud/blend/images/ui_hearts_control.bxaml";
constexpr const char* kStudSheet = "sprites/commonlego/pickups_commonstud.btga";
constexpr const char* kCameraLockedSprite = "sprites/ui_hud/ui_cameralocked.btga";
constexpr int kHudHeartSlots = 6;
constexpr float kStudBarHoldSeconds = 2.0f;   // argument of 0x240fc0 in 0x22ddf4
constexpr unsigned kStudBounceUpdates = 5;    // 0x166e08 tests `update count - 5 < change`
constexpr float kStudSinkDepth = 5.5f;        // third argument of 0x23ca00 (sink 0)

enum class StudBarState : std::uint8_t { Hidden, TurningOn, Shown, TurningOff };

struct HudConfig {
    // sprites/portraits/<portrait>.bpng: name column of tablescharacters for the player's row (9: "police").
    std::string portrait = "police";
    // heartsBG_<background>: 0x193604 picks it from the character's class flags (0x19 = cop).
    // STAND-IN: the flag test is not replicated, the caller names the background.
    std::string heartsBackground = "cop";
    // Class attribute "Health", default 4 (0x236e44); the +1 of unlock 17 (0x236e6c) is not replicated.
    unsigned hearts = 4;
};

struct StudSink {
    float x = 0, y = 0;
};

class Hud {
public:
    Hud(Resources& resources, const HudConfig& config);

    void update(float dt);
    // 0x2316b4: `vehicleImage` picks UI_HeartVehicle_3DS (the player's state is 0x11a or 0x11c) for all six hearts.
    void setHearts(unsigned hearts, bool vehicleImage = false);
    // 0x193604 while the player is in a vehicle: the portrait is the vehicle's (loaded by 0x262188) on heartsBG_generic. An empty path
    // returns to the character's portrait and background.
    void setVehiclePortrait(const std::string& path) { vehiclePortrait_ = path; }
    // 0x22ddf4 via the pickup hook 0x29c090: the counter now shows `total` with the icon of stud type 0..3.
    void studsBanked(unsigned type, std::uint64_t total);
    // STAND-IN: `spinSeconds`, the clock of the icon animation (15 frames per second), has the same unrecovered origin as
    // the studs in the world.
    // STAND-IN: the page's TransitionOn storyboard starts when the Hud is created (the original starts it when 0x17c0f4
    // shows the HUD, the trigger is not traced).
    void draw(Canvas& top, double spinSeconds);

    // Sink 0 of 0x23ca00: the centre of Studs_Count until the bar is drawn, then the centre of the icon.
    StudSink studSink() const { return sink_; }
    StudBarState studBarState() const { return barState_; }
    unsigned hearts() const { return hearts_; }
    // 0x1b1150: the crossed-out camera icon, alpha 0 hides it.
    void setCameraLockAlpha(float alpha) { cameraLockAlpha_ = alpha; }
    std::uint64_t studDisplay() const { return studValue_; }
    float studIconSize() const { return barLayout().iconSize; }

private:
    enum class HeartBoard : std::uint8_t { None, Loop, Lose, Gain };
    struct Heart {
        HeartBoard board = HeartBoard::None;
        float time = 0;
    };
    struct BarLayout {
        float centreX, topY;
        float iconX, iconY, iconSize;
    };

    float boardLength(const std::string& layoutPath, const std::string& storyboard);
    float pageOffsetY() const;
    BarLayout barLayout() const;
    float bounce() const;
    void startBar(float hold);
    void drawStudBar(Canvas& top, double spinSeconds);

    Resources& resources_;
    HudConfig config_;
    float seconds_ = 0;
    float cameraLockAlpha_ = 0;
    unsigned updates_ = 0;
    unsigned hearts_ = kHudHeartSlots;
    bool vehicleHearts_ = false;
    std::string vehiclePortrait_;
    std::array<Heart, kHudHeartSlots> heart_{};
    StudBarState barState_ = StudBarState::Hidden;
    float barStart_ = 0, barHold_ = kStudBarHoldSeconds;
    float onLength_ = 0, offLength_ = 0;
    unsigned studType_ = 0;
    std::uint64_t studValue_ = 0;
    unsigned changeUpdate_ = 0;
    float baseX_ = 0, baseY_ = 0, width_ = 0, height_ = 0;
    StudSink sink_;
};

}
