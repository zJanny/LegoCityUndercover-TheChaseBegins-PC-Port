#pragma once
// The award viewer screen (docs/re/ui-screens.md): mode 0x514030 (vtable 0x362b40: enter 0x2f2184, draw 0x2f224c, update 0x2f26c8, leave 0x2f205c),
// set up by 0x1d2db4, with page 0x514058 (vtable 0x363a58: enter 0x304e20, leave request 0x304de8, A 0x304d5c, selection changed 0x304d70,
// B 0x304d2c). Top screen: the picture UI_Shop_Topscreen and the bronze, silver and gold counts; bottom screen: the round menu (round_menu.hpp)
// with the items General and the seven levels, and the Back shortcut. No SDL dependency.
// STAND-INS (listed in docs/plans/features/ui-menus-award-viewer.md):
// - UI-S12 The screen opens in the update after the native and closes when the round menu is idle; the mode fades 0x25d26c / 0x22b6d4 (0.5 s each),
//   the gate 0x375b08 bit 0x80 and the calls of the mode's enter and leave besides the counts are not reproduced. The saved selection lives as long
//   as the level session (the original's page object 0x514058 is static).
// - UI-S13 The top screen draws no award models (Prop_Award_Bronze, _Silver, _Gold with Prop_AwardShine, draw 0x1d3210); the counts use the font
//   frontendfont, white, centred on their point (the font state 0x255244 / 0x2551f8 is not read).
#include "frontend/pause_menu.hpp"
#include "frontend/round_menu.hpp"
#include "runtime/award_viewer.hpp"
#include <array>
#include <string>
#include <vector>

namespace chase::frontend {

constexpr const char* kAwardTopTexture = "sprites/ui_shop/ui_shop_topscreen.btga";  // 0x1d2db4: "Sprites/UI_Shop/UI_Shop_Topscreen.tga"
constexpr const char* kAwardItemFolder = "Sprites";
constexpr const char* kAwardGeneralImage = "UI_AwardViewer/UI_AwardViewer_General.png";
constexpr const char* kAwardPortraitFolder = "UI_LevelSelect/Portraits/";
constexpr std::uint32_t kAwardTitleKey = 0x90a614ef;    // gametext, round menu title (0x2206d8)
constexpr std::uint32_t kAwardGeneralKey = 0xc6103ad9;  // gametext, item 1
// TablesLevels sheet 0: the level's gametext key (row +8) and name (row +0x30).
constexpr unsigned kAwardLevelKeyColumn = 11, kAwardLevelNameColumn = 21;
// Draw 0x2f224c pass 6: "%u/%u" at (width / 2 - 114, 190), (width / 2, 190), (width / 2 + 114, 190).
constexpr float kAwardCountY = 190.0f, kAwardCountSpacing = 114.0f;
constexpr int kAwardSoundBack = 0x11;

struct AwardViewerSetup {
    PauseMenuSetup shared;  // round menu timings and centres, shortcut timings and the Back label
    std::string title;
    std::array<RoundMenuItem, runtime::kAwardViewerItems> items;
};
AwardViewerSetup awardViewerSetup(Resources& resources, const std::string& gameTextFile, const std::string& systemTextFile);

struct AwardViewerInput {
    bool left = false, right = false, a = false, b = false;
    TouchInput touch;
};

class AwardViewer {
public:
    explicit AwardViewer(const AwardViewerSetup& setup);
    // Enter 0x2f2184: the counts of item 1 (levels 2..8), the round menu created (0x2206f0) and the page requested (0x217f88).
    void open(const std::array<runtime::LevelAwardData, runtime::kAwardLevels>& levels);
    // Update 0x2f26c8: the round menu 0x1f08d4, the page manager 0x1f0754 with the page, the close once the round menu is idle.
    void update(float dt, const AwardViewerInput& input);
    bool isOpen() const { return opened_ && !finished_; }
    bool finished() const { return finished_; }
    bool pageShown() const { return page_; }
    const RoundMenu& round() const { return round_; }
    const runtime::AwardCounts& counts() const { return counts_; }
    std::vector<int> takeSounds();
    std::array<std::string, 3> countTexts() const;
    void drawTop(Canvas& top, Resources& resources) const;
    void drawBottom(Canvas& bottom, Resources& resources) const;

private:
    void enterPage();
    void requestClose();
    void recount();

    AwardViewerSetup setup_;
    RoundMenu round_;
    std::array<runtime::LevelAwardData, runtime::kAwardLevels> levels_{};
    runtime::AwardCounts counts_;
    bool opened_ = false, finished_ = false, closing_ = false;
    bool pageRequested_ = false, page_ = false, justSwitched_ = false;
    int savedSelection_ = 0;
    ShortcutState back_;
    std::vector<int> sounds_;
};

}
