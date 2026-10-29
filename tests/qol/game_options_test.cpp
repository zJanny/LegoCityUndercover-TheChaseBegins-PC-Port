// Port side of the Game tab (src/runtime/qol_game_options.hpp) over the Options page state (src/runtime/options_page.hpp).
// Spec: docs/design/qol-settings.md entry 2026-10-28T09:32:07 (same values, same save, in sync with the original Options page) and the
// user's decision of 2026-10-29 relayed in the brief of 20261029-qolwire-B (the language row opens the Languages page).
// Option block and live bytes: recorded from a save state by 20261028-uiport-servant on 2026-10-28, paused on the Options page after
// mission 1-3, slot 1 in use, sound effects 5, music 5, Invert camera on, language 1 (as pinned in tests/options_page_test.cpp).
#include "qol/game_tab.hpp"
#include "runtime/options_page.hpp"
#include "runtime/qol_game_options.hpp"

#include <array>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

using Block = std::array<std::uint8_t, 8>;
constexpr Block recordedOptions = {0x8a, 0x01, 0x01, 0x23, 0x0a, 0x05, 0x08, 0x05};
constexpr Block noSlotOptions = {0x8a, 0x01, 0xff, 0x21, 0x0a, 0x0a, 0x08, 0x08};

OptionsTexts texts() {
    OptionsTexts t;
    t.sound = "SFX";
    t.music = "Music";
    t.invertCamera = "Invert Camera";
    t.yes = "Yes";
    t.no = "No";
    t.language = "Language";
    return t;
}

template <class F>
bool throwsRange(F&& f) {
    try {
        f();
    } catch (const std::out_of_range&) {
        return true;
    }
    return false;
}

void readsTheSameValues() {
    Block options = recordedOptions;
    OptionVolumes live{5, 5};
    OptionsPage page;
    QolGameOptions game(page, options.data(), live);
    require(game.sfxVolume() == 5 && game.musicVolume() == 5 && game.invertCamera() && game.language() == 1, "recorded state read");
    Block reset = noSlotOptions;
    OptionVolumes liveOnly{7, 6};
    QolGameOptions noSlot(page, reset.data(), liveOnly);
    require(noSlot.sfxVolume() == 7 && noSlot.musicVolume() == 6 && !noSlot.invertCamera(), "without a slot the live bytes are read");
}

void originalPageChangesAreSeen() {
    Block options = recordedOptions;
    OptionVolumes live{5, 5};
    OptionsPage page;
    QolGameOptions game(page, options.data(), live);
    qol::GameTab tab(game);
    page.update(int(OptionsRow::SoundVolume), true, false, options.data(), live);
    page.update(int(OptionsRow::MusicVolume), false, true, options.data(), live);
    page.update(int(OptionsRow::InvertCamera), true, false, options.data(), live);
    require(tab.value(qol::GameRow::SfxVolume) == 6, "sound step of the original page seen by the tab");
    require(tab.value(qol::GameRow::MusicVolume) == 4, "music step of the original page seen by the tab");
    require(tab.value(qol::GameRow::InvertCamera) == 0, "invert toggle of the original page seen by the tab");
}

void tabChangesReachTheOriginalPage() {
    Block options = recordedOptions;
    OptionVolumes live{5, 5};
    OptionsPage page;
    QolGameOptions game(page, options.data(), live);
    qol::GameTab tab(game);
    require(!game.saveNeeded(), "no change yet");
    require(tab.step(qol::GameRow::SfxVolume, 1) == qol::GameResult::Changed, "tab sound step");
    require(tab.setValue(qol::GameRow::MusicVolume, 9) == qol::GameResult::Changed, "tab music set");
    require(tab.activate(qol::GameRow::InvertCamera) == qol::GameResult::Changed, "tab invert toggle");
    const OptionsTexts t = texts();
    require(optionsRowText(0, t, options.data(), live, 1) == "SFX: 6", "original page shows the tab's sound volume");
    require(optionsRowText(1, t, options.data(), live, 1) == "Music: 9", "original page shows the tab's music volume");
    require(optionsRowText(2, t, options.data(), live, 1) == "Invert Camera: No", "original page shows the tab's invert camera");
    require(options[5] == 6 && options[7] == 9 && (options[3] & kInvertCameraBit) == 0, "the options block of the save is edited");
    require(game.saveNeeded() && page.changed(), "the page's changed flag requests the save on leaving");
    require(live.sound == 6, "the sound effects byte is restored from the slot after a music change");
}

void gainsAndPageRequest() {
    Block options = recordedOptions;
    OptionVolumes live{5, 5};
    OptionsPage page;
    QolGameOptions game(page, options.data(), live);
    qol::GameTab tab(game);
    tab.setValue(qol::GameRow::SfxVolume, 7);
    const std::optional<float> gain = game.takeSoundGain();
    require(gain && *gain == 0.7f, "the sound gain of the last step is reported");
    require(!game.takeSoundGain(), "a gain is reported once");
    require(game.takePageRequest() == OptionsPageRequest::None, "no page requested by volume edits");
    require(tab.activate(qol::GameRow::Language) == qol::GameResult::OpenedLanguages, "language row activation");
    require(game.takePageRequest() == OptionsPageRequest::Languages, "the Languages page is requested as the original A does");
    require(game.takePageRequest() == OptionsPageRequest::None, "a request is reported once");
    require(options[1] == 1, "activating the language row does not change the language");
}

void noSlotUsesLiveBytes() {
    Block options = noSlotOptions;
    OptionVolumes live{7, 6};
    OptionsPage page;
    QolGameOptions game(page, options.data(), live);
    game.setSfxVolume(2);
    require(live.sound == 2 && options == noSlotOptions, "without a slot only the live byte changes");
}

void rejectsOutOfRange() {
    Block options = recordedOptions;
    OptionVolumes live{5, 5};
    OptionsPage page;
    QolGameOptions game(page, options.data(), live);
    require(throwsRange([&] { game.setSfxVolume(11); }), "volume 11 throws");
    require(throwsRange([&] { game.setMusicVolume(-1); }), "volume -1 throws");
    require(options == recordedOptions && live.sound == 5 && !game.saveNeeded(), "rejected values change nothing");
    Block badLanguage = recordedOptions;
    badLanguage[1] = 10;
    QolGameOptions bad(page, badLanguage.data(), live);
    require(throwsRange([&] { bad.language(); }), "language 10 in the block throws");
    badLanguage[1] = 0;
    require(throwsRange([&] { bad.language(); }), "language 0 in the block throws");
}

}

int main() {
    readsTheSameValues();
    originalPageChangesAreSeen();
    tabChangesReachTheOriginalPage();
    gainsAndPageRequest();
    noSlotUsesLiveBytes();
    rejectsOutOfRange();
    return failures == 0 ? 0 : 1;
}
