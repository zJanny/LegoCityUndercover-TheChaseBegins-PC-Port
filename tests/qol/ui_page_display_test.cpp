// Spec: docs/design/qol-settings-drawing.md entry 2026-10-30T21:56:04: "Row types" bullet (Window mode arrow selector, resolution as a dropdown of the
// detected modes), "Display changes" bullet (a window mode or resolution change asks Keep these settings), answer 8 (Reset to defaults: window mode
// and resolution, with the keep-settings countdown, asks first), answer 9 (resolution greyed out in full screen, "Desktop resolution"), answer 11
// (Enter on a selector does nothing), answer 14 (greyed rows skipped by Up and Down), answer 23 (Windowed and Fullscreen only, wrapping),
// answer 24 (the resolution already in use opens no keep-settings dialog).
// Defaults: window mode fullscreen and no stored resolution (src/qol/settings.hpp defaultWindowMode; tests/qol/display_tab_test.cpp).
// Cases written by 20261101-qolui3-B on 2026-11-01. The resolution list and desktop size are test fixtures.
#include "qol/ui_page_display.hpp"

#include <iostream>
#include <sstream>
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

const qol::ResolutionChoices fixture{{800, 600}, {1280, 720}, {1920, 1080}};
const qol::Resolution desktop{1920, 1080};

float fakeWidth(std::string_view text, float pixelSize) { return 0.5f * pixelSize * static_cast<float>(text.size()); }

struct Driver {
    qol::Settings settings;
    DisplayPage page;
    UiNav nav;
    DisplayPageResult last;
    int actions = 0;
    std::vector<ActionKind> kinds;

    explicit Driver(int width = 1920, int height = 1080)
        : page(settings, fixture, desktop), nav(fakeWidth) {
        nav.setWindow(width, height);
        nav.setTabs({"Game", "Display", "Controls", "Quality of life"});
        nav.setRows(page.rows());
        nav.setDropdownInfo([this](int row) { return page.dropdown(row); });
    }

    void absorb(const std::vector<Action>& list) {
        for (const Action& action : list) {
            ++actions;
            kinds.push_back(action.kind);
            const DisplayPageResult result = page.handle(action);
            if (result.changed) last.changed = true;
            if (result.needsKeepSettings) last.needsKeepSettings = result.needsKeepSettings;
            if (result.needsConfirm) last.needsConfirm = result.needsConfirm;
        }
        if (nav.screen().openDropdownRow < 0) nav.setRows(page.rows());
    }

    void press(Key key) {
        last = {};
        actions = 0;
        kinds.clear();
        absorb(nav.handle(keyEvent(key)));
    }
};

std::string written(const qol::Settings& settings) {
    std::ostringstream out;
    qol::writeSettings(out, settings);
    return out.str();
}

void originalDefaultsUnchanged() {
    Driver d;
    require(written(d.settings) == written(qol::Settings{}), "building the page and its rows leaves the settings untouched");
    require(d.settings.windowMode() == qol::WindowMode::Fullscreen && !d.settings.resolution(), "defaults: fullscreen, no stored resolution");
    const auto rows = d.page.rows();
    require(rows.size() == 3, "three rows: window mode, resolution, reset");
    require(rows[0].label == "Window mode" && rows[0].kind == qol::ui::RowKind::Selector && rows[0].value == "Fullscreen" && rows[0].enabled,
        "window mode selector shows Fullscreen");
    require(rows[1].label == "Resolution" && rows[1].kind == qol::ui::RowKind::Dropdown, "resolution dropdown row");
    require(rows[2].label == "Reset to defaults" && rows[2].kind == qol::ui::RowKind::Button && rows[2].enabled, "reset button");
}

void resolutionGreyInFullscreen() {
    Driver d;
    const auto rows = d.page.rows();
    require(!rows[1].enabled && rows[1].value == "Desktop resolution", "full screen: resolution disabled, reads Desktop resolution (answer 9)");
    d.press(Key::Down);
    require(d.nav.screen().focusRow == 2, "Down jumps over the greyed resolution row (answer 14)");
    d.press(Key::Up);
    require(d.nav.screen().focusRow == 0, "Up jumps back over it");
    d.settings.setResolution({1280, 720});
    require(d.page.rows()[1].value == "Desktop resolution" && !d.page.rows()[1].enabled, "a stored size is hidden in full screen");
}

