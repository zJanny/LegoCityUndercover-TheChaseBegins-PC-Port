// The Options page of the pause screen and the live volume bytes (src/runtime/options_page.cpp; docs/re/save-format.md, docs/re/ui-screens.md).
// The options block and the live bytes of kRecordedOptions / kRecordedVolumes were read by the emulator servant 20261028-uiport-servant on
// 2026-10-28, recorded from a save state: paused on the Options page after mission 1-3, slot 1 in use with sound effects 5, music 5 and
// Invert camera on (0x51b490: 8a 01 01 23 0a 05 08 05, rest zero; 0x37751d = 5, 0x37751e = 5; the page showed "Music: 5", the SFX row
// "SFX: 5" and the Invert camera row "Invert Camera: Yes" with the Select shortcut). The page rules are read from the code (0x31aebc,
// 0x31ae48, 0x31ada4, 0x31ae14, 0x31ad20, 0x31af84, 0x1e4804, 0x303a40, 0x300378) by 20261028-uiport-B.
#include "runtime/options_page.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

using Block = std::array<std::uint8_t, 8>;
constexpr Block kRecordedOptions = {0x8a, 0x01, 0x01, 0x23, 0x0a, 0x05, 0x08, 0x05};
constexpr OptionVolumes kRecordedVolumes = {5, 5};
constexpr Block kResetOptions = {0x8a, 0x01, 0xff, 0x21, 0x0a, 0x0a, 0x08, 0x08};

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

void loadingScreen() {
    OptionVolumes live;
    loadingScreenVolumes(kRecordedOptions.data(), live);
    require(live.sound == kRecordedVolumes.sound && live.music == kRecordedVolumes.music, "the loading screen takes the volumes of the slot at +2");
    OptionVolumes untouched{3, 4};
    loadingScreenVolumes(kResetOptions.data(), untouched);
    require(untouched.sound == 3 && untouched.music == 4, "without a slot at +2 the live bytes stay");
    require(!optionsSlot(kResetOptions.data()) && optionsSlot(kRecordedOptions.data()) == std::size_t(1), "+2 is the slot 0 or 1");
}

void rowTexts() {
    const OptionsTexts t = texts();
    require(optionsRowText(0, t, kRecordedOptions.data(), kRecordedVolumes, 1) == "SFX: 5", "SFX row of the recorded state");
    require(optionsRowText(1, t, kRecordedOptions.data(), kRecordedVolumes, 1) == "Music: 5", "Music row of the recorded state");
    require(optionsRowText(2, t, kRecordedOptions.data(), kRecordedVolumes, 1) == "Invert Camera: Yes", "Invert camera row of the recorded state");
    require(optionsRowText(3, t, kRecordedOptions.data(), kRecordedVolumes, 1) == "Language", "the Language row is the label only");
    require(optionsRowText(0, t, kRecordedOptions.data(), kRecordedVolumes, 2) == "SFX : 5", "French puts a space before the colon");
    require(optionsRowText(1, t, kResetOptions.data(), OptionVolumes{7, 6}, 1) == "Music: 6", "without a slot the live byte is shown");
    require(optionsRowText(2, t, kResetOptions.data(), kRecordedVolumes, 1) == "Invert Camera: No", "bit 1 clear shows No");
}

