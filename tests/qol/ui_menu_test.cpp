// The settings menu controller UiMenu (src/qol/ui_menu.*) driven with plain events, as the presenter feeds it.
// Spec: docs/design/qol-settings-drawing.md entry 2026-10-30T21:56:04 (layout, keys, mouse, controller, row types, Controls tab,
// display confirm) and its answers 4 (Language applies on close), 6 (tabs wrap, rows do not), 8 (Reset to defaults asks first),
// 13 (Enter cancels the Swap dialog, Swap needs a click or the focus), 17 (Cancel keeps the prompt open), 20 (countdown wording),
// 22 (reset restores the start language), 29 (painter values), 30 (Left/Right and Enter in a two-button question), 32 (the button text
// "Reset to defaults" on every tab) and 33 (the reset question opens on No); Yes / No of the reset question from
// docs/design/qol-settings.md entry 2026-10-29T11:06:51; the help texts of the Game, Display and Controls rows are the draft of QOLSET-97
// (not approved yet). Expectations written from the spec by 20261101-qolui3-F on 2026-11-01. The Quit game row: docs/design/qol-controls.md entry
// 2026-11-01T13:50:44 (Yes/No on the Game tab, Escape cancels), default focus No as for the reset question (answer 33); cases by 20261101-qolctl-B.
#include "qol/ui_menu.hpp"

#include "qol/active_settings.hpp"
#include "render/ui_font.hpp"

#include <algorithm>
#include <iostream>
#include <set>
#include <string>

namespace {

using namespace qol;
using namespace qol::ui;

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

float measure(std::string_view text, float pixels) { return static_cast<float>(text.size()) * pixels * 0.5f; }
float lineHeight(float pixels) { return pixels * 1.2f; }

struct FakeGame final : GameOptions {
    int sfx = 10, music = 8, lang = 1, languagePages = 0;
    bool invert = false;
    int sfxVolume() const override { return sfx; }
    int musicVolume() const override { return music; }
    bool invertCamera() const override { return invert; }
    int language() const override { return lang; }
    void setSfxVolume(int volume) override { sfx = volume; }
    void setMusicVolume(int volume) override { music = volume; }
    void setInvertCamera(bool value) override { invert = value; }
    void openLanguages() override { ++languagePages; }
};

const std::vector<std::string> languages{"English", "Francais", "Italiano", "Deutsch", "Espanol", "Nederlands", "Dansk", "Portugues", "Russkiy"};

std::vector<BindingEntry> bindings() {
    return {{"Up", "key", "W", "hold"}, {"Up", "key", "Up", "hold"}, {"A", "key", "Space", "hold"}, {"A", "mouse", "Left", "hold"}};
}

bool validInput(const std::string& source, const std::string& code) {
    static const std::set<std::string> keys{"W", "Up", "Space", "F", "G"};
    return (source == "key" && keys.count(code) != 0) || (source == "mouse" && (code == "Left" || code == "Right"));
}

struct Fixture {
    Settings settings;
    FakeGame game;
    ControlsTab tab{AnyInput{}, {"Up", "A"}, bindings(), bindings(), validInput};
    ControlsTabPort port{tab};
    UiMenu menu{measure, lineHeight};
    double now = 100;

    explicit Fixture(int width = 1920, int height = 1080, bool full = true) {
        menu.setWindow(width, height);
        MenuSetup setup;
        setup.settings = &settings;
        if (full) {
            setup.game = &game;
            setup.languageNames = languages;
            setup.startLanguage = 1;
            setup.controls = &port;
            setup.display = DisplaySetup{{{1280, 720}, {1920, 1080}, {3840, 2160}}, {1920, 1080}};
        }
        menu.open(setup);
    }

