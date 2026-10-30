// The pause screen and its round menu (src/frontend/pause_menu.cpp, src/frontend/round_menu.cpp; docs/re/ui-screens.md). Synthetic storyboard
// lengths of the shipped layouts (main.bxaml: TransitionOn 0.5, TransitionOn_Round 0.5, TransitionOff_Round 0.5, Left_Move / Right_Move 0.5,
// Loop 0.2; pausetop.bxaml: TransitionOn 0.4, TransitionOff 0; ui_frontend/backbutton: TransitionOn 1.1, TransitionOff 0.4; chase-layoutdump)
// and the button centres the oracle read from the round menu object (request r38-B-1: +0xf4 = 54, 119; +0xfc = 159, 119; +0x104 = 264, 119).
#include "frontend/pause_menu.hpp"
#include "runtime/pad.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::frontend;
constexpr float kDt = 1.0f / 30;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

PauseMenuSetup setup() {
    PauseMenuSetup s;
    s.round.transitionOn = 0.5f;
    s.round.transitionOnRound = 0.5f;
    s.round.transitionOffRound = 0.5f;
    s.round.transitionOff = 0.0f;
    s.round.leftMove = s.round.rightMove = 0.5f;
    s.round.loop = 0.2f;
    s.centres = {{{54, 119}, {159, 119}, {264, 119}}};
    s.topTransitionOn = 0.4f;
    s.topTransitionOff = 0.0f;
    s.shortcutOn = 1.1f;
    s.shortcutOff = 0.4f;
    s.texts.items[0] = "Resume";
    s.texts.items[1] = "Options";
    s.texts.items[2] = "Credits";
    s.texts.items[3] = "Exit";
    return s;
}

void run(PauseMenu& menu, int updates, const PauseInput& input = {}) {
    for (int i = 0; i < updates; ++i) menu.update(kDt, input);
}

PauseInput press(bool PauseInput::*button) {
    PauseInput input;
    input.*button = true;
    return input;
}

PauseMenu opened() {
    PauseMenu menu(setup());
    menu.open();
    run(menu, 25);
    require(menu.round().state() == RoundMenuState::Active, "the round menu is open after TransitionOn_Round");
    menu.takeSounds();
    return menu;
}

void tap(PauseMenu& menu, float x, float y, int updates) {
    PauseInput touching;
    touching.touch = {true, x, y};
    run(menu, updates, touching);
    run(menu, 1);
}

void opening() {
    PauseMenu menu(setup());
    menu.open();
    require(menu.isOpen() && !menu.pageShown(), "page 0 is entered by the page manager in an update");
    menu.update(kDt, {});
    require(menu.pageShown() && menu.round().state() == RoundMenuState::Opening, "page 0 opens the round menu");
    require(menu.round().items().size() == 4 && menu.round().selected() == 0, "four entries, Resume selected");
    require(menu.round().itemAt(1) == 3 && menu.round().itemAt(2) == 0 && menu.round().itemAt(3) == 1, "Exit left, Resume centre, Options right");
    run(menu, 14);
    require(menu.round().state() == RoundMenuState::Opening, "TransitionOn_Round runs 0.5 s");
    run(menu, 2);
    require(menu.round().state() == RoundMenuState::Active, "then the menu takes input");
}

void navigation() {
    PauseMenu menu = opened();
    menu.update(kDt, press(&PauseInput::right));
    require(menu.round().selected() == 1 && menu.round().move() == 1, "Right selects Options");
    require(menu.takeSounds() == std::vector<int>{kRoundMenuSoundMove}, "move sound 0x12");
    require(menu.round().itemAt(2) == 0, "the slots keep the old pictures while Left_Move runs");
    run(menu, 15);
    require(menu.round().move() == 0 && menu.round().itemAt(2) == 1, "after the move the centre shows Options");
    menu.update(kDt, press(&PauseInput::left));
    menu.update(kDt, press(&PauseInput::left));
    require(menu.round().selected() == 3, "Left twice from Options wraps to Exit");
}

