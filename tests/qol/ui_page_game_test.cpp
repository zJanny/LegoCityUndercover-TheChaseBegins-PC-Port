// Spec: docs/design/qol-settings-drawing.md entry 2026-10-30T21:56:04: answer 3 (volumes 0 to 100 in steps of 10, one original step each),
// answer 4 (Language selector, applies on close), answer 8 (Reset to defaults on the tab, asks first), answer 11 (Enter on a selector does nothing),
// "Row types" bullet (volume slider with a number, `< On >` toggle, arrow selector, reset button).
// Original values: volumes 0..10 in steps of 1, defaults sound 10 and music 8, languages 1..9 in the order English, French, Italian, German,
// Spanish, Dutch, Danish, Portuguese, Russian (tests/qol/game_tab_test.cpp header; src/save/save.hpp Language).
// Answer 22 (the user, 2026-11-01, "Approve"): Reset to defaults restores the language the game started with.
// Cases written by 20261101-qolui3-B on 2026-11-01.
// Quit game row (20261101-qolctl-B, 2026-11-01): docs/design/qol-controls.md entry 2026-11-01T13:50:44 (a "Quit game" row with Yes/No on the Game tab);
// it sits before "Reset to defaults" because every tab ends with the reset button (qol-settings-drawing.md 2026-10-30T21:56:04, row types).
#include "qol/ui_page_game.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using namespace qol::ui;

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

struct Fake : qol::GameOptions {
    int sfx = 10, music = 8, lang = 1;
    bool invert = false;
    int writes = 0, languagePages = 0;
    int sfxVolume() const override { return sfx; }
    int musicVolume() const override { return music; }
    bool invertCamera() const override { return invert; }
    int language() const override { return lang; }
    void setSfxVolume(int v) override { sfx = v; ++writes; }
    void setMusicVolume(int v) override { music = v; ++writes; }
    void setInvertCamera(bool v) override { invert = v; ++writes; }
    void openLanguages() override { ++languagePages; }
};

const std::vector<std::string> names{"English", "French", "Italian", "German", "Spanish", "Dutch", "Danish", "Portuguese", "Russian"};

float fakeWidth(std::string_view text, float pixelSize) { return 0.5f * pixelSize * static_cast<float>(text.size()); }

Fake& withLanguage(Fake& fake, int language) {
    fake.lang = language;
    return fake;
}

struct Driver {
    Fake fake;
    GamePage page;
    UiNav nav;
    GamePageResult last;
    int actions = 0;

    explicit Driver(int startLanguage = qol::firstLanguage, int width = 1920, int height = 1080)
        : page(withLanguage(fake, startLanguage), names, startLanguage), nav(fakeWidth) {
        nav.setWindow(width, height);
        nav.setTabs({"Game", "Display", "Controls", "Quality of life"});
        nav.setSliderSteps(10);
        nav.setRows(page.rows());
        nav.setDropdownInfo([this](int row) { return page.dropdown(row); });
    }

    void press(Key key) {
        last = {};
        actions = 0;
        for (const Action& action : nav.handle(keyEvent(key))) {
            ++actions;
            const GamePageResult result = page.handle(action);
            if (result.changed) last.changed = true;
            if (result.languageChosen) last.languageChosen = result.languageChosen;
            if (result.needsConfirm) last.needsConfirm = result.needsConfirm;
            if (result.quitAsked) last.quitAsked = true;
        }
        nav.setRows(page.rows());
    }

    void down(int count) {
        for (int i = 0; i < count; ++i) press(Key::Down);
    }
};

void originalDefaultsUnchanged() {
    Driver d;
    require(d.fake.writes == 0 && d.fake.sfx == 10 && d.fake.music == 8 && !d.fake.invert && d.fake.lang == 1,
        "building the page and its rows writes nothing and leaves the original defaults");
    require(defaultSfxVolume == 10 && defaultMusicVolume == 8, "default volumes are the original's 10 and 8");
    const auto rows = d.page.rows();
    require(rows.size() == 6, "six rows: two sliders, toggle, selector, quit, reset");
    require(rows[0].label == "SFX volume" && rows[0].kind == qol::ui::RowKind::Slider && rows[0].value == "100" && rows[0].fraction == 1.0,
        "SFX slider shows 100");
    require(rows[1].label == "Music volume" && rows[1].kind == qol::ui::RowKind::Slider && rows[1].value == "80" && rows[1].fraction == 0.8,
        "Music slider shows 80");
    require(rows[2].label == "Invert camera" && rows[2].kind == qol::ui::RowKind::Toggle && rows[2].value == "Off", "invert camera Off");
    require(rows[3].label == "Language" && rows[3].kind == qol::ui::RowKind::Selector && rows[3].value == "English", "language English");
    require(rows[4].label == "Quit game" && rows[4].kind == qol::ui::RowKind::Button && rows[4].value == "Quit game", "quit button");
    require(rows[5].label == "Reset to defaults" && rows[5].kind == qol::ui::RowKind::Button, "reset button is the last row");
    for (const auto& row : rows) require(row.enabled, "all Game rows enabled");
    for (int row = 0; row < 6; ++row) require(d.page.dropdown(row).count == 0, "no dropdown on the Game page");
}

