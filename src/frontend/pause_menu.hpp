#pragma once
// The pause screen of a level (docs/re/ui-screens.md): mode object 0x527358 (vtable 0x3636b0: enter 0x30004c, update 0x300378, draw 0x300204,
// leave 0x2fff00), set up by 0x263bd0, with page 0 of the status screen (vtable 0x363f34: enter 0x309a5c, leave request 0x309a18,
// A 0x309868, B 0x3097e8, update 0x309b74) and the Options page (vtable 0x364888, runtime/options_page.hpp). Top screen: sprites/Blend/PauseTop/PauseTop; bottom screen: the round menu (round_menu.hpp)
// with the Back and Select shortcuts (0x2203ac). No SDL dependency.
// STAND-INS (listed in docs/plans/features/ui-menus.md):
// - UI-S4 Credits and Exit play the select sound and do nothing else (page 0x51456c and the quit question are not ported).
// - UI-S10 The save request of the Options page (task 0x1c34a4) is done by the caller at once (takeSaveRequest, then saveDone).
// - UI-S11 A on the Language row of the Options page plays the select sound and stays on the page (the Languages page 0x364a68 is not
//   ported).
// - UI-S5 The six collectible counts, the ability count and the ticker text of the top screen are not drawn (save progress 0x20a630 / 0x20a7fc,
//   0x239184 are not ported; the ticker font, line top and spacing are stand-ins, see runtime/pause_ticker.hpp); the ability icon is the one of case 0 (UI_Scanner_DistressedCitizen) and the
//   portrait border is portrait_border_cop (the class tables 0x51b24c, 0x519454 and 0x519474 are not read).
// - UI-S6 The page manager 0x376d6c is busy only while the screen closes; its other busy tests (0x25207c, 0x1ff080, 0x1fbedc, 0x272fcc,
//   0x214bf8) are not modelled.
#include "frontend/round_menu.hpp"
#include "runtime/options_page.hpp"
#include "runtime/pause_ticker.hpp"
#include "save/save.hpp"
#include <array>
#include <optional>
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

enum class PauseEntry { Resume = 0, Options = 1, Credits = 2, Exit = 3, Languages = 4 };
// Pages of the pause object (+0x340 page 0, +0x348 Options); None while the screen closes.
enum class PausePage { None, Status, Options };

struct PauseTexts {
    std::string items[kPauseEntryCount];
    std::string title, subtitle, select, back;
    std::string optionsSubtitle;
    runtime::OptionsTexts options;
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
    bool start = false, left = false, right = false, up = false, down = false, a = false, b = false;
    TouchInput touch;
};

struct ShortcutState {
    bool shown = false;
    float time = 0;
};

// The gains the pause screen set (0x1f4408 sound effects, 0x1f36ec music), the last of each since the previous take.
struct PauseGains {
    std::optional<float> sound, music;
};

class PauseMenu {
public:
    explicit PauseMenu(const PauseMenuSetup& setup);
    // The live options block (kOptionsSize bytes, 0x51b490), the live volume bytes (0x37751d / 0x37751e) and the language index the
    // Options page edits and shows. Without a binding the menu uses its own block with the reset defaults of 0x2081cc.
    void bindOptions(std::uint8_t* options, runtime::OptionVolumes* volumes, int language);
    // 0x30004c: the top screen's TransitionOn, the round menu created (0x2206f0), page 0 requested (0x217f88).
    void open();
    // 0x300378 with the page manager 0x1f0754 and page 0.
    void update(float dt, const PauseInput& input);
    bool isOpen() const { return opened_ && !finished_; }
    // The screen has closed (0x2e5c60 is reached): the level goes on.
    bool finished() const { return finished_; }
    bool closing() const { return closing_; }
    bool pageShown() const { return page_ != PausePage::None; }
    PausePage page() const { return page_; }
    bool backShown() const { return back_.shown; }
    bool selectShown() const { return select_.shown; }
    const runtime::OptionsPage& optionsPage() const { return options_; }
    const std::uint8_t* options() const { return boundOptions_ ? boundOptions_ : ownOptions_.data(); }
    const runtime::OptionVolumes& optionVolumes() const { return boundVolumes_ ? *boundVolumes_ : ownVolumes_; }
    PauseGains takeGains();
    // The Options page asked for its save (B after a change); the caller stores the options and calls saveDone (callback 0x26435c).
    bool takeSaveRequest();
    void saveDone();
    const RoundMenu& round() const { return round_; }
    // Entries chosen whose effect is not ported (UI-S4), in order.
    std::vector<PauseEntry> takeUnported();
    std::vector<int> takeSounds();
    // The ticker text of draw 0x29ec00; empty while no mission title is set.
    void setTickerText(const std::string& text) { tickerText_ = text; }
    LayoutScene topScene() const;
    void drawTop(Canvas& top, Resources& resources) const;
    void drawBottom(Canvas& bottom, Resources& resources) const;

private:
    void drawTicker(Canvas& top, Resources& resources) const;
    void enterPage();
    void requestPage(PausePage page);
    void requestClose();
    void pageSelect();
    void apply(const runtime::OptionsPageActions& actions);
    void writeOptionTexts();
    void setShortcut(ShortcutState& shortcut, bool shown);
    std::uint8_t* liveOptions();
    runtime::OptionVolumes& volumes();

    PauseMenuSetup setup_;
    RoundMenu round_;
    bool opened_ = false, finished_ = false, closing_ = false;
    PausePage pageRequested_ = PausePage::None, page_ = PausePage::None;
    bool justSwitched_ = false;
    int savedSelection_ = 0;
    runtime::OptionsPage options_;
    std::array<std::uint8_t, save::kOptionsSize> ownOptions_{};
    runtime::OptionVolumes ownVolumes_;
    std::uint8_t* boundOptions_ = nullptr;
    runtime::OptionVolumes* boundVolumes_ = nullptr;
    int language_ = 1;
    PauseGains gains_;
    bool saveRequested_ = false;
    std::string tickerText_;
    mutable runtime::PauseTicker ticker_;
    mutable float tickerSeconds_ = 0;
    float topTime_ = 0, topOnTime_ = 0;  // since the last top-screen storyboard started, since TransitionOn started
    ShortcutState back_, select_;
    std::vector<PauseEntry> unported_;
    std::vector<int> sounds_;
};

}