    MenuEffects key(Key k) { return menu.handle(keyEvent(k), now); }
    MenuEffects event(const Event& e) { return menu.handle(e, now); }
    const RowSpec& row(int index) const { return menu.screen().rows[static_cast<std::size_t>(index)]; }
    void goTo(const std::string& title) {
        for (int i = 0; i < 4 && menu.tabTitles()[static_cast<std::size_t>(menu.screen().selectedTab)] != title; ++i) key(Key::E);
    }
};

std::vector<std::string> texts(const Fixture& f) {
    chase::render::UiDrawList list;
    f.menu.paint(list);
    std::vector<std::string> out;
    for (const auto& command : list.commands())
        if (command.kind == chase::render::UiCommandKind::Text) out.push_back(command.text);
    return out;
}

bool has(const std::vector<std::string>& list, const std::string& text) { return std::find(list.begin(), list.end(), text) != list.end(); }

void originalModeFirst() {
    setActiveSettings(Settings{});
    Fixture f;
    require(f.settings.master() == Master::Original, "the menu opens over the default master Original");
    f.goTo("Quality of life");
    require(f.row(0).value == "Original", "the preset row shows Original");
    const MenuEffects closed = f.key(Key::Escape);
    require(closed.closed && !closed.settingsChanged && !closed.language && !f.menu.isOpen(), "closing without a change changes nothing");
    require(f.settings.master() == Master::Original && f.settings.windowMode() == defaultWindowMode && !f.settings.resolution() && f.game.sfx == 10 && f.game.music == 8 && !f.game.invert && f.tab.bindings() == bindings(),
        "every store keeps its value");
}

void opensOnTheGameTab() {
    Fixture f;
    require(f.menu.tabTitles() == std::vector<std::string>{"Game", "Display", "Controls", "Quality of life"}, "the four tabs in order");
    require(f.menu.screen().selectedTab == 0 && f.menu.screen().focusRow == 0, "the menu opens on the first row of Game");
    Fixture bare(1920, 1080, false);
    require(bare.menu.tabTitles() == std::vector<std::string>{"Quality of life"}, "a tab whose store is missing is not shown");
}

void tabsWrap() {
    Fixture f;
    f.key(Key::E);
    require(f.menu.screen().selectedTab == 1, "E switches to the next tab");
    f.key(Key::PageDown);
    f.key(Key::PadRB);
    require(f.menu.screen().selectedTab == 3, "PageDown and RB switch on");
    f.key(Key::E);
    require(f.menu.screen().selectedTab == 0, "E on the last tab goes to the first (answer 6)");
    f.key(Key::Q);
    require(f.menu.screen().selectedTab == 3, "Q on the first tab goes to the last");
    f.key(Key::PadLB);
    f.key(Key::PageUp);
    require(f.menu.screen().selectedTab == 1, "LB and PageUp switch back");
    require(f.menu.device() == InputDevice::Keyboard, "the last key was a keyboard key");
    f.key(Key::PadLB);
    require(f.menu.device() == InputDevice::Pad, "a pad button switches the hints to the pad");
}

void sliderAndToggle() {
    Fixture f;
    MenuEffects e = f.key(Key::Left);
    require(e.gameChanged && f.game.sfx == 9 && f.row(0).value == "90", "Left lowers the SFX slider by one step of 10");
    f.key(Key::Down);
    e = f.key(Key::Right);
    require(e.gameChanged && f.game.music == 9 && f.row(1).value == "90", "Right raises the Music slider");
    f.key(Key::Down);
    e = f.key(Key::Enter);
    require(e.gameChanged && f.game.invert && f.row(2).value == "On", "Enter flips Invert camera");
    for (int i = 0; i < 6; ++i) f.key(Key::Down);
    require(f.menu.screen().focusRow == 5, "Down on the last row stays (answer 6)");
}

void languageOnClose() {
    Fixture f;
    f.key(Key::Down);
    f.key(Key::Down);
    f.key(Key::Down);
    const MenuEffects e = f.key(Key::Right);
    require(f.row(3).value == "Francais" && f.game.lang == 1 && !e.language, "the Language selector keeps the choice until close");
    const MenuEffects closed = f.key(Key::Escape);
    require(closed.closed && closed.language == 2, "closing applies the chosen language (answer 4)");
}

void resetAsksFirst() {
    Fixture f;
    f.key(Key::Left);
    for (int i = 0; i < 5; ++i) f.key(Key::Down);
    f.key(Key::Enter);
    std::optional<MenuDialog> dialog = f.menu.dialog();
    require(f.menu.modal() == Modal::Dialog && dialog && dialog->title == gameResetQuestion &&
            dialog->buttons == std::vector<std::string>{"Yes", "No"},
        "Reset to defaults asks first with Yes and No");
    require(has(texts(f), "Yes") && has(texts(f), gameResetQuestion), "the question is painted");
    f.key(Key::Escape);
    require(f.menu.isOpen() && f.menu.modal() == Modal::None && f.game.sfx == 9, "Esc declines and closes only the question");
    f.key(Key::Enter);
    require(f.menu.dialog()->focusedButton == 1, "the question opens with the focus on No (answer 33)");
    MenuEffects e = f.key(Key::Enter);
    require(!e.gameChanged && f.game.sfx == 9 && f.menu.modal() == Modal::None, "Enter on No declines (answer 33)");
    f.key(Key::Enter);
    f.key(Key::Left);
    require(f.menu.dialog()->focusedButton == 0, "Left moves the focus to Yes");
    f.key(Key::Right);
    f.key(Key::Left);
    e = f.key(Key::Enter);
    require(e.gameChanged && f.game.sfx == 10 && f.menu.modal() == Modal::None, "Enter on Yes resets the tab");
}

void openQuitQuestion(Fixture& f) {
    for (int i = 0; i < 4; ++i) f.key(Key::Down);
    f.key(Key::Enter);
}

void quitGameOutcomes() {
    {
        Fixture f;
        require(f.row(4).label == "Quit game" && f.row(4).kind == RowKind::Button, "the Game tab has the Quit game row before Reset to defaults");
        openQuitQuestion(f);
        const std::optional<MenuDialog> dialog = f.menu.dialog();
        require(f.menu.modal() == Modal::Dialog && dialog && dialog->title == quitGameQuestion &&
                dialog->buttons == std::vector<std::string>{"Yes", "No"} && dialog->focusedButton == 1,
            "Quit game asks Quit the game? with Yes and No, focus on No");
        require(has(texts(f), quitGameQuestion), "the quit question is painted");
        const MenuEffects enter = f.key(Key::Enter);
        require(!enter.quitGame && f.menu.isOpen() && f.menu.modal() == Modal::None, "Enter on the default No does not quit");
    }
    {
        Fixture f;
        openQuitQuestion(f);
        f.key(Key::Left);
        const MenuEffects yes = f.key(Key::Enter);
        require(yes.quitGame && !yes.gameChanged && f.game.sfx == 10 && f.menu.modal() == Modal::None, "Left then Enter on Yes quits and changes no store");
    }
    {
        Fixture f;
        openQuitQuestion(f);
        const MenuEffects esc = f.key(Key::Escape);
        require(!esc.quitGame && !esc.closed && f.menu.isOpen() && f.menu.modal() == Modal::None, "Escape cancels the question and keeps the menu open");
        f.key(Key::Enter);
        require(f.menu.dialog() && f.menu.dialog()->focusedButton == 1, "the question opens on No again after Escape");
        const MenuEffects padB = f.key(Key::PadB);
        require(!padB.quitGame && f.menu.isOpen() && f.menu.modal() == Modal::None, "pad B cancels the question");
    }
    {
        Fixture f;
        openQuitQuestion(f);
        const DialogLayout d = computeDialogLayout(*f.menu.dialog(), computeScale(1920, 1080), measure, lineHeight);
        const Rect no = d.buttons.at(1), yes = d.buttons.at(0);
        const MenuEffects declined = f.event(mouseDown(no.x + no.width / 2, no.y + no.height / 2));
        require(!declined.quitGame && f.menu.modal() == Modal::None, "a click on No cancels");
        f.key(Key::Enter);
        require(f.menu.dialog() && f.menu.dialog()->title == quitGameQuestion, "Enter on the row asks again");
        const MenuEffects stale = f.key(Key::Enter);
        require(!stale.quitGame && f.menu.modal() == Modal::None, "a cancelled question leaves no pending quit behind");
        f.key(Key::Enter);
        const MenuEffects clicked = f.event(mouseDown(yes.x + yes.width / 2, yes.y + yes.height / 2));
        require(clicked.quitGame, "a click on Yes quits");
    }
    {
        Fixture f;
        f.game.sfx = 5;
        openQuitQuestion(f);
        f.key(Key::Escape);
        f.key(Key::Down);
        f.key(Key::Enter);
        f.key(Key::Left);
        const MenuEffects reset = f.key(Key::Enter);
        require(!reset.quitGame && reset.gameChanged, "the reset question after a cancelled quit question resets and does not quit");
    }
}

void quitTabInPlay() {
    Settings settings;
    UiMenu menu{measure, lineHeight};
    menu.setWindow(1920, 1080);
    MenuSetup setup;
    setup.settings = &settings;
    setup.quitTab = true;
    menu.open(setup);
    require(menu.tabTitles() == std::vector<std::string>{"Game", "Quality of life"}, "with quitTab and no game options the Game tab is shown (in play)");
    require(menu.screen().rows.size() == 1 && menu.screen().rows[0].label == "Quit game", "it holds the Quit game row alone");
    menu.handle(keyEvent(Key::Enter), 0);
    require(menu.dialog() && menu.dialog()->title == quitGameQuestion && menu.dialog()->focusedButton == 1, "Enter asks, focus on No");
    menu.handle(keyEvent(Key::Left), 0);
    const MenuEffects yes = menu.handle(keyEvent(Key::Enter), 0);
    require(yes.quitGame && !yes.gameChanged && menu.isOpen(), "Yes quits");
    const MenuEffects close = menu.handle(keyEvent(Key::Escape), 0);
    require(close.closed && !close.language && !close.quitGame, "Escape closes the menu without a language change or a quit");
    MenuSetup bare;
    bare.settings = &settings;
    menu.open(bare);
    require(menu.tabTitles() == std::vector<std::string>{"Quality of life"}, "without quitTab and game options there is no Game tab");
}

void controlsResetDeclinedByClick() {
    Fixture f;
    f.goTo("Controls");
    f.key(Key::Right);
    f.key(Key::Right);
    f.key(Key::Enter);
    const std::vector<BindingEntry> changed = f.tab.bindings();
    f.key(Key::Down);
    f.key(Key::Down);
    f.key(Key::Enter);
    require(changed != bindings() && f.menu.modal() == Modal::Dialog && f.tab.confirming(), "the Controls reset asks first");
    const DialogLayout d = computeDialogLayout(*f.menu.dialog(), computeScale(1920, 1080), measure, lineHeight);
    const Rect no = d.buttons.at(1);
    f.event(mouseDown(no.x + no.width / 2, no.y + no.height / 2));
    require(f.menu.modal() == Modal::None && !f.tab.confirming() && f.tab.bindings() == changed,
        "a click on No declines and clears the pending reset of the Controls tab");
}

void resetButtonText() {
    Fixture f;
    for (int tab = 0; tab < 4; ++tab) {
        const RowSpec& last = f.menu.screen().rows.back();
        require(last.label == "Reset to defaults" && last.kind == RowKind::Button && last.value == "Reset to defaults",
            "the last row of " + f.menu.tabTitles()[static_cast<std::size_t>(tab)] + " is the Reset to defaults button (answer 32)");
        f.key(Key::E);
    }
    require(has(texts(f), "Reset to defaults"), "the button text is painted");
}

void displayKeepSettings() {
    Fixture f;
    f.goTo("Display");
    require(!f.row(1).enabled && f.row(1).value == desktopResolutionText, "Resolution is greyed out in full screen (answer 9)");
    MenuEffects e = f.key(Key::Right);
    require(e.settingsChanged && e.windowChanged && f.settings.windowMode() == WindowMode::Windowed, "the window mode applies at once");
    std::optional<MenuDialog> dialog = f.menu.dialog();
    require(f.menu.modal() == Modal::Dialog && dialog && dialog->title == "Keep these settings?" && dialog->countdownText == "Reverting in 15 seconds",
        "Keep these settings? with its 15-second countdown");
    f.menu.tick(f.now + 14.2);
    require(f.menu.dialog()->countdownText == "Reverting in 1 second", "one second is singular (answer 20)");
    e = f.key(Key::Enter);
    require(f.menu.modal() == Modal::None && f.settings.windowMode() == WindowMode::Windowed && !e.windowChanged, "Enter keeps the settings");

    f.key(Key::Down);
    f.key(Key::Enter);
    require(f.menu.screen().openDropdownRow == 1, "Enter opens the resolution list");
    f.key(Key::Escape);
    require(f.menu.isOpen() && f.menu.screen().openDropdownRow < 0, "Esc closes only the list");
    f.key(Key::Enter);
    f.key(Key::Up);
    f.key(Key::Up);
    require(has(texts(f), "1280x720"), "the list items are painted");
    e = f.key(Key::Enter);
    require(e.windowChanged && f.settings.resolution() == Resolution{1280, 720} && f.menu.modal() == Modal::Dialog,
        "choosing a size applies it and asks to keep it");
    e = f.menu.tick(f.now + 15);
    require(e.windowChanged && e.settingsChanged && !f.settings.resolution() && f.menu.modal() == Modal::None,
        "the timeout reverts to the previous size");
}

void swapDialog() {
    Fixture f;
    f.goTo("Controls");
    require(f.menu.screen().columnTitles == std::vector<std::string>{"Action", "Slot 1", "Slot 2", "Mode"}, "the column titles");
    require(has(texts(f), "Slot 2") && has(texts(f), "W") && has(texts(f), "< Hold >"), "titles and cells are painted");
    f.key(Key::Right);
    f.key(Key::Enter);
    require(f.menu.modal() == Modal::Prompt && f.menu.dialog() && f.menu.dialog()->title == "Press a key... (Esc cancel, Del clear)",
        "a slot opens the prompt");
    f.event(promptInput(InputSource::Key, "Space"));
    std::optional<MenuDialog> dialog = f.menu.dialog();
    require(f.menu.modal() == Modal::SwapDialog && dialog && dialog->buttons == std::vector<std::string>{"Swap", "Cancel"} && dialog->focusedButton == 1,
        "a key of another action asks Swap or Cancel, Cancel focused");
    f.key(Key::Enter);
    require(f.menu.modal() == Modal::Prompt && f.tab.bindings() == bindings(), "Enter cancels and the prompt stays open (answers 13 and 17)");
    f.event(promptInput(InputSource::Key, "Space"));
    f.key(Key::Left);
    require(f.menu.dialog()->focusedButton == 0, "Left moves the focus to Swap");
    const MenuEffects e = f.key(Key::Enter);
    require(e.controlsChanged && f.menu.modal() == Modal::None && f.row(1).cells[1] == "Space" && f.row(2).cells[0] == "Up",
        "Swap exchanges the two bindings");
}

void swapByClick(int width, int height) {
    Fixture f(width, height);
    f.goTo("Controls");
    f.key(Key::Right);
    f.key(Key::Enter);
    f.event(promptInput(InputSource::Key, "Space"));
    const DialogLayout d = computeDialogLayout(*f.menu.dialog(), computeScale(width, height), measure, lineHeight);
    const Rect swap = d.buttons.at(0);
    f.event(mouseMove(swap.x + swap.width / 2, swap.y + swap.height / 2));
    require(f.menu.dialog()->focusedButton == 0, "hovering Swap focuses it at " + std::to_string(height));
    const MenuEffects e = f.event(mouseDown(swap.x + swap.width / 2, swap.y + swap.height / 2));
    require(e.controlsChanged && f.row(1).cells[1] == "Space", "a click on Swap swaps at " + std::to_string(height));
}

void helpTexts(int width, int height) {
    Fixture f(width, height);
    const std::string at = " at " + std::to_string(height);
    require(f.menu.helpText() == "Volume of the sound effects. Default: 100", "the SFX row names its default (draft QOLSET-97)" + at);
    for (int i = 0; i < 3; ++i) f.key(Key::Down);
    require(f.menu.helpText() == "Language of the game. It changes when you close the menu. Default: English",
        "the Language default is the start language (answer 22)" + at);
    const UiLayout layout = computeLayout(f.menu.screen(), measure);
    const float room = static_cast<float>(layout.helpLine.width - layout.scale.padding);
    for (int tab = 0; tab < 4; ++tab) {
        for (std::size_t row = 0; row < f.menu.screen().rows.size(); ++row) {
            const std::string help = f.menu.helpText();
            require(!help.empty() && chase::render::uiTextWidth(help, layout.scale.textPixels) <= room,
                "every row of " + f.menu.tabTitles()[static_cast<std::size_t>(tab)] + " has a help text that fits the help line in the bundled font" + at);
            f.key(Key::Down);
        }
        f.key(Key::E);
    }
}

void escLayers() {
    Fixture f;
    f.goTo("Controls");
    f.key(Key::Enter);
    f.key(Key::Escape);
    require(f.menu.isOpen() && f.menu.modal() == Modal::None, "Esc first cancels the prompt");
    const MenuEffects closed = f.key(Key::Escape);
    require(closed.closed && !f.menu.isOpen(), "then Esc closes the menu");
    Fixture pad;
    pad.key(Key::PadB);
    require(!pad.menu.isOpen(), "pad B goes back");
}

void mouse(int width, int height) {
    Fixture f(width, height);
    const std::string size = " at " + std::to_string(width) + "x" + std::to_string(height);
    const UiLayout layout = computeLayout(f.menu.screen(), measure);
    const Rect tab = layout.tabs.at(3).rect;
    f.event(mouseDown(tab.x + tab.width / 2, tab.y + tab.height / 2));
    require(f.menu.screen().selectedTab == 3 && f.menu.device() == InputDevice::Mouse, "a click on a tab switches" + size);
    const UiLayout qolLayout = computeLayout(f.menu.screen(), measure);
    const Rect value = qolLayout.rows.at(1).valueArea;
    f.event(mouseMove(value.x + value.width / 2, value.y + value.height / 2));
    require(f.menu.screen().focusRow == 1 && !f.menu.helpText().empty(), "hovering a row focuses it and shows its help" + size);
    const MenuEffects e = f.event(mouseDown(value.x + value.width / 2, value.y + value.height / 2));
    require(e.settingsChanged && f.settings.master() == Master::Custom, "a click on a feature toggle switches to Custom" + size);
    chase::render::UiDrawList list;
    f.menu.paint(list);
    require(!chase::render::buildUiBatch(list, width, height).runs.empty(), "the menu paints a batch" + size);
    const MenuEffects closed = f.event(rightClick(10, 10));
    require(closed.closed, "right-click goes back" + size);
}

}

int main() {
    originalModeFirst();
    opensOnTheGameTab();
    tabsWrap();
    sliderAndToggle();
    languageOnClose();
    resetAsksFirst();
    quitGameOutcomes();
    quitTabInPlay();
    controlsResetDeclinedByClick();
    resetButtonText();
    displayKeepSettings();
    swapDialog();
    escLayers();
    for (const auto& size : {std::pair<int, int>{1280, 720}, {1920, 1080}, {3840, 2160}}) {
        swapByClick(size.first, size.second);
        mouse(size.first, size.second);
        helpTexts(size.first, size.second);
    }
    if (failures == 0) std::cout << "qol-ui-menu: all checks passed\n";
    return failures == 0 ? 0 : 1;
}
