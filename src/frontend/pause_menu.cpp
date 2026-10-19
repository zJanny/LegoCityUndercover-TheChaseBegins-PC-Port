#include "frontend/pause_menu.hpp"
#include "frontend/pages.hpp"
#include <algorithm>

namespace chase::frontend {
namespace {

constexpr const char* kItemImages[kPauseEntryCount] = {"UI_StatusScreen_Resume.png", "UI_StatusScreen_Options.png", "UI_StatusScreen_Credits.png",
                                                       "UI_StatusScreen_Exit.png"};
// 0x263bd0: the six item icons (0x225520 on item +0x48), in item order.
constexpr const char* kItemIcons[6] = {
    "sprites/blend/drc_scanner/images/icons/ui_scanner_redbrick.bpng",       "sprites/blend/drc_scanner/images/icons/ui_scanner_charactertoken.bpng",
    "sprites/blend/drc_scanner/images/icons/ui_scanner_vehicletoken.bpng",   "sprites/blend/drc_scanner/images/icons/ui_scanner_postcardpoint.bpng",
    "sprites/ui_statusscreen/progressicons/ui_progressicons_superbuildits.bpng", "sprites/ui_statusscreen/progressicons/ui_progressicons_ramps.bpng"};
// STAND-IN UI-S5: case 0 of 0x2adadc and the cop border.
constexpr const char* kAbilityIcon = "sprites/blend/drc_scanner/images/icons/ui_scanner_distressedcitizen.bpng";
constexpr const char* kPortraitBorder = "sprites/blend/pausetop/images/portrait_border_cop.bpng";

float storyboardLength(const assets::UiLayout& layout, const std::string& name) {
    float length = 0;
    for (const assets::UiStoryboard& board : layout.storyboards)
        if (board.name == name)
            for (const assets::UiTrack& track : board.tracks)
                length = std::max(length, track.repeat == 0 ? track.duration : track.duration * float(track.repeat));
    return length;
}

}

PauseMenuSetup pauseMenuSetup(Resources& resources, const std::string& gameTextFile, const std::string& systemTextFile) {
    PauseMenuSetup setup;
    const assets::UiLayout& round = resources.layout(kRoundMenuLayout);
    setup.round = roundMenuTimings(round);
    setup.centres = roundMenuButtonCentres(round);
    const assets::UiLayout& top = resources.layout(kPauseTopLayout);
    setup.topTransitionOn = storyboardLength(top, "TransitionOn");
    setup.topTransitionOff = storyboardLength(top, "TransitionOff");
    const assets::UiLayout& button = resources.layout(kShortcutBackLayout);
    setup.shortcutOn = storyboardLength(button, "TransitionOn");
    setup.shortcutOff = storyboardLength(button, "TransitionOff");
    const std::uint32_t keys[kPauseEntryCount] = {kPauseResumeKey, kPauseOptionsKey, kPauseCreditsKey, kPauseExitKey};
    for (int i = 0; i < kPauseEntryCount; ++i) setup.texts.items[i] = resources.textForKey(gameTextFile, keys[i]);
    setup.texts.title = resources.textForKey(gameTextFile, kPauseTitleKey);
    setup.texts.subtitle = resources.textForKey(gameTextFile, kPauseSubtitleKey);
    setup.texts.select = resources.textForKey(gameTextFile, kShortcutSelectKey);
    setup.texts.back = resources.textForKey(systemTextFile, kShortcutBackKey);
    return setup;
}

PauseMenu::PauseMenu(const PauseMenuSetup& setup) : setup_(setup), round_(setup.round, setup.centres) {}

void PauseMenu::open() {
    opened_ = true;
    finished_ = closing_ = false;
    topTime_ = topOnTime_ = 0;
    round_ = RoundMenu(setup_.round, setup_.centres);
    round_.create();
    round_.setTitle(setup_.texts.title);
    pageRequested_ = true;
    page_ = justSwitched_ = false;
    back_ = select_ = {};
}

void PauseMenu::enterPage() {
    std::vector<RoundMenuItem> items;
    for (int i = 0; i < kPauseEntryCount; ++i) items.push_back({setup_.texts.items[i], kItemImages[i]});
    round_.open(kPauseItemFolder, items, savedSelection_);
    round_.setSubtitle(setup_.texts.subtitle);
    back_ = {true, 0};
    select_ = {true, 0};
}

void PauseMenu::requestClose() {
    closing_ = true;
    topTime_ = 0;
    pageRequested_ = false;
    if (page_) {
        round_.close(true);
        back_ = {false, 0};
        select_ = {false, 0};
    }
}

void PauseMenu::pageSelect() {
    sounds_.push_back(kPauseSoundSelect);
    const PauseEntry entry = PauseEntry(round_.selected());
    if (entry == PauseEntry::Resume) requestClose();
    else unported_.push_back(entry);
    savedSelection_ = round_.selected();
}

void PauseMenu::update(float dt, const PauseInput& input) {
    if (!isOpen()) return;
    topTime_ += dt;
    topOnTime_ += dt;
    back_.time += dt;
    select_.time += dt;
    RoundMenuInput menuInput;
    menuInput.left = input.left;
    menuInput.right = input.right;
    menuInput.a = input.a;
    menuInput.b = input.b;
    menuInput.touch = input.touch;
    const int result = round_.update(dt, menuInput);
    for (const int id : round_.takeSounds()) sounds_.push_back(id);
    const bool busy = closing_;
    if (!busy) {
        if (justSwitched_) {
            justSwitched_ = false;
        } else if (page_ == pageRequested_) {
            if (page_ && result == 1) {
                pageSelect();
            } else if (page_ && result == -1) {
                sounds_.push_back(kPauseSoundBack);
                requestClose();
            }
        } else if (round_.state() == RoundMenuState::Idle) {
            if (pageRequested_) enterPage();
            page_ = pageRequested_;
            justSwitched_ = true;
        }
    }
    for (const int id : round_.takeSounds()) sounds_.push_back(id);
    if (closing_ && round_.state() == RoundMenuState::Idle && topTime_ >= setup_.topTransitionOff) {
        finished_ = true;
        return;
    }
    if (!closing_ && input.start) {
        requestClose();
        for (const int id : round_.takeSounds()) sounds_.push_back(id);
    }
}

std::vector<PauseEntry> PauseMenu::takeUnported() {
    std::vector<PauseEntry> out;
    out.swap(unported_);
    return out;
}

std::vector<int> PauseMenu::takeSounds() {
    std::vector<int> out;
    out.swap(sounds_);
    return out;
}

LayoutScene PauseMenu::topScene() const {
    LayoutScene scene;
    scene.storyboard = "TransitionOn";
    scene.time = topOnTime_;
    if (closing_) scene.laterStoryboards.push_back({"TransitionOff", topTime_});
    for (int i = 0; i < 6; ++i) {
        const std::string item = "genericItem_" + std::to_string(i);
        scene.images[item + "/icon"] = kItemIcons[i];
        scene.hidden.insert(item + "/icon_Locked");
    }
    scene.images["abilityItem_0/icon"] = kAbilityIcon;
    scene.hidden.insert("abilityItem_0/icon_Locked");
    scene.images["portrait/border"] = kPortraitBorder;
    scene.hidden.insert("portrait/icon_Locked");
    return scene;
}

void PauseMenu::drawTop(Canvas& top, Resources& resources) const {
    top.clear(0, 0, 0);
    drawLayout(top, resources, kPauseTopLayout, topScene(), 0, 0);
}

void PauseMenu::drawBottom(Canvas& bottom, Resources& resources) const {
    bottom.clear(0, 0, 0);
    drawLayout(bottom, resources, kRoundMenuLayout, round_.scene(), 0, 0);
    ShortcutView back, select;
    back.label = setup_.texts.back;
    back.storyboard = back_.shown ? "TransitionOn" : "TransitionOff";
    back.time = back_.time;
    back.drawn = page_ || closing_;
    select.label = setup_.texts.select;
    select.storyboard = select_.shown ? "TransitionOn" : "TransitionOff";
    select.time = select_.time;
    select.drawn = back.drawn;
    drawBackSelectShortcuts(bottom, resources, back, select);
}

}
