#include "frontend/flow.hpp"
#include <algorithm>

namespace chase::frontend {

namespace {

// STAND-IN: the boot screen (row 0, 0x312edc) waits for an asynchronous load whose length depends on the console;
// these are the durations of its two pages on the PC port (docs/re/front-end.md).
constexpr float kGameCardPageSeconds = 1.0f;
constexpr float kSavePageSeconds = 1.5f;
constexpr float kBootFadeSeconds = 0.5f;
constexpr float kBootSeconds = kGameCardPageSeconds + kSavePageSeconds + kBootFadeSeconds;
// Select_Slot1/2 of page_selectslot last 0.3 s (docs/re/ui-layouts.md dump of the storyboards).
constexpr float kSlotConfirmSeconds = 0.3f;
// STAND-IN: the oracle shows the emptied slot page for a moment before the saving page (docs/re/front-end.md); its length
// was not measured.
constexpr float kSlotClearedSeconds = 0.5f;
// STAND-INS: the original shows these screens while it loads asynchronously or while the in-engine cutscene runs.
constexpr float kSavingSeconds = 2.0f;
constexpr float kLoadingSeconds = 4.0f;
constexpr float kMissionCardSeconds = 6.0f;
// STAND-IN: the save indicator (0x24004c state 3) turns off after the OK press; the length of its transition is not read.
constexpr float kSaveIndicatorOffSeconds = 0.3f;
constexpr std::uint32_t kSkipButtons = kButtonA | kButtonB | kButtonX | kButtonY | kButtonStart;

}

std::string localisationSuffix(int language) {
    switch (language) {
    case 2: return "fr";
    case 3: return "it";
    case 4: return "de";
    case 5: return "es";
    case 6: return "nl";
    case 7: return "da";
    case 8: return "po";
    case 9: return "ru";
    default: return "uk";
    }
}

Flow::Flow(MovieHost* movies, FlowConfig config) : movies_(movies), config_(config), language_(config.language) {
    start(Screen::Boot);
}

void Flow::start(Screen screen) {
    handoff_ = {};
    blackUpdates_ = 0;
    enter(screen);
}

void Flow::enter(Screen screen) {
    screen_ = screen;
    screenUpdates_ = 0;
    screenSeconds_ = 0;
    pageSeconds_ = 0;
    selectionSeconds_ = 0;
    confirmSeconds_ = 0;
    movieOpen_ = false;
    movieEnded_ = false;
    confirming_ = false;
    selectedRow_ = 0;
    selectedSlot_ = 0;
    prompt_ = Prompt::None;
    movieSeconds_ = 0;
    switch (screen) {
    case Screen::FusionMovie:
        movieOpen_ = movies_ && movies_->open("splash5", localisationSuffix(language_));
        movieEnded_ = !movieOpen_;
        break;
    case Screen::IntroMovie:
        movieOpen_ = movies_ && movies_->open("01_gameintro", localisationSuffix(language_));
        movieEnded_ = !movieOpen_;
        break;
    case Screen::Prompt:
        showPrompt(config_.streetPass.alreadyActive && config_.streetPass.available && !config_.streetPass.restricted
                       ? Prompt::StreetPassOn
                       : Prompt::StreetPassQuestion);
        break;
    default:
        break;
    }
}

void Flow::next(Screen screen, bool blackBefore) {
    enter(screen);
    if (blackBefore) blackUpdates_ = kBlackUpdates;
}

void Flow::showPrompt(Prompt prompt) {
    prompt_ = prompt;
    pageSeconds_ = 0;
}

std::vector<int> Flow::takeSounds() {
    std::vector<int> out;
    out.swap(sounds_);
    return out;
}

void Flow::updatePrompt(const FrontInput& input) {
    const bool a = input.pressed & kButtonA, b = input.pressed & kButtonB;
    const StreetPassEnvironment& system = config_.streetPass;
    switch (prompt_) {
    case Prompt::StreetPassQuestion:
        if (b) {
            showPrompt(Prompt::HomeNotice);
        } else if (a) {
            if (!system.available) showPrompt(Prompt::AgreementMissing);
            else if (system.restricted) showPrompt(Prompt::ParentalControls);
            else showPrompt(system.activationFails ? Prompt::TooManyTitles : Prompt::StreetPassOn);
        }
        break;
    case Prompt::AgreementMissing:
        if (a) showPrompt(Prompt::AgreementHelp);
        break;
    case Prompt::AgreementHelp:
        if (b) showPrompt(Prompt::AgreementMissing);
        else if (a) showPrompt(Prompt::HomeNotice);
        break;
    case Prompt::ParentalControls:
    case Prompt::TooManyTitles:
    case Prompt::StreetPassOn:
        if (a) showPrompt(Prompt::HomeNotice);
        break;
    case Prompt::HomeNotice:
        if (a) next(Screen::Autosave, false);
        break;
    case Prompt::None:
        break;
    }
}

void Flow::update(float dt, const FrontInput& input) {
    if (done()) return;
    ++tick_;
    if (blackUpdates_ > 0) {
        --blackUpdates_;
        return;
    }
    const float elapsed = screenSeconds_;
    ++screenUpdates_;
    screenSeconds_ += dt;
    pageSeconds_ += dt;
    selectionSeconds_ += dt;
    switch (screen_) {
    case Screen::Boot:
        if (elapsed >= kBootSeconds) next(Screen::Logos, true);
        break;
    case Screen::Logos:
        if (elapsed > kLogoSeconds) next(Screen::FusionMovie, true);
        break;
    case Screen::FusionMovie:
        if (!movieEnded_) {
            if (!config_.firstRun && (input.pressed & kSkipButtons)) movieEnded_ = true;
            else if (!movies_->advance(dt)) movieEnded_ = true;
            else movieSeconds_ = movies_->frameSeconds();
        }
        if (movieEnded_) next(config_.firstRun ? Screen::Language : Screen::Prompt, true);
        break;
    case Screen::Language:
        if ((input.pressed & kButtonUp) && selectedRow_ > 0) {
            --selectedRow_;
            sounds_.push_back(kSoundMove);
        }
        if ((input.pressed & kButtonDown) && selectedRow_ + 1 < kLanguageOfRow.size()) {
            ++selectedRow_;
            sounds_.push_back(kSoundMove);
        }
        if (input.pressed & kButtonA) {
            language_ = kLanguageOfRow[selectedRow_];
            sounds_.push_back(kSoundConfirm);
            next(Screen::Prompt, false);
        }
        break;
    case Screen::Prompt:
        updatePrompt(input);
        break;
    case Screen::Autosave:
        if (confirming_) {
            confirmSeconds_ += dt;
            if (confirmSeconds_ >= kSaveIndicatorOffSeconds) next(Screen::SlotSelect, false);
        } else if (input.pressed & kButtonA) {
            confirming_ = true;
        }
        break;
    case Screen::SlotSelect:
        if (confirming_) {
            confirmSeconds_ += dt;
            if (confirmSeconds_ >= kSlotConfirmSeconds + kSlotClearedSeconds) {
                handoff_.slot = selectedSlot_;
                handoff_.newGame = !slotUsed(selectedSlot_);
                next(handoff_.newGame ? Screen::SavingGame : Screen::Loading, false);
            }
            break;
        }
        if ((input.pressed & kButtonUp) && selectedSlot_ > 0) {
            --selectedSlot_;
            selectionSeconds_ = 0;
        }
        if ((input.pressed & kButtonDown) && selectedSlot_ + 1 < config_.slotCount) {
            ++selectedSlot_;
            selectionSeconds_ = 0;
        }
        if (input.pressed & kButtonA) confirming_ = true;
        break;
    case Screen::SavingGame:
        if (elapsed >= kSavingSeconds) next(Screen::Loading, true);
        break;
    case Screen::Loading:
        if (elapsed >= kLoadingSeconds) next(handoff_.newGame ? Screen::IntroMovie : Screen::Done, handoff_.newGame);
        break;
    case Screen::IntroMovie:
        if (!movieEnded_) {
            if (!movies_->advance(dt)) movieEnded_ = true;
            else movieSeconds_ = movies_->frameSeconds();
        }
        if (movieEnded_) next(Screen::MissionCard, true);
        break;
    case Screen::MissionCard:
        if (elapsed >= kMissionCardSeconds || (input.pressed & kButtonA)) next(Screen::Done, false);
        break;
    case Screen::Done:
        break;
    }
}

}