void stickToDpad() {
    chase::runtime::PadDevice pad;
    std::size_t last = chase::runtime::kNoStickDirection;
    chase::runtime::PadState right;
    right.stickX = 145;
    pad.poll(right);
    chase::runtime::stickToDpad(pad, last);
    require(pad.slot(3).press == 1 && pad.slot(3).value == 1.0f, "a full push right is a D-pad Right press (0x2289b8)");
    pad.poll(right);
    chase::runtime::stickToDpad(pad, last);
    require(pad.slot(3).press == 0 && pad.slot(3).value == 1.0f, "holding gives no second press");
    pad.poll({});
    chase::runtime::stickToDpad(pad, last);
    require(pad.slot(3).release == 1 && last == chase::runtime::kNoStickDirection, "letting go releases it");
}

void touch() {
    PauseMenu menu = opened();
    tap(menu, 54, 119, 3);
    require(menu.round().selected() == 3, "a tap on the left button turns the menu");
    run(menu, 16);
    tap(menu, 159 + 31, 119, 2);
    require(menu.takeUnported() == std::vector<PauseEntry>{PauseEntry::Exit}, "a tap on the centre button selects it");
    require(menu.takeSounds() == std::vector<int>{kRoundMenuSoundMove, kPauseSoundSelect}, "move then select sound");
    tap(menu, 159, 119, int(kRoundMenuTapUpdates));
    require(menu.takeUnported().empty(), "a touch of 10 updates is not a tap");
    tap(menu, 54, 119 - 33, 2);
    require(menu.round().selected() == 3, "outside the 32 pixel radius nothing happens");
    PauseInput swipe;
    swipe.touch.down = true;
    for (int i = 0; i < 6; ++i) {
        swipe.touch.x = 250.0f - 20.0f * float(i);
        swipe.touch.y = 120;
        menu.update(kDt, swipe);
    }
    require(menu.round().selected() == 0, "a swipe to the left turns the menu to the next entry");
    run(menu, 1);
}

void resume() {
    PauseMenu menu = opened();
    menu.update(kDt, press(&PauseInput::a));
    require(menu.closing(), "A on Resume closes the screen");
    require(menu.takeSounds() == std::vector<int>{kPauseSoundSelect, kRoundMenuSoundClose}, "select sound, then the close sound of 0x20eec4");
    run(menu, 14);
    require(!menu.finished(), "TransitionOff_Round runs 0.5 s");
    run(menu, 2);
    require(menu.finished() && !menu.isOpen(), "then the level goes on");

    PauseMenu back = opened();
    back.update(kDt, press(&PauseInput::b));
    require(back.closing(), "B closes the screen");
    require(back.takeSounds() == std::vector<int>{kPauseSoundBack, kRoundMenuSoundClose}, "back sound and close sound");

    PauseMenu start = opened();
    start.update(kDt, press(&PauseInput::start));
    require(start.closing() && start.takeSounds() == std::vector<int>{kRoundMenuSoundClose}, "Start closes with the close sound only");
    start.update(kDt, press(&PauseInput::a));
    require(start.takeUnported().empty() && start.takeSounds().empty(), "no input while the screen closes");

    PauseMenu early(setup());
    early.open();
    early.update(kDt, press(&PauseInput::start));
    require(early.closing() && early.pageShown(), "the page manager enters page 0 before the Start test of the same update (0x300378)");
    run(early, 16);
    require(early.finished(), "Start in the first update closes after TransitionOff_Round");
}

void unported() {
    PauseMenu menu = opened();
    menu.update(kDt, press(&PauseInput::right));
    run(menu, 16);
    for (int entry = 2; entry < kPauseEntryCount; ++entry) {
        menu.update(kDt, press(&PauseInput::right));
        run(menu, 16);
        menu.update(kDt, press(&PauseInput::a));
        require(!menu.closing() && menu.page() == PausePage::Status, "Credits and Exit leave the screen open (UI-S4)");
    }
    require((menu.takeUnported() == std::vector<PauseEntry>{PauseEntry::Credits, PauseEntry::Exit}), "the two entries are recorded");
}