void steps() {
    Block options = kRecordedOptions;
    OptionVolumes live = kRecordedVolumes;
    OptionsPage page;
    OptionsPageActions down = page.update(int(OptionsRow::MusicVolume), false, true, options.data(), live);
    require(live.music == 4 && options[7] == 4 && options[6] == 8, "Down on Music writes the slot's music byte");
    require(live.sound == 4 && options[5] == 5, "the music step leaves its value in the sound effects byte, not in the slot's SFX byte");
    require(down.musicGain == 0.4f && down.soundGain == 0.4f && down.sounds == std::vector<int>{kOptionsSoundStep}, "music gain, sound gain, step sound");
    require(page.changed() && down.textsChanged, "a step marks the page changed");

    restoreSoundVolume(options.data(), live);
    require(live.sound == 5, "a Left or Right press of the pause update reloads the SFX byte from the slot");

    for (int i = 0; i < 7; ++i) page.update(int(OptionsRow::SoundVolume), true, false, options.data(), live);
    require(live.sound == 10 && options[5] == 10, "Up stops at 10");
    OptionVolumes zero{0, 0};
    const OptionsPageActions none = page.update(int(OptionsRow::SoundVolume), false, true, options.data(), zero);
    require(zero.sound == 0 && none.sounds.empty() && !none.soundGain && none.textsChanged, "Down at 0 changes nothing but rewrites the texts");

    Block noSlot = kResetOptions;
    OptionVolumes globals;
    page.update(int(OptionsRow::SoundVolume), false, true, noSlot.data(), globals);
    require(globals.sound == 9 && noSlot[4] == 10 && noSlot[5] == 10, "without a slot only the live byte changes");

    OptionsPage invert;
    invert.update(int(OptionsRow::InvertCamera), true, false, options.data(), live);
    require((options[3] & kInvertCameraBit) == 0 && invert.changed(), "Up on Invert camera toggles bit 1 and marks the change");
    const OptionsPageActions language = invert.update(int(OptionsRow::Language), true, true, options.data(), live);
    require(language.sounds.empty() && language.textsChanged, "Up and Down on the Language row only rewrite the texts");
}

void buttons() {
    Block options = kRecordedOptions;
    OptionVolumes live = kRecordedVolumes;
    OptionsPage page;
    require(page.enter().backShortcut == true, "enter shows Back");
    require(page.selectionChanged(0).selectShortcut == false && page.selectionChanged(1).selectShortcut == false, "no Select on the volume rows");
    require(page.selectionChanged(2).selectShortcut == true && page.selectionChanged(3).selectShortcut == true, "Select on Invert camera and Language");
    const OptionsPageActions toggle = page.a(int(OptionsRow::InvertCamera), options.data());
    require((options[3] & kInvertCameraBit) == 0 && toggle.sounds == std::vector<int>{kOptionsSoundSelect} && !page.changed(),
            "A on Invert camera toggles bit 1 with the select sound and does not mark the page changed");
    require(page.a(int(OptionsRow::Language), options.data()).page == OptionsPageRequest::Languages, "A on Language asks for the Languages page");
    require(page.savedSelection() == 3, "A saves the selection");
    const OptionsPageActions plainBack = page.b();
    require(plainBack.page == OptionsPageRequest::Page0 && plainBack.sounds == std::vector<int>{kOptionsSoundBack} && !plainBack.saveRequest,
            "B without a change goes back to page 0 with the back sound");
    require(page.savedSelection() == 0, "B resets the saved selection");

    page.update(int(OptionsRow::MusicVolume), false, true, options.data(), live);
    const OptionsPageActions saving = page.b();
    require(saving.saveRequest && saving.closeRoundMenu && saving.page == OptionsPageRequest::None && saving.sounds.empty(),
            "B after a change asks for the save and closes the round menu");
    const OptionsPageActions done = page.saveDone();
    require(done.page == OptionsPageRequest::Page0 && !page.changed(), "the save callback goes to page 0 and clears the change");

    page.update(int(OptionsRow::InvertCamera), true, false, options.data(), live);
    require(page.changed(), "a toggle by Up marks the page changed");
    const OptionsPageActions languagesDone = page.languagesSaved();
    require(languagesDone.page == OptionsPageRequest::Options && !page.changed(), "the save callback of the Languages page goes back to the Options page (0x2b4c44)");

    const OptionsPageActions leave = page.leaveRequest(options.data(), live);
    require(live.sound == 5 && leave.soundGain == 0.5f && leave.closeRoundMenu, "the leave request reloads the SFX byte and sets its gain");
}

}

int main() {
    try {
        loadingScreen();
        rowTexts();
        steps();
        buttons();
    } catch (const std::exception& error) {
        std::cerr << "options-page-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "options-page-test: ok\n";
    return 0;
}
