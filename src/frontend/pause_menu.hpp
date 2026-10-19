#pragma once
// The pause screen of a level (docs/re/ui-screens.md): mode object 0x527358 (vtable 0x3636b0: enter 0x30004c, update 0x300378, draw 0x300204,
// leave 0x2fff00), set up by 0x263bd0, with page 0 of the status screen (vtable 0x363f34: enter 0x309a5c, leave request 0x309a18,
// A 0x309868, B 0x3097e8, update 0x309b74). Top screen: sprites/Blend/PauseTop/PauseTop; bottom screen: the round menu (round_menu.hpp)
// with the Back and Select shortcuts (0x2203ac). No SDL dependency.
// STAND-INS (listed in docs/plans/features/ui-menus.md):
// - UI-S4 Options, Credits and Exit play the select sound and do nothing else (pages 0x364888 and 0x51456c and the quit question are not ported).
// - UI-S5 The six collectible counts, the ability count and the ticker text of the top screen are not drawn (save progress 0x20a630 / 0x20a7fc,
//   0x239184 and the ticker 0x2ba8c8 are not ported); the ability icon is the one of case 0 (UI_Scanner_DistressedCitizen) and the
//   portrait border is portrait_border_cop (the class tables 0x51b24c, 0x519454 and 0x519474 are not read).
// - UI-S6 The page manager 0x376d6c is busy only while the screen closes; its other busy tests (0x25207c, 0x1ff080, 0x1fbedc, 0x272fcc,
//   0x214bf8) are not modelled.
#include "frontend/round_menu.hpp"
#include <string>
#include <vector>

namespace chase::frontend {

constexpr const char* kPauseTopLayout = "sprites/blend/pausetop/pausetop.bxaml";
constexpr const char* kPauseItemFolder = "Sprites/UI_StatusScreen";
constexpr const char* kShortcutBackLayout = "sprites/blend/ui_frontend/backbutton.bxaml";
constexpr const char* kShortcutSelectLayout = "sprites/blend/ui_frontend/selectbutton.bxaml";
// Text keys: page 0 items (0x309a5c, gametext), the round menu title (0x263bd0) and subtitle (0x1eca3c), the shortcut labels (0x2347d4).
constexpr std::uint32_t kPauseResumeKey = 0xa8caa86d, kPauseOptionsKey = 0xba538c9e, kPauseCreditsKey = 0xf63f20ba, kPauseExitKey = 0xb5f4aa1e;
constexpr std::uint32_t kPauseTitleKey = 0xc8c9ea6e, kPauseSubtitleKey = 0x1bb8e015;
constexpr std::uint32_t kShortcutSelectKey = 0x715c59fc;  // gametext
constexpr std::uint32_t kShortcutBackKey = 0x08be4916;    // systemtext
constexpr int kPauseSoundSelect = 0x14, kPauseSoundBack = 0x11;
constexpr int kPauseEntryCount = 4;

enum class PauseEntry { Resume = 0, Options = 1, Credits = 2, Exit = 3 };

struct PauseTexts {
    std::string items[kPauseEntryCount];
    std::string title, subtitle, select, back;
};

struct PauseMenuSetup {
    RoundMenuTimings round;
    std::array<std::array<float, 2>, 3> centres{};
    float topTransitionOn = 0, topTransitionOff = 0;
    float shortcutOn = 0, shortcutOff = 0;
    PauseTexts texts;
};
PauseMenuSetup pauseMenuSetup(Resources& resources, const std::string& gameTextFile, const std::string& systemTextFile);

struct PauseInput {
    bool start = false, left = false, right = false, a = false, b = false;
    TouchInput touch;
};

struct ShortcutState {
    bool shown = false;
    float time = 0;
};

class PauseMenu {
public:
    explicit PauseMenu(const PauseMenuSetup& setup);
    // 0x30004c: the top screen's TransitionOn, the round menu created (0x2206f0), page 0 requested (0x217f88).
    void open();
    // 0x300378 with the page manager 0x1f0754 and page 0.
    void update(float dt, const PauseInput& input);
    bool isOpen() const { return opened_ && !finished_; }
    // The screen has closed (0x2e5c60 is reached): the level goes on.
    bool finished() const { return finished_; }
    bool closing() const { return closing_; }
    bool pageShown() const { return page_; }
    const RoundMenu& round() const { return round_; }
    // Entries chosen whose effect is not ported (UI-S4), in order.
    std::vector<PauseEntry> takeUnported();
    std::vector<int> takeSounds();
    LayoutScene topScene() const;
    void drawTop(Canvas& top, Resources& resources) const;
    void drawBottom(Canvas& bottom, Resources& resources) const;

private:
    void enterPage();
    void requestClose();
    void pageSelect();

    PauseMenuSetup setup_;
    RoundMenu round_;
    bool opened_ = false, finished_ = false, closing_ = false;
    bool pageRequested_ = false, page_ = false, justSwitched_ = false;
    int savedSelection_ = 0;
    float topTime_ = 0, topOnTime_ = 0;  // since the last top-screen storyboard started, since TransitionOn started
    ShortcutState back_, select_;
    std::vector<PauseEntry> unported_;
    std::vector<int> sounds_;
};

}
