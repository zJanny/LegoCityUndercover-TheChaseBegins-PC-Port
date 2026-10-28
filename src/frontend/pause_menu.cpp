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
    setup.texts.optionsSubtitle = resources.textForKey(gameTextFile, runtime::kOptionsSubtitleKey);
    runtime::OptionsTexts& options = setup.texts.options;
    options.sound = resources.textForKey(gameTextFile, runtime::kOptionsSoundKey);
    options.music = resources.textForKey(gameTextFile, runtime::kOptionsMusicKey);
    options.invertCamera = resources.textForKey(gameTextFile, runtime::kOptionsInvertKey);
    options.yes = resources.textForKey(gameTextFile, runtime::kOptionsYesKey);
    options.no = resources.textForKey(gameTextFile, runtime::kOptionsNoKey);
    options.language = resources.textForKey(gameTextFile, runtime::kOptionsLanguageKey);
    return setup;
}

PauseMenu::PauseMenu(const PauseMenuSetup& setup) : setup_(setup), round_(setup.round, setup.centres) {
    save::SaveImage defaults;
    defaults.resetOptions(std::uint8_t(language_));
    std::copy(defaults.options(), defaults.options() + save::kOptionsSize, ownOptions_.begin());
}

void PauseMenu::bindOptions(std::uint8_t* options, runtime::OptionVolumes* volumes, int language) {
    boundOptions_ = options;
    boundVolumes_ = volumes;
    language_ = language;
}

std::uint8_t* PauseMenu::liveOptions() { return boundOptions_ ? boundOptions_ : ownOptions_.data(); }

runtime::OptionVolumes& PauseMenu::volumes() { return boundVolumes_ ? *boundVolumes_ : ownVolumes_; }

void PauseMenu::open() {
    opened_ = true;
    finished_ = closing_ = false;
    topTime_ = topOnTime_ = 0;
    ticker_.reset();
    tickerSeconds_ = 0;
    round_ = RoundMenu(setup_.round, setup_.centres);
    round_.create();
    round_.setTitle(setup_.texts.title);
    pageRequested_ = PausePage::Status;
    page_ = PausePage::None;
    justSwitched_ = false;
    saveRequested_ = false;
    back_ = select_ = {};
}

void PauseMenu::setShortcut(ShortcutState& shortcut, bool shown) {
    if (shortcut.shown != shown) shortcut = {shown, 0};
}

void PauseMenu::writeOptionTexts() {
    for (int row = 0; row < runtime::kOptionsRowCount; ++row)
        round_.setItemText(row, runtime::optionsRowText(row, setup_.texts.options, liveOptions(), volumes(), language_));
}

void PauseMenu::apply(const runtime::OptionsPageActions& actions) {
    for (const int id : actions.sounds) sounds_.push_back(id);
    if (actions.soundGain) gains_.sound = actions.soundGain;
    if (actions.musicGain) gains_.music = actions.musicGain;
    if (actions.backShortcut) setShortcut(back_, *actions.backShortcut);
    if (actions.selectShortcut) setShortcut(select_, *actions.selectShortcut);
    if (actions.closeRoundMenu) round_.close(false);
    if (actions.saveRequest) saveRequested_ = true;
    if (actions.textsChanged) writeOptionTexts();
    if (actions.page == runtime::OptionsPageRequest::Page0) requestPage(PausePage::Status);
    if (actions.page == runtime::OptionsPageRequest::Languages) unported_.push_back(PauseEntry::Languages);
    for (const int id : round_.takeSounds()) sounds_.push_back(id);
}

void PauseMenu::enterPage() {
    if (pageRequested_ == PausePage::Options) {
        std::vector<RoundMenuItem> items;
        for (const char* image : runtime::kOptionsItemImages) items.push_back({std::string(), image});
        round_.open(runtime::kOptionsItemFolder, items, options_.savedSelection());
        round_.setSubtitle(setup_.texts.optionsSubtitle);
        apply(options_.enter());
        apply(options_.selectionChanged(round_.selected()));
        return;
    }
    std::vector<RoundMenuItem> items;
    for (int i = 0; i < kPauseEntryCount; ++i) items.push_back({setup_.texts.items[i], kItemImages[i]});
    round_.open(kPauseItemFolder, items, savedSelection_);
    round_.setSubtitle(setup_.texts.subtitle);
    setShortcut(back_, true);
    setShortcut(select_, true);
}

