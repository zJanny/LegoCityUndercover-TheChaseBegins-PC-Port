#include "runtime/options_page.hpp"

namespace chase::runtime {
namespace {

constexpr std::size_t kLastSlotOffset = 2, kFlagsOffset = 3, kSoundOffset = 4, kMusicOffset = 6;
constexpr int kFrenchLanguage = 2;

}

std::optional<std::size_t> optionsSlot(const std::uint8_t* options) {
    const std::uint8_t slot = options[kLastSlotOffset];
    if (slot > 1) return std::nullopt;
    return slot;
}

void loadingScreenVolumes(const std::uint8_t* options, OptionVolumes& live) {
    if (const std::optional<std::size_t> slot = optionsSlot(options)) {
        live.sound = options[kSoundOffset + *slot];
        live.music = options[kMusicOffset + *slot];
    }
}

void restoreSoundVolume(const std::uint8_t* options, OptionVolumes& live) {
    if (const std::optional<std::size_t> slot = optionsSlot(options)) live.sound = options[kSoundOffset + *slot];
}

std::string optionsRowText(int row, const OptionsTexts& texts, const std::uint8_t* options, const OptionVolumes& live, int language) {
    const std::string separator = language == kFrenchLanguage ? " : " : ": ";
    const std::optional<std::size_t> slot = optionsSlot(options);
    switch (OptionsRow(row)) {
    case OptionsRow::SoundVolume:
        return texts.sound + separator + std::to_string(slot ? options[kSoundOffset + *slot] : live.sound);
    case OptionsRow::MusicVolume:
        return texts.music + separator + std::to_string(slot ? options[kMusicOffset + *slot] : live.music);
    case OptionsRow::InvertCamera:
        return texts.invertCamera + separator + ((options[kFlagsOffset] & kInvertCameraBit) ? texts.yes : texts.no);
    case OptionsRow::Language:
        return texts.language;
    }
    return {};
}

OptionsPageActions OptionsPage::enter() {
    OptionsPageActions actions;
    actions.textsChanged = true;
    actions.backShortcut = true;
    return actions;
}

OptionsPageActions OptionsPage::selectionChanged(int selected) {
    OptionsPageActions actions;
    actions.selectShortcut = !(selected == int(OptionsRow::SoundVolume) || selected == int(OptionsRow::MusicVolume));
    return actions;
}

OptionsPageActions OptionsPage::a(int selected, std::uint8_t* options) {
    OptionsPageActions actions;
    savedSelection_ = selected;
    if (selected == int(OptionsRow::Language)) {
        actions.sounds.push_back(kOptionsSoundSelect);
        actions.page = OptionsPageRequest::Languages;
    } else if (selected == int(OptionsRow::InvertCamera)) {
        actions.sounds.push_back(kOptionsSoundSelect);
        options[kFlagsOffset] ^= kInvertCameraBit;
        actions.textsChanged = true;
    }
    return actions;
}

OptionsPageActions OptionsPage::b() {
    OptionsPageActions actions;
    if (changed_) {
        actions.saveRequest = true;
        actions.selectShortcut = false;
        actions.backShortcut = false;
        actions.closeRoundMenu = true;
    } else {
        actions.sounds.push_back(kOptionsSoundBack);
        actions.page = OptionsPageRequest::Page0;
    }
    savedSelection_ = 0;
    return actions;
}

OptionsPageActions OptionsPage::saveDone() {
    OptionsPageActions actions;
    actions.page = OptionsPageRequest::Page0;
    changed_ = false;
    return actions;
}

OptionsPageActions OptionsPage::leaveRequest(const std::uint8_t* options, OptionVolumes& live) {
    OptionsPageActions actions;
    restoreSoundVolume(options, live);
    actions.soundGain = gainOf(live.sound);
    actions.closeRoundMenu = true;
    actions.selectShortcut = false;
    actions.backShortcut = false;
    return actions;
}

void OptionsPage::step(int selected, int direction, std::uint8_t* options, OptionVolumes& live, OptionsPageActions& actions) {
    const std::optional<std::size_t> slot = optionsSlot(options);
    const auto moved = [&](std::uint8_t value) {
        return direction < 0 ? value > 0 : value < kOptionsMaxVolume;
    };
    if (selected == int(OptionsRow::SoundVolume)) {
        if (moved(live.sound)) {
            changed_ = true;
            live.sound = std::uint8_t(live.sound + direction);
            if (slot) options[kSoundOffset + *slot] = live.sound;
            actions.soundGain = gainOf(live.sound);
            actions.sounds.push_back(kOptionsSoundStep);
        }
    } else if (selected == int(OptionsRow::MusicVolume)) {
        if (moved(live.music)) {
            changed_ = true;
            live.music = std::uint8_t(live.music + direction);
            if (slot) options[kMusicOffset + *slot] = live.music;
            actions.musicGain = gainOf(live.music);
            live.sound = live.music;
            actions.soundGain = gainOf(live.sound);
            actions.sounds.push_back(kOptionsSoundStep);
        }
    } else if (selected == int(OptionsRow::InvertCamera)) {
        changed_ = true;
        options[kFlagsOffset] ^= kInvertCameraBit;
    }
    actions.textsChanged = true;
}

OptionsPageActions OptionsPage::update(int selected, bool up, bool down, std::uint8_t* options, OptionVolumes& live) {
    OptionsPageActions actions;
    if (down) step(selected, -1, options, live, actions);
    if (up) step(selected, 1, options, live, actions);
    return actions;
}

}
