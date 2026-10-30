// The always shown "PC Settings" entry of the round pause menu, pinned to qol-settings.md §2026-10-28T09:32:07 and
// docs/rules/qol.md "Faithful first": the last entry of the round pause menu, chosen like the others (D-pad, A, touch), opens the menu only,
// changes no game behaviour, shown with every QoL feature off; the original entries and the Options page stay unchanged.
// Round and button centres as in tests/pause_menu_test.cpp (storyboard lengths of the shipped layouts, the oracle's centres of
// Button_2, Button_3 and Button_4). Written by 20261030-qolpanels-C, 2026-10-30.
#include "qol/pause_entry.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::frontend;
constexpr float kDt = 1.0f / 30;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

PauseMenuSetup original() {
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

PauseMenuSetup withEntry() {
    PauseMenuSetup s = original();
    qol::addPauseEntry(s);
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

PauseMenu opened(const PauseMenuSetup& setup) {
    PauseMenu menu(setup);
    menu.open();
    run(menu, 25);
    require(menu.round().state() == RoundMenuState::Active, "the round menu is open");
    menu.takeSounds();
    return menu;
}

void entryIsLast() {
    const PauseMenuSetup setup = withEntry();
    require(setup.extraEntries.size() == 1 && setup.extraEntries[0].text == "PC Settings" &&
                setup.extraEntries[0].image == "UI_StatusScreen_Options.png",
        "one extra entry, PC Settings, with the picture of Options (qol-settings-drawing.md Q11)");
    PauseMenu menu(setup);
    menu.open();
    menu.update(kDt, {});
    const auto& items = menu.round().items();
    require(items.size() == 5 && items[0].text == "Resume" && items[1].text == "Options" && items[2].text == "Credits" && items[3].text == "Exit" &&
                items[4].text == "PC Settings",
        "the four original entries in their order, PC Settings last");
    require(items[0].image == "UI_StatusScreen_Resume.png" && items[1].image == "UI_StatusScreen_Options.png" &&
                items[2].image == "UI_StatusScreen_Credits.png" && items[3].image == "UI_StatusScreen_Exit.png",
        "the original pictures stay");
    require(menu.round().selected() == 0, "Resume stays the first selection");
    PauseMenu plain(original());
    plain.open();
    plain.update(kDt, {});
    require(plain.round().items().size() == 4, "without the entry the menu has the four original entries");
}

void dpad() {
    PauseMenu menu = opened(withEntry());
    menu.update(kDt, press(&PauseInput::left));
    require(menu.round().selected() == 4, "Left from Resume reaches PC Settings, the last entry");
    run(menu, 16);
    menu.update(kDt, press(&PauseInput::left));
    require(menu.round().selected() == 3, "Left again is Exit");
    run(menu, 16);
    menu.update(kDt, press(&PauseInput::right));
    run(menu, 16);
    menu.update(kDt, press(&PauseInput::right));
    run(menu, 16);
    require(menu.round().selected() == 0, "Right from PC Settings wraps to Resume");
}

void activation() {
    PauseMenu menu = opened(withEntry());
    menu.update(kDt, press(&PauseInput::left));
    run(menu, 16);
    menu.takeSounds();
    menu.update(kDt, press(&PauseInput::a));
    require(menu.takeExtraChosen() == std::vector<std::size_t>{0}, "A on PC Settings chooses the extra entry 0");
    require(menu.takeExtraChosen().empty(), "and is taken once");
    require(menu.takeUnported().empty(), "it is not an unported original entry");
    require(!menu.closing() && menu.isOpen() && menu.page() == PausePage::Status, "the pause screen stays open on its first page");
    require(menu.takeSounds() == std::vector<int>{kPauseSoundSelect}, "the select sound only");
    require(menu.round().selected() == 4, "the entry stays selected when the menu returns");
}

void touch() {
    PauseMenu menu = opened(withEntry());
    PauseInput touching;
    touching.touch = {true, 54, 119};
    run(menu, 3, touching);
    run(menu, 1);
    require(menu.round().selected() == 4, "a tap on the left button turns to PC Settings");
    run(menu, 16);
    touching.touch = {true, 159 + 31, 119};
    run(menu, 2, touching);
    run(menu, 1);
    require(menu.takeExtraChosen() == std::vector<std::size_t>{0}, "a tap on the centre button chooses it");
}

void originalEntriesUnchanged() {
    PauseMenu menu = opened(withEntry());
    menu.update(kDt, press(&PauseInput::a));
    require(menu.closing(), "A on Resume still closes the screen");
    PauseMenu options = opened(withEntry());
    options.update(kDt, press(&PauseInput::right));
    run(options, 16);
    options.update(kDt, press(&PauseInput::a));
    run(options, 20);
    require(options.page() == PausePage::Options && options.takeExtraChosen().empty(), "A on Options still opens the Options page");
    PauseMenu credits = opened(withEntry());
    for (int i = 0; i < 2; ++i) {
        credits.update(kDt, press(&PauseInput::right));
        run(credits, 16);
    }
    credits.update(kDt, press(&PauseInput::a));
    require(credits.takeUnported() == std::vector<PauseEntry>{PauseEntry::Credits} && credits.takeExtraChosen().empty(), "Credits is still unported");
}

void holdsNothingUnbound() {
    qol::PauseSettings settings;
    PauseMenu menu = opened(withEntry());
    menu.update(kDt, press(&PauseInput::left));
    run(menu, 16);
    menu.update(kDt, press(&PauseInput::a));
    require(!qol::pauseEntryHolds(settings, menu), "without a bound menu nothing holds the pause screen");
    require(!qol::pauseEntryPresented(settings, 960, 600), "and nothing is presented");
    SDL_Event event{};
    event.type = SDL_KEYDOWN;
    require(!qol::pauseEntryConsumes(settings, event, nullptr), "and no event is consumed");
}

}

int main() {
    try {
        entryIsLast();
        dpad();
        activation();
        touch();
        originalEntriesUnchanged();
        holdsNothingUnbound();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << "\n";
        return 1;
    }
    std::cout << "qol-pause-entry: all checks passed\n";
    return 0;
}
