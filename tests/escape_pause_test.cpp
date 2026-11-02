// Escape on the pause screen's pages (QOLCTL-12): Escape is the fixed Start binding everywhere, except that on every page of the open
// pause screen (main wheel, Options, Languages) it presses B and goes back one page; the main wheel's back is Resume. Spec: docs/design/qol-controls.md entry 2026-11-01T13:50:44 (in play
// Escape opens the pause menu through the path of Start; in a menu it goes back or closes). The pause screen is the real PauseMenu with the
// synthetic storyboard lengths of tests/pause_menu_test.cpp; page changes and sounds are the ones that test pins for B and Start.
// Cases written by 20261102-qolctl2-C on 2026-11-02.
#include "frontend/pause_menu.hpp"
#include "input.hpp"
#include "runtime/qol_escape.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;
using namespace chase::frontend;
constexpr float kDt = 1.0f / 30;
constexpr int kEscape = 27;
constexpr int kReturn = 13;

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
    s.grid.blockbusterOn = 0.4f;
    s.grid.transitionOn = 0.5f;
    s.grid.leftMove = s.grid.rightMove = 0.4f;
    s.grid.show = 0.24f;
    s.grid.hide = 0.2f;
    s.grid.pressed = 0.5f;
    const float tiles[runtime::kItemGridTiles][2] = {{57, 53}, {57, 111}, {107, 81}, {107, 140}, {157, 52}, {157, 110}, {208, 81}, {208, 139}, {258, 52}, {258, 110}};
    for (int i = 0; i < runtime::kItemGridTiles; ++i) s.gridGeometry.tiles[std::size_t(i)] = {tiles[i][0], tiles[i][1], 32};
    s.gridGeometry.leftArrow = {10, 111, 16};
    s.gridGeometry.rightArrow = {306, 111, 16};
    s.topTransitionOn = 0.4f;
    s.topTransitionOff = 0.0f;
    s.shortcutOn = 1.1f;
    s.shortcutOff = 0.4f;
    s.texts.items[0] = "Resume";
    s.texts.items[1] = "Options";
    s.texts.items[2] = "Credits";
    s.texts.items[3] = "Exit";
    s.texts.optionsSubtitle = "Options";
    s.texts.options.sound = "SFX";
    s.texts.options.music = "Music";
    s.texts.options.invertCamera = "Invert Camera";
    s.texts.options.yes = "Yes";
    s.texts.options.no = "No";
    s.texts.options.language = "Language";
    const char* const names[] = {"English", "French", "Italian", "German", "Spanish", "Dutch", "Danish", "Portuguese", "Russian"};
    for (int i = 0; i < runtime::kLanguageCount; ++i) s.texts.languageNames[std::size_t(i)] = names[i];
    return s;
}

struct Rig {
    PauseMenu menu;
    InputState input;
    Rig() : menu(setup()), input(bindings()) {
        runtime::bindEscape(input, kEscape);
        menu.open();
        run(25);
        menu.takeSounds();
    }
    static std::vector<Binding> bindings() {
        std::istringstream file("Start key Return hold\n");
        return readBindings(file, [](const std::string&) { return kReturn; });
    }
    PausePage page() const { return menu.isOpen() ? menu.page() : PausePage::None; }
    void run(int updates, PauseInput input = {}) {
        for (int i = 0; i < updates; ++i) menu.update(kDt, input);
    }
    void press(bool PauseInput::*button) {
        PauseInput pressed;
        pressed.*button = true;
        menu.update(kDt, pressed);
    }
    void escapeDown() {
        runtime::routeEscape(input, kEscape, page());
        input.event(Source::Key, kEscape, true);
        PauseInput pressed;
        pressed.start = input.active(Action::Start);
        pressed.b = input.active(Action::B);
        menu.update(kDt, pressed);
    }
    void escapeUp() { input.event(Source::Key, kEscape, false); }
    void escape() {
        escapeDown();
        escapeUp();
    }
    void toOptions() {
        press(&PauseInput::right);
        run(16);
        press(&PauseInput::a);
        run(36);
        require(menu.page() == PausePage::Options, "the Options page is shown");
        menu.takeSounds();
    }
    void toLanguages() {
        toOptions();
        press(&PauseInput::left);
        run(16);
        press(&PauseInput::a);
        run(36);
        require(menu.page() == PausePage::Languages, "the Languages page is shown");
        menu.takeSounds();
    }
};