void rowsFollowTheStore() {
    Driver d;
    d.fake.sfx = 3;
    d.fake.music = 0;
    d.fake.invert = true;
    const auto rows = d.page.rows();
    require(rows[0].value == "30" && rows[1].value == "0" && rows[2].value == "On", "rows read the store, so the original page stays in sync");
}

void sliderStepsFrom0To100() {
    Driver d;
    for (int step = 9; step >= 0; --step) {
        d.press(Key::Left);
        require(d.last.changed && d.fake.sfx == step, "SFX Left lowers by one original step to " + std::to_string(step));
        require(d.page.rows()[0].value == std::to_string(step * 10), "SFX number shows " + std::to_string(step * 10));
    }
    d.press(Key::Left);
    require(d.actions == 0 && d.fake.sfx == 0, "no step below 0");
    for (int step = 1; step <= 10; ++step) {
        d.press(Key::Right);
        require(d.fake.sfx == step, "SFX Right raises to " + std::to_string(step));
    }
    d.press(Key::Right);
    require(d.actions == 0 && d.fake.sfx == 10 && d.page.rows()[0].value == "100", "no step above 10");
    d.down(1);
    d.press(Key::Right);
    require(d.fake.music == 9 && d.fake.sfx == 10 && d.page.rows()[1].value == "90", "Music slider is independent");
    require(d.fake.writes == 21, "one write per step");
}

void sliderByMouse() {
    Driver d;
    const qol::ui::UiLayout lay = d.nav.layout();
    const qol::Rect track = lay.rows[0].track;
    const int y = track.y + track.height / 2;
    d.last = {};
    for (const Action& action : d.nav.handle(mouseDown(track.x + track.width / 2, y))) d.last.changed = d.page.handle(action).changed;
    d.nav.handle(mouseUp(track.x + track.width / 2, y));
    require(d.last.changed && d.fake.sfx == 5, "a click in the middle of the track sets 50 (5 original steps)");
}

void invertCameraToggle() {
    Driver d;
    d.down(2);
    d.press(Key::Enter);
    require(d.last.changed && d.fake.invert && d.page.rows()[2].value == "On", "Enter turns invert camera on");
    d.press(Key::Right);
    require(!d.fake.invert && d.page.rows()[2].value == "Off", "Right flips it back");
    d.press(Key::Left);
    require(d.fake.invert, "Left flips it too");
}

void languageSelectorWraps() {
    Driver d;
    d.down(3);
    d.press(Key::Right);
    require(d.last.languageChosen == 2 && d.page.rows()[3].value == "French", "Right selects French and reports it");
    require(d.fake.lang == 1 && d.fake.languagePages == 0 && !d.last.changed,
        "the game's language is not touched before the menu closes");
    for (int i = 0; i < 7; ++i) d.press(Key::Right);
    require(d.page.language() == 9 && d.page.rows()[3].value == "Russian", "eight steps reach Russian");
    d.press(Key::Right);
    require(d.last.languageChosen == 1 && d.page.rows()[3].value == "English", "Right wraps from the last language to the first");
    d.press(Key::Left);
    require(d.last.languageChosen == 9 && d.page.rows()[3].value == "Russian", "Left wraps from the first language to the last");
    d.press(Key::Enter);
    require(d.actions == 0 && d.page.language() == 9 && d.fake.languagePages == 0, "Enter on the selector does nothing (answer 11)");
}

void languageByMouse() {
    Driver d;
    const qol::ui::UiLayout lay = d.nav.layout();
    const qol::Rect rightArrow = lay.rows[3].rightArrow;
    d.last = {};
    for (const Action& action : d.nav.handle(mouseDown(rightArrow.x + rightArrow.width / 2, rightArrow.y + rightArrow.height / 2)))
        d.last = d.page.handle(action);
    require(d.last.languageChosen == 2, "a click on the right arrow selects the next language");
}

void resetAsksThenResets() {
    Driver d(3);
    require(d.page.language() == 3 && d.page.rows()[3].value == "Italian", "the page starts on the game's language");
    d.fake.sfx = 2;
    d.fake.music = 9;
    d.fake.invert = true;
    d.down(3);
    d.press(Key::Right);
    require(d.last.languageChosen == 4, "the selector moved to German first");
    d.down(2);
    d.press(Key::Enter);
    require(d.last.needsConfirm && *d.last.needsConfirm == "Reset the Game settings?", "the reset button asks first");
    require(!d.last.changed && d.fake.sfx == 2 && d.fake.music == 9 && d.fake.invert, "asking changes nothing");
    const GamePageResult result = d.page.reset();
    require(result.changed && d.fake.sfx == 10 && d.fake.music == 8 && !d.fake.invert, "reset restores volumes and invert camera");
    require(result.languageChosen == 3 && d.page.language() == 3 && d.page.rows()[3].value == "Italian",
        "reset reports and shows the language the game started with (answer 22)");
    const GamePageResult again = d.page.reset();
    require(!again.changed && !again.languageChosen && !again.needsConfirm, "a second reset changes nothing");
}