void windowModeSelector() {
    Driver d;
    d.press(Key::Right);
    require(d.settings.windowMode() == qol::WindowMode::Windowed && d.page.rows()[0].value == "Windowed", "Right changes to Windowed");
    require(d.last.needsKeepSettings && d.last.changed, "a window mode change asks to keep the settings");
    if (d.last.needsKeepSettings) {
        const KeepSettingsChange& change = *d.last.needsKeepSettings;
        require(change.previous.windowMode == qol::WindowMode::Fullscreen && !change.previous.resolution &&
                change.current.windowMode == qol::WindowMode::Windowed && !change.current.resolution,
            "the outcome carries the old and the new values");
    }
    const auto rows = d.page.rows();
    require(rows[1].enabled && rows[1].value == "1920x1080", "windowed: resolution enabled, shows the desktop size while none is stored");
    d.press(Key::Left);
    require(d.settings.windowMode() == qol::WindowMode::Fullscreen && d.last.needsKeepSettings, "the two values wrap in both directions");
    d.press(Key::Left);
    require(d.settings.windowMode() == qol::WindowMode::Windowed, "Left wraps from the first to the last value");
    d.press(Key::Enter);
    require(d.actions == 0 && d.settings.windowMode() == qol::WindowMode::Windowed && !d.last.needsKeepSettings,
        "Enter on the selector does nothing (answer 11)");
}

void dropdownListAndSelection() {
    Driver d;
    d.press(Key::Right);
    require(d.page.dropdown(0).count == 0, "no dropdown on the Window mode row");
    require(d.page.dropdown(2).count == 0, "no dropdown on the reset button");
    require(d.page.dropdown(1).count == 3 && d.page.dropdown(1).selected == 2, "the list has the detected modes, the desktop size selected");
    require((d.page.dropdownItems(1) == std::vector<std::string>{"800x600", "1280x720", "1920x1080"}) && d.page.dropdownItems(0).empty(),
        "the list items are the detected modes");
    d.press(Key::Down);
    require(d.nav.screen().focusRow == 1, "the resolution row takes the focus when windowed");
    d.press(Key::Enter);
    require(d.nav.screen().openDropdownRow == 1 && d.nav.dropdownFocus() == 2 && !d.last.needsKeepSettings, "Enter opens the list on the current mode");
    d.press(Key::Up);
    d.press(Key::Up);
    d.press(Key::Enter);
    require(d.settings.resolution() == std::optional<qol::Resolution>{{800, 600}} && d.page.rows()[1].value == "800x600",
        "choosing the first item stores 800x600");
    require(d.last.needsKeepSettings && d.last.needsKeepSettings->previous.resolution == std::nullopt &&
            d.last.needsKeepSettings->current.resolution == std::optional<qol::Resolution>{{800, 600}} &&
            d.last.needsKeepSettings->previous.windowMode == qol::WindowMode::Windowed,
        "a resolution change asks to keep the settings with the old and the new size");
    d.press(Key::Enter);
    d.press(Key::Enter);
    require(d.settings.resolution() == std::optional<qol::Resolution>{{800, 600}} && !d.last.needsKeepSettings, "choosing the current mode again changes nothing");
    d.press(Key::Down);
    d.press(Key::Enter);
    d.press(Key::Escape);
    require(d.nav.screen().openDropdownRow < 0 && !d.last.needsKeepSettings, "Esc closes the list without a change");
    require(d.page.dropdown(1).selected == 0, "the stored size is the selected item");
    d.settings.setResolution({1000, 500});
    require(d.page.dropdown(1).selected == 0 && d.page.rows()[1].value == "1000x500", "a stored size outside the list shows as it is and selects the first item");
}