void inPlay() {
    InputState input(Rig::bindings());
    runtime::bindEscape(input, kEscape);
    runtime::routeEscape(input, kEscape, PausePage::None);
    input.event(Source::Key, kEscape, true);
    require(input.active(Action::Start) && !input.active(Action::B), "in play Escape is Start, so it opens the pause menu");
}

void mainPage() {
    Rig rig;
    require(rig.page() == PausePage::Status, "the pause screen shows its main page");
    rig.escape();
    require(rig.menu.closing(), "Escape on the main page closes the pause screen like B");
    require(rig.menu.takeSounds() == std::vector<int>{kPauseSoundBack, kRoundMenuSoundClose}, "with the back sound and the close sound of B");
    rig.run(16);
    require(rig.menu.finished(), "and the level goes on");
}

void optionsPage() {
    Rig rig;
    rig.toOptions();
    rig.escape();
    require(!rig.menu.closing(), "Escape on the Options page does not close the pause screen");
    rig.run(16);
    require(rig.page() == PausePage::Status && !rig.menu.closing(), "it goes back one page, to the main page");
    require(rig.menu.round().selected() == 1, "on the Options entry, as B does");
    rig.run(20);
    rig.escape();
    require(rig.menu.closing(), "a second Escape closes the pause screen");
}

void languagesPage() {
    Rig rig;
    rig.toLanguages();
    rig.escape();
    require(!rig.menu.closing() && rig.menu.grid().state() == runtime::ItemGridState::Closing, "Escape on the Languages page closes the grid like B");
    require(rig.menu.takeSounds() == std::vector<int>{runtime::kLanguagesSoundBack}, "with the back sound");
    rig.run(16);
    require(rig.page() == PausePage::Options && !rig.menu.takeSaveRequest(), "back on the Options page, no save");
    rig.run(25);
    rig.escape();
    rig.run(16);
    require(rig.page() == PausePage::Status && !rig.menu.closing(), "Escape again goes to the main page");
    rig.run(20);
    rig.escape();
    require(rig.menu.closing(), "and one more closes the screen");
}

void heldKeyKeepsItsButton() {
    Rig rig;
    rig.toOptions();
    rig.escapeDown();
    require(rig.input.active(Action::B) && !rig.input.active(Action::Start), "Escape pressed on the Options page is B");
    rig.run(16);
    require(rig.page() == PausePage::Status, "the page went back while the key is held");
    runtime::routeEscape(rig.input, kEscape, rig.page());
    require(rig.input.active(Action::B) && !rig.input.active(Action::Start), "a held key stays B after the page changed");
    rig.escapeUp();
    require(!rig.input.active(Action::B) && !rig.input.active(Action::Start), "releasing it releases B");
}

void otherKeysAreNotRouted() {
    InputState input(Rig::bindings());
    runtime::bindEscape(input, kEscape);
    runtime::routeEscape(input, kReturn, PausePage::Options);
    input.event(Source::Key, kReturn, true);
    require(input.active(Action::Start) && !input.active(Action::B), "a key of the controls file keeps its action on the Options page");
    input.event(Source::Key, kReturn, false);
    runtime::routeEscape(input, kEscape, PausePage::Languages);
    input.event(Source::Key, kEscape, true);
    require(input.active(Action::B), "Escape is B on the Languages page");
    input.event(Source::Key, kEscape, false);
    runtime::routeEscape(input, kEscape, PausePage::Status);
    input.event(Source::Key, kEscape, true);
    require(input.active(Action::B) && !input.active(Action::Start), "and B on the main page (user's answer of 2026-11-02)");
    input.reset();
    runtime::routeEscape(input, kEscape, PausePage::None);
    input.event(Source::Key, kEscape, true);
    require(input.active(Action::Start), "after a focus loss the next press is routed again");
}
}

int main() {
    try {
        inPlay();
        mainPage();
        optionsPage();
        languagesPage();
        heldKeyKeepsItsButton();
        otherKeysAreNotRouted();
    } catch (const std::exception& error) {
        std::cerr << "escape-pause-pages: " << error.what() << '\n';
        return 1;
    }
    std::cout << "escape-pause-pages: all checks passed\n";
    return 0;
}