void PauseMenu::requestPage(PausePage page) {
    pageRequested_ = page;
    if (page_ == PausePage::Options) {
        apply(options_.leaveRequest(liveOptions(), volumes()));
    } else if (page_ == PausePage::Status) {
        round_.close(closing_);
        setShortcut(back_, false);
        setShortcut(select_, false);
    }
}

void PauseMenu::requestClose() {
    closing_ = true;
    topTime_ = 0;
    requestPage(PausePage::None);
}

void PauseMenu::pageSelect() {
    sounds_.push_back(kPauseSoundSelect);
    const PauseEntry entry = PauseEntry(round_.selected());
    savedSelection_ = round_.selected();
    if (entry == PauseEntry::Resume) requestClose();
    else if (entry == PauseEntry::Options) requestPage(PausePage::Options);
    else unported_.push_back(entry);
}

void PauseMenu::update(float dt, const PauseInput& input) {
    if (!isOpen()) return;
    topTime_ += dt;
    tickerSeconds_ += dt;
    topOnTime_ += dt;
    back_.time += dt;
    select_.time += dt;
    if (input.left || input.right) {
        runtime::restoreSoundVolume(liveOptions(), volumes());
        gains_.sound = runtime::gainOf(volumes().sound);
    }
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
            if (page_ == PausePage::Options) {
                apply(options_.selectionChanged(round_.selected()));
                apply(options_.update(round_.selected(), input.up, input.down, liveOptions(), volumes()));
                if (result == 1) apply(options_.a(round_.selected(), liveOptions()));
                else if (result == -1) apply(options_.b());
            } else if (page_ == PausePage::Status && result == 1) {
                pageSelect();
            } else if (page_ == PausePage::Status && result == -1) {
                sounds_.push_back(kPauseSoundBack);
                requestClose();
            }
        } else if (round_.state() == RoundMenuState::Idle) {
            const PausePage entering = pageRequested_;
            if (entering != PausePage::None) enterPage();
            page_ = entering;
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

PauseGains PauseMenu::takeGains() {
    PauseGains out = gains_;
    gains_ = {};
    return out;
}

bool PauseMenu::takeSaveRequest() {
    const bool requested = saveRequested_;
    saveRequested_ = false;
    return requested;
}

void PauseMenu::saveDone() { apply(options_.saveDone()); }

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
    drawTicker(top, resources);
}

void PauseMenu::drawTicker(Canvas& top, Resources& resources) const {
    if (tickerText_.empty()) return;
    const BitmapFont& font = resources.font("frontendfont");
    const auto& extended = resources.extendedCharacters();
    ticker_.advance(tickerSeconds_, float(textWidth(font, extended, tickerText_, 0)));
    tickerSeconds_ = 0;
    const int clipX = int(runtime::kPauseTickerClip[0]), clipWidth = int(runtime::kPauseTickerClip[2]);
    Canvas strip(clipWidth, top.height());
    strip.clearTransparent();
    drawText(strip, font, extended, tickerText_, ticker_.x() - float(clipX), runtime::kPauseTickerY, Align::Left, Colour{1, 1, 1, 1}, 0);
    for (int y = 0; y < strip.height(); ++y)
        for (int x = 0; x < clipWidth; ++x) {
            const std::uint8_t* source = strip.pixel(x, y);
            if (source[3] == 0) continue;
            std::uint8_t* target = top.pixel(clipX + x, y);
            const float alpha = float(source[3]) / 255.0f;
            for (int c = 0; c < 3; ++c) target[c] = std::uint8_t(float(source[c]) * alpha + float(target[c]) * (1.0f - alpha) + 0.5f);
            target[3] = 255;
        }
}

void PauseMenu::drawBottom(Canvas& bottom, Resources& resources) const {
    bottom.clear(0, 0, 0);
    drawLayout(bottom, resources, kRoundMenuLayout, round_.scene(), 0, 0);
    ShortcutView back, select;
    back.label = setup_.texts.back;
    back.storyboard = back_.shown ? "TransitionOn" : "TransitionOff";
    back.time = back_.time;
    back.drawn = page_ != PausePage::None || closing_;
    select.label = setup_.texts.select;
    select.storyboard = select_.shown ? "TransitionOn" : "TransitionOff";
    select.time = select_.time;
    select.drawn = back.drawn;
    drawBackSelectShortcuts(bottom, resources, back, select);
}

}