PauseMenuSetup optionsSetup() {
    PauseMenuSetup s = setup();
    s.texts.optionsSubtitle = "Options";
    s.texts.options.sound = "SFX";
    s.texts.options.music = "Music";
    s.texts.options.invertCamera = "Invert Camera";
    s.texts.options.yes = "Yes";
    s.texts.options.no = "No";
    s.texts.options.language = "Language";
    const char* const names[] = {"English", "French", "Italian", "German", "Spanish", "Dutch", "Danish", "Portuguese", "Russian"};
    for (int i = 0; i < chase::runtime::kLanguageCount; ++i) s.texts.languageNames[std::size_t(i)] = names[i];
    return s;
}

// The block and live bytes the emulator servant 20261028-uiport-servant read on 2026-10-28, recorded from a save state (paused on the
// Options page after mission 1-3, slot 1: sound effects 5, music 5, Invert camera on); see tests/options_page_test.cpp.
void optionsPage() {
    std::array<std::uint8_t, chase::save::kOptionsSize> options{};
    const std::uint8_t recorded[8] = {0x8a, 0x01, 0x01, 0x23, 0x0a, 0x05, 0x08, 0x05};
    std::copy(recorded, recorded + 8, options.begin());
    chase::runtime::OptionVolumes live{5, 5};
    PauseMenu menu(optionsSetup());
    menu.bindOptions(options.data(), &live, 1);
    menu.open();
    run(menu, 25);
    menu.update(kDt, press(&PauseInput::right));
    run(menu, 16);
    menu.takeSounds();
    menu.takeGains();
    menu.update(kDt, press(&PauseInput::a));
    require(menu.page() == PausePage::Status && menu.round().state() == RoundMenuState::Closing, "A on Options closes page 0's round menu first");
    require(menu.takeSounds() == std::vector<int>{kPauseSoundSelect, kRoundMenuSoundClose}, "select sound and close sound");
    run(menu, 16);
    require(menu.page() == PausePage::Options, "then the page manager enters the Options page");
    require(menu.round().items().size() == 4 && menu.round().selected() == 0, "four rows, SFX selected first");
    require(menu.round().items()[0].text == "SFX: 5" && menu.round().items()[1].text == "Music: 5" &&
                menu.round().items()[2].text == "Invert Camera: Yes" && menu.round().items()[3].text == "Language",
            "the row texts of the recorded state");
    require(menu.backShown() && !menu.selectShown(), "Back on, no Select on a volume row");
    run(menu, 20);
    menu.takeSounds();

    menu.update(kDt, press(&PauseInput::right));
    run(menu, 16);
    PauseInput down;
    down.down = true;
    menu.takeGains();
    menu.update(kDt, down);
    require(live.music == 4 && options[7] == 4 && live.sound == 4, "Down on Music: music 4, the SFX byte previews it");
    require(menu.round().items()[1].text == "Music: 4", "the row text follows");
    const PauseGains gains = menu.takeGains();
    require(gains.music == 0.4f && gains.sound == 0.4f, "both gains are set");
    menu.update(kDt, press(&PauseInput::left));
    require(live.sound == 5 && menu.takeGains().sound == 0.5f, "Left restores the SFX byte of the slot before the turn");
    run(menu, 16);
    menu.update(kDt, press(&PauseInput::left));
    run(menu, 16);
    require(menu.round().selected() == 3 && menu.selectShown(), "Language row, Select shown");
    menu.takeSounds();

    menu.update(kDt, press(&PauseInput::b));
    require(menu.takeSaveRequest() && menu.round().state() == RoundMenuState::Closing, "B after a change asks for the save");
    require(!menu.takeSaveRequest(), "once");
    menu.saveDone();
    run(menu, 16);
    require(menu.page() == PausePage::Status && menu.round().selected() == 1, "after the save page 0 again, on Options");
    require(!menu.optionsPage().changed() && menu.optionsPage().savedSelection() == 0, "the change is cleared, the selection reset");

    run(menu, 20);
    menu.update(kDt, press(&PauseInput::a));
    run(menu, 16);
    require(menu.page() == PausePage::Options && menu.round().selected() == 0, "Options again, on the first row");
    menu.update(kDt, press(&PauseInput::left));
    run(menu, 16);
    menu.update(kDt, press(&PauseInput::left));
    run(menu, 16);
    require(menu.round().selected() == 3, "Language row");
    menu.takeSounds();
    menu.update(kDt, press(&PauseInput::a));
    require(menu.takeSounds() == std::vector<int>{chase::runtime::kOptionsSoundSelect, kRoundMenuSoundClose} && menu.round().state() == RoundMenuState::Closing,
            "A on Language plays the select sound and closes the Options rows");
    run(menu, 16);
    require(menu.page() == PausePage::Languages, "then the page manager enters the Languages page");
    require(menu.round().items().size() == 9 && menu.round().items()[0].text == "English" && menu.round().items()[5].text == "Danish" &&
                menu.round().items()[6].text == "Dutch" && menu.round().items()[8].text == "Portuguese",
            "nine rows in the order of table 0x3759b0");
    require(menu.round().items()[4].image == "UI_Language_Spanish.png" && menu.round().items()[8].image == "UI_Language_Portuguese.png", "the flag pictures");
    require(menu.backShown() && menu.selectShown(), "Back and Select shown (UI-S14: a row is selected at once)");
    run(menu, 20);
    menu.takeSounds();

    menu.update(kDt, press(&PauseInput::b));
    require(menu.takeSounds() == std::vector<int>{chase::runtime::kLanguagesSoundBack, kRoundMenuSoundClose} && !menu.backShown() && !menu.selectShown(),
            "B plays the back sound and the list closes with both shortcuts off");
    run(menu, 16);
    require(menu.page() == PausePage::Options && menu.round().selected() == 3 && !menu.takeSaveRequest(), "back on the Options page, Language row, no save");
    require(menu.language() == 1 && !menu.takeLanguageChange() && options[1] == 0x01, "no language changed");

    run(menu, 25);
    menu.update(kDt, press(&PauseInput::a));
    run(menu, 56);
    menu.update(kDt, press(&PauseInput::right));
    run(menu, 16);
    menu.update(kDt, press(&PauseInput::right));
    run(menu, 16);
    require(menu.round().selected() == 2, "third row: Italian");
    menu.takeSounds();
    menu.update(kDt, press(&PauseInput::a));
    require(menu.languagesPage().pending() == 3 && !menu.backShown() && !menu.selectShown() && menu.round().state() == RoundMenuState::Closing,
            "A makes the language pending and closes the list");
    require(!menu.takeSaveRequest() && options[1] == 0x01, "nothing changes while the list closes");
    run(menu, 16);
    require(menu.takeLanguageChange() == 3 && options[1] == 3 && menu.language() == 3, "after the close options +1 takes the language");
    require(menu.takeSaveRequest() && !menu.takeSaveRequest(), "and the options are saved once");
    require(menu.page() == PausePage::Languages, "the page waits for the save");
    menu.saveDone();
    run(menu, 16);
    require(menu.page() == PausePage::Options && menu.round().selected() == 3 && !menu.optionsPage().changed(), "the save callback returns to the Options page");
    require(menu.round().items()[3].text == "Language", "row texts are written again");
    menu.update(kDt, press(&PauseInput::b));
    run(menu, 16);
    require(menu.page() == PausePage::Status && !menu.takeSaveRequest(), "B without a change goes to page 0 without a save");
}
}

int main() {
    try {
        opening();
        navigation();
        stickToDpad();
        touch();
        resume();
        unported();
        optionsPage();
    } catch (const std::exception& error) {
        std::cerr << "pause-menu-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "pause-menu-test: ok\n";
    return 0;
}