void resetAsksThenResets() {
    Driver d;
    d.press(Key::Right);
    d.settings.setResolution({1280, 720});
    d.press(Key::Down);
    d.press(Key::Down);
    d.press(Key::Enter);
    require(d.last.needsConfirm && *d.last.needsConfirm == "Reset the Display settings?", "the reset button asks first");
    require(d.settings.windowMode() == qol::WindowMode::Windowed && d.settings.resolution(), "asking changes nothing");
    const DisplayPageResult result = d.page.reset();
    require(d.settings.windowMode() == qol::WindowMode::Fullscreen && !d.settings.resolution(), "reset restores fullscreen and the desktop size");
    require(result.changed && result.needsKeepSettings, "reset goes through the keep-settings outcome");
    if (result.needsKeepSettings) {
        require(result.needsKeepSettings->previous.windowMode == qol::WindowMode::Windowed &&
                result.needsKeepSettings->previous.resolution == std::optional<qol::Resolution>{{1280, 720}} &&
                result.needsKeepSettings->current == DisplayState{},
            "the reset outcome carries the old values and the defaults");
    }
    const DisplayPageResult again = d.page.reset();
    require(!again.changed && !again.needsKeepSettings && !again.needsConfirm, "a second reset changes nothing and asks nothing");
    require(written(d.settings) == written(qol::Settings{}), "the settings are the defaults again");
}

void resetOnlyTouchesDisplay() {
    Driver d;
    d.settings.setMaster(qol::Master::Improved);
    d.settings.setResolution({800, 600});
    d.page.reset();
    require(d.settings.master() == qol::Master::Improved, "reset leaves the Quality of life state alone");
}

void rejections() {
    bool threw = false;
    qol::Settings settings;
    try { DisplayPage page(settings, {{399, 240}}, desktop); } catch (const std::invalid_argument&) { threw = true; }
    require(threw, "an invalid resolution list is rejected");
    DisplayPage page(settings, fixture, desktop);
    Action action;
    action.kind = ActionKind::DropdownSelected;
    action.index = 1;
    action.item = 3;
    require(!page.handle(action).changed && !settings.resolution(), "an item outside the list is ignored");
    action.item = -1;
    require(!page.handle(action).changed, "a negative item is ignored");
    action.index = 0;
    action.item = 0;
    require(!page.handle(action).changed, "a dropdown selection on another row is ignored");
    action.kind = ActionKind::ValueChanged;
    action.index = 1;
    action.direction = 1;
    require(!page.handle(action).changed && settings.windowMode() == qol::WindowMode::Fullscreen, "a value change on the dropdown row is ignored");
    action.kind = ActionKind::Activate;
    action.index = 1;
    require(!page.handle(action).needsConfirm, "activating the dropdown row asks nothing");
    qol::Settings other;
    DisplayPage emptyList(other, {}, qol::Resolution{0, 0});
    other.setWindowMode(qol::WindowMode::Windowed);
    require(emptyList.dropdown(1).count == 0 && emptyList.rows()[1].value == "Desktop resolution", "no detected modes: empty list, desktop text");
}

void sizes() {
    const int heights[][2] = {{1280, 720}, {1920, 1080}, {3840, 2160}};
    for (const auto& size : heights) {
        Driver d(size[0], size[1]);
        const qol::ui::UiLayout lay = d.nav.layout();
        const qol::Rect arrow = lay.rows[0].rightArrow;
        d.last = {};
        d.absorb(d.nav.handle(mouseDown(arrow.x + arrow.width / 2, arrow.y + arrow.height / 2)));
        require(d.settings.windowMode() == qol::WindowMode::Windowed && d.last.needsKeepSettings,
            "a click on the arrow changes the window mode at " + std::to_string(size[1]) + "p");
        const qol::Rect row = d.nav.layout().rows[1].rect;
        d.absorb(d.nav.handle(mouseDown(row.x + row.width - 5, row.y + row.height / 2)));
        require(d.nav.screen().openDropdownRow == 1, "a click on the resolution row opens the list at " + std::to_string(size[1]) + "p");
        const auto items = d.nav.layout().dropdown.items;
        require(items.size() == 3, "the popup shows the three modes");
        if (items.size() == 3) {
            d.last = {};
            d.absorb(d.nav.handle(mouseDown(items[1].x + 3, items[1].y + items[1].height / 2)));
            require(d.settings.resolution() == std::optional<qol::Resolution>{{1280, 720}} && d.last.needsKeepSettings,
                "a click on an item selects it at " + std::to_string(size[1]) + "p");
        }
    }
}

}

int main() {
    originalDefaultsUnchanged();
    resolutionGreyInFullscreen();
    windowModeSelector();
    dropdownListAndSelection();
    resetAsksThenResets();
    resetOnlyTouchesDisplay();
    rejections();
    sizes();
    return failures == 0 ? 0 : 1;
}