void quitAsksFirst() {
    Driver d;
    d.down(4);
    d.press(Key::Enter);
    require(d.last.needsConfirm && *d.last.needsConfirm == "Quit the game?" && d.last.quitAsked && !d.last.changed,
        "Enter on Quit game asks first and changes nothing");
    require(d.fake.writes == 0 && d.page.rows()[4].label == "Quit game", "the stores are untouched and the row stays");
    const GamePageResult reset = d.page.reset();
    require(!reset.quitAsked, "a reset never asks to quit");
    d.press(Key::Down);
    d.press(Key::Enter);
    require(d.last.needsConfirm && *d.last.needsConfirm == "Reset the Game settings?" && !d.last.quitAsked, "the reset row asks its own question");
    require(d.page.help(4) == "Closes the game after you confirm.", "help text of the Quit game row");
    for (const Key key : {Key::Left, Key::Right}) {
        Driver e;
        e.down(4);
        e.press(key);
        require(e.actions == 0 && !e.last.quitAsked, "Left and Right on the Quit game button do nothing");
    }
}

void quitOnlyPage() {
    GamePage page;
    const auto rows = page.rows();
    require(page.quitOnly() && rows.size() == 1 && rows[0].label == "Quit game" && rows[0].kind == qol::ui::RowKind::Button,
        "without game options the page is the Quit game row alone (in play)");
    Action activate;
    activate.kind = ActionKind::Activate;
    activate.index = 0;
    const GamePageResult asked = page.handle(activate);
    require(asked.quitAsked && asked.needsConfirm && *asked.needsConfirm == "Quit the game?" && !asked.changed, "its row asks to quit");
    Action change;
    change.kind = ActionKind::ValueChanged;
    change.index = 0;
    change.direction = 1;
    const GamePageResult none = page.handle(change);
    require(!none.changed && !none.quitAsked && !none.languageChosen, "a value change does nothing");
    const GamePageResult reset = page.reset();
    require(!reset.changed && !reset.languageChosen && !reset.quitAsked, "there is nothing to reset");
    require(page.help(0) == "Closes the game after you confirm.", "help of the row");
}

void rejections() {
    Fake fake;
    bool threw = false;
    try { GamePage page(fake, {"English"}, 1); } catch (const std::invalid_argument&) { threw = true; }
    require(threw, "a wrong number of language names is rejected");
    threw = false;
    try { GamePage page(fake, names, 10); } catch (const std::invalid_argument&) { threw = true; }
    require(threw, "a start language outside 1 to 9 is rejected");
    threw = false;
    fake.lang = 0;
    try { GamePage page(fake, names, 1); } catch (const std::invalid_argument&) { threw = true; }
    require(threw, "a stored language outside 1 to 9 is rejected");
    fake.lang = 1;
    GamePage page(fake, names, 1);
    Action action;
    action.kind = ActionKind::ValueChanged;
    for (const int index : {4, 5}) {
        action.index = index;
        const GamePageResult result = page.handle(action);
        require(!result.changed && !result.quitAsked && !result.needsConfirm && fake.writes == 0, "a value change on a button row is ignored");
    }
    action.kind = ActionKind::DropdownSelected;
    action.index = 0;
    action.item = 0;
    require(!page.handle(action).changed && fake.writes == 0, "a dropdown selection is ignored");
}

void sizes() {
    const int sizes[][2] = {{1280, 720}, {1920, 1080}, {3840, 2160}};
    for (const auto& size : sizes) {
        Driver d(qol::firstLanguage, size[0], size[1]);
        d.press(Key::Left);
        require(d.fake.sfx == 9, "slider key step at " + std::to_string(size[1]) + "p");
        const qol::ui::UiLayout lay = d.nav.layout();
        const qol::Rect track = lay.rows[1].track;
        for (const Action& action : d.nav.handle(mouseDown(track.x + track.width - 1, track.y + track.height / 2))) d.page.handle(action);
        d.nav.handle(mouseUp(track.x, track.y));
        require(d.fake.music == 10, "slider click at the end of the track at " + std::to_string(size[1]) + "p");
    }
}

}

int main() {
    originalDefaultsUnchanged();
    rowsFollowTheStore();
    sliderStepsFrom0To100();
    sliderByMouse();
    invertCameraToggle();
    languageSelectorWraps();
    languageByMouse();
    resetAsksThenResets();
    quitAsksFirst();
    quitOnlyPage();
    rejections();
    sizes();
    return failures == 0 ? 0 : 1;
}
