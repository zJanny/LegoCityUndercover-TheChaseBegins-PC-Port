#pragma once
// The round menu of the bottom screen: object 0x376014 with the layout sprites/UI_RoundaboutMenu/Main (docs/re/ui-screens.md).
// Create 0x2206f0, items 0x1ec7a0, open 0x1eca54, update 0x1f08d4, slot pictures 0x212c98, end of a move 0x270300, close 0x20eec4.
// Only the round mode (object +0xdd = 1, opened with a selection by 0x1eca54) that the pause menu uses is ported. No SDL dependency.
// STAND-IN UI-S1: a storyboard that is stopped (0x252828) keeps its last pose (the oracle picture after B shows no element that only the
// stopped TransitionOn hides), later started storyboards act on top of earlier ones, and one started with the frames 0xffff, 0xffff
// (0x245b40) is placed at its end; the storyboard player 0x245b40 / 0x252828 itself is not read (see the property evaluator, HUD-3).
#include "frontend/layout.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace chase::frontend {

constexpr const char* kRoundMenuLayout = "sprites/ui_roundaboutmenu/main.bxaml";
constexpr int kRoundMenuSlots = 5;                 // Button_1..Button_5; slot u shows item (selected + u - move - 2) mod count
constexpr float kRoundMenuTouchRadius = 32.0f;     // 0x228aac distance test against the centres of Button_2, Button_3, Button_4
constexpr unsigned kRoundMenuTapUpdates = 10;      // a touch released before 10 updates is a tap
constexpr unsigned kRoundMenuSwipeUpdates = 4;     // a swipe needs more than 4 touched updates,
constexpr unsigned kRoundMenuSwipeGapUpdates = 15; // 15 updates after the previous swipe
constexpr float kRoundMenuSwipeDistance = 30.0f;   // and a move of more than 30 pixels against the position 4 updates before
constexpr int kRoundMenuSoundMove = 0x12, kRoundMenuSoundClose = 0x11;

struct RoundMenuItem {
    std::string text;
    std::string image;  // file name in the item folder (0x1ec7a0 "%s/%s")
    bool enabled = true;
    bool isNew = false;
};

struct TouchInput {
    bool down = false;
    float x = 0, y = 0;  // bottom-screen pixels
};

struct RoundMenuInput {
    bool left = false, right = false;  // D-pad press edges (slots 2 and 3)
    bool a = false, b = false;         // press edges of slots 4 and 5
    TouchInput touch;
};

enum class RoundMenuState : std::uint8_t { Idle = 0, Active = 1, Opening = 2, Closing = 3 };

// Lengths of the storyboards the round mode plays (the layout's data).
struct RoundMenuTimings {
    float transitionOn = 0, transitionOnRound = 0, transitionOffRound = 0, transitionOff = 0, leftMove = 0, rightMove = 0, loop = 0;
};
RoundMenuTimings roundMenuTimings(const assets::UiLayout& layout);
// Bottom-screen centres of Button_2, Button_3 and Button_4 (object +0xf4, +0xfc, +0x104: written from the element positions every update
// while nothing touches the screen).
std::array<std::array<float, 2>, 3> roundMenuButtonCentres(const assets::UiLayout& layout);

class RoundMenu {
public:
    RoundMenu(const RoundMenuTimings& timings, const std::array<std::array<float, 2>, 3>& centres);

    // 0x2206f0 with the shown flag: 0x24581c shows the layout with its TransitionOn placed at its end (UI-S1).
    void create();
    // 0x1ec7a0 then 0x1eca54 (round mode): the items, then TransitionOn_Round once TransitionOn has ended.
    void open(const std::string& folder, const std::vector<RoundMenuItem>& items, int selected);
    // 0x20eec4: TransitionOff_Round (with the close sound when the menu was open or opening); `transitionOff` also starts TransitionOff.
    void close(bool transitionOff);
    // 0x1f08d4: 1 = select, -1 = back, 0 = nothing.
    int update(float dt, const RoundMenuInput& input);

    void setTitle(const std::string& text) { title_ = text; }
    void setSubtitle(const std::string& text) { subtitle_ = text; }
    RoundMenuState state() const { return state_; }
    int selected() const { return selected_; }
    int move() const { return move_; }
    int itemAt(int slot) const;
    const std::vector<RoundMenuItem>& items() const { return items_; }
    std::vector<int> takeSounds();
    LayoutScene scene() const;

private:
    struct Playing {
        std::string name;
        float time = 0;
        float length = 0;
        bool loop = false;
        bool running = true;
    };
    void start(const std::string& name, float length, bool loop = false, bool atEnd = false);
    void stop(const std::string& name);
    bool finished(const std::string& name) const;
    void refreshTexts();
    void endMove();
    int touchDirection(const TouchInput& touch, bool& select, bool buttonsPressed);
    static float distance(const std::array<float, 2>& a, const std::array<float, 2>& b);

    RoundMenuTimings timings_;
    std::array<std::array<float, 2>, 3> centres_;
    std::vector<Playing> boards_;
    std::vector<RoundMenuItem> items_;
    std::string folder_, title_, subtitle_, iconText_, iconOutText_;
    RoundMenuState state_ = RoundMenuState::Idle;
    bool shown_ = false, transitionOnPending_ = false;
    int selected_ = 0, move_ = 0;
    unsigned touchUpdates_ = 0, swipeUpdate_ = 0;
    std::array<std::array<float, 2>, 5> touchHistory_{};
    std::vector<int> sounds_;
};

}
