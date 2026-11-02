// The View distance slider of the Display tab driven through the menu controller and the stores, by key and mouse.
// Spec: docs/design/qol-view-distance.md entry 2026-11-02T18:43:07 (the user's decisions of 2026-11-02): Display tab, x1.0 to x20.0 (0.25 steps to x4.0, whole steps above)
// shown as "x1.0", always live in every preset except Original, which shows x1.0 greyed out and skipped by Up and Down and keeps
// the stored value, Reset to defaults on the Display tab sets x1.0, `view_distance <factor>` in config/qol.cfg. Menu keys and the reset
// question: docs/design/qol-settings-drawing.md entry 2026-10-30T21:56:04 (answers 14 and 33). Cases written by 20261102-qolmenu-E on
// 2026-11-02; display modes and the window size are test fixtures.
#include "qol/active_settings.hpp"
#include "qol/menu_stores.hpp"
#include "qol/ui_menu.hpp"
#include "qol/view_distance.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace {

using namespace qol::ui;

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

float measure(std::string_view text, float pixels) { return static_cast<float>(text.size()) * pixels * 0.5f; }
float lineHeight(float pixels) { return pixels * 1.2f; }

std::string slurp(const std::string& path) {
    std::ifstream file(path);
    return std::string(std::istreambuf_iterator<char>(file), {});
}

constexpr int viewRow = 2;
constexpr int resetRow = 3;

struct Menu {
    std::string path;
    std::vector<qol::Settings> applied;
    qol::StoresPorts ports;
    std::unique_ptr<qol::MenuStores> stores;
    UiMenu menu{measure, lineHeight};

    Menu() {
        const auto folder = std::filesystem::temp_directory_path() / "chase-qol-view-distance-menu";
        std::filesystem::remove_all(folder);
        std::filesystem::create_directories(folder);
        path = (folder / "qol.cfg").string();
        qol::setActiveSettings(qol::Settings{});
        ports.settingsPath = path;
        ports.modes = {{800, 600}, {1280, 720}, {1920, 1080}};
        ports.desktop = {1920, 1080};
        ports.applyWindow = [this](const qol::Settings& settings) { applied.push_back(settings); };
        stores = std::make_unique<qol::MenuStores>(ports);
        stores->open();
        menu.setWindow(1920, 1080);
        menu.open(menuSetupFor(*stores, {}));
    }
    qol::MenuEffects send(const Event& event) {
        const qol::MenuEffects effects = menu.handle(event, 0);
        stores->commit(effects);
        return effects;
    }
    qol::MenuEffects press(Key key) { return send(keyEvent(key)); }
    void go(const std::string& title) {
        for (int i = 0; i < 4 && menu.tabTitles()[static_cast<std::size_t>(menu.screen().selectedTab)] != title; ++i) press(Key::E);
    }
    const RowSpec& row(int index) const { return menu.screen().rows[static_cast<std::size_t>(index)]; }
    qol::MenuEffects clickTrack(double fraction) {
        const RowLayout layout = computeLayout(menu.screen(), measure).rows[viewRow];
        const int span = layout.track.width - layout.thumb.width;
        const int x = layout.track.x + layout.thumb.width / 2 + static_cast<int>(fraction * span);
        const int y = layout.track.y + layout.track.height / 2;
        qol::MenuEffects effects = send(mouseDown(x, y));
        effects.merge(send(mouseUp(x, y)));
        return effects;
    }
    void setMaster(qol::Master master) {
        go("Quality of life");
        for (int i = 0; i < 3 && stores->settings().master() != master; ++i) press(Key::Right);
        go("Display");
    }
};

void originalShowsOneGreyedOut() {
    Menu m;
    m.go("Display");
    require(m.menu.tabTitles() == std::vector<std::string>{"Display", "Quality of life"}, "fixture: the Display and Quality of life tabs");
    require(m.row(viewRow).label == "View distance" && m.row(viewRow).kind == RowKind::Slider, "the Display tab has the View distance slider");
    require(m.row(viewRow).value == "x1.0" && !m.row(viewRow).enabled && m.row(viewRow).fraction == 0.0, "Original: x1.0, greyed out");
    require(m.row(resetRow).label == "Reset to defaults", "the slider sits above Reset to defaults");
    m.press(Key::Down);
    require(m.menu.screen().focusRow == resetRow, "Down skips the greyed-out Resolution and View distance rows");
    m.press(Key::Up);
    require(m.menu.screen().focusRow == 0, "Up skips them back");
    const qol::MenuEffects clicked = m.clickTrack(1.0);
    require(!clicked.settingsChanged && m.stores->settings().viewDistanceStep() == 0, "a click on the greyed-out slider changes nothing");
    require(qol::viewDistance(260.0f) == 260.0f, "the far plane stays 260");
}

void keysMoveInQuarterSteps() {
    Menu m;
    m.setMaster(qol::Master::Improved);
    require(qol::activeSettings().master() == qol::Master::Improved, "fixture: Improved");
    require(m.row(viewRow).enabled && m.row(viewRow).value == "x1.0", "Improved: the slider is live at x1.0");
    m.press(Key::Down);
    require(m.menu.screen().focusRow == viewRow, "Down reaches the slider (Resolution is greyed out in full screen)");
    const qol::MenuEffects step = m.press(Key::Right);
    require(step.settingsChanged && !step.windowChanged && m.applied.empty(), "Right changes the value and leaves the window alone");
    require(m.row(viewRow).value == "x1.25" && m.stores->settings().viewDistanceStep() == 1, "Right moves one quarter step to x1.25");
    require(slurp(m.path).find("view_distance 1.25\n") != std::string::npos, "qol.cfg has view_distance 1.25");
    require(qol::viewDistance(260.0f) == 325.0f, "the active store applies at once: far plane 325");
    for (int i = 0; i < 11; ++i) m.press(Key::Right);
    require(m.row(viewRow).value == "x4.0" && m.stores->settings().viewDistanceStep() == 12, "twelve steps reach x4.0");
    m.press(Key::Right);
    require(m.row(viewRow).value == "x5.0" && slurp(m.path).find("view_distance 5.0\n") != std::string::npos, "above x4.0 one step is a whole unit");
    for (int i = 0; i < 15; ++i) m.press(Key::Right);
    require(m.row(viewRow).value == "x20.0" && m.row(viewRow).fraction == 1.0 && m.stores->settings().viewDistanceStep() == 28,
        "28 presses reach x20.0");
    require(qol::viewDistance(260.0f) == 5200.0f, "x20.0: far plane 5200");
    require(!m.press(Key::Right).settingsChanged && m.row(viewRow).value == "x20.0", "Right at x20.0 stays");
    m.press(Key::Left);
    require(m.row(viewRow).value == "x19.0" && slurp(m.path).find("view_distance 19.0\n") != std::string::npos, "Left goes back to x19.0");
    for (int i = 0; i < 15; ++i) m.press(Key::Left);
    require(m.row(viewRow).value == "x4.0", "Left steps back to x4.0");
    m.press(Key::Left);
    require(m.row(viewRow).value == "x3.75", "and on in quarter steps");
}

void mouseSetsTheValue() {
    Menu m;
    m.setMaster(qol::Master::Improved);
    qol::MenuEffects clicked = m.clickTrack(1.0);
    require(clicked.settingsChanged && m.row(viewRow).value == "x20.0", "a click at the right end gives x20.0");
    clicked = m.clickTrack(0.5);
    require(clicked.settingsChanged && m.row(viewRow).value == "x6.0" && m.stores->settings().viewDistanceStep() == 14, "a click in the middle gives step 14, x6.0");
    clicked = m.clickTrack(0.0);
    require(clicked.settingsChanged && m.row(viewRow).value == "x1.0", "a click at the left end gives x1.0");
    require(slurp(m.path).find("view_distance") == std::string::npos, "x1.0 writes no view_distance line");
    require(m.applied.empty(), "the slider never applies the window");
}

void presetsKeepTheValue() {
    Menu m;
    m.setMaster(qol::Master::Improved);
    m.clickTrack(9.0 / 28.0);
    require(m.row(viewRow).value == "x3.25" && qol::viewDistance(260.0f) == 845.0f, "fixture: x3.25 in Improved");
    m.setMaster(qol::Master::Original);
    require(m.row(viewRow).value == "x1.0" && !m.row(viewRow).enabled, "Original shows x1.0 greyed out");
    require(m.stores->settings().viewDistanceStep() == 9 && slurp(m.path).find("view_distance 3.25\n") != std::string::npos,
        "Original keeps the stored x3.25");
    require(qol::viewDistance(260.0f) == 260.0f, "Original always uses x1.0");
    m.setMaster(qol::Master::Improved);
    require(m.row(viewRow).value == "x3.25" && m.row(viewRow).enabled, "Improved shows the stored value again");
}

void resetSetsOne() {
    Menu m;
    m.setMaster(qol::Master::Improved);
    m.clickTrack(1.0);
    m.press(Key::Down);
    m.press(Key::Down);
    require(m.menu.screen().focusRow == resetRow, "fixture: the reset button has the focus");
    m.press(Key::Enter);
    require(m.menu.modal() == Modal::Dialog && m.stores->settings().viewDistanceStep() == 28, "the reset asks first and changes nothing yet");
    m.press(Key::Left);
    const qol::MenuEffects reset = m.press(Key::Enter);
    require(reset.settingsChanged && m.row(viewRow).value == "x1.0" && m.stores->settings().viewDistanceStep() == 0, "Yes sets x1.0");
    require(slurp(m.path).find("view_distance") == std::string::npos && qol::viewDistance(260.0f) == 260.0f, "the reset reaches qol.cfg and the hooks");
    require(m.menu.modal() == Modal::None && m.applied.empty(), "no keep-settings question while the window keeps its values");
    require(qol::activeSettings().master() == qol::Master::Improved, "the reset leaves the preset alone");

    Menu original;
    original.setMaster(qol::Master::Improved);
    original.clickTrack(1.0);
    original.setMaster(qol::Master::Original);
    original.press(Key::Down);
    original.press(Key::Enter);
    original.press(Key::Left);
    original.press(Key::Enter);
    require(original.stores->settings().viewDistanceStep() == 0, "the reset in Original also sets the stored value back to x1.0");
}

}

int main() {
    originalShowsOneGreyedOut();
    keysMoveInQuarterSteps();
    mouseSetsTheValue();
    presetsKeepTheValue();
    resetSetsOne();
    qol::setActiveSettings(qol::Settings{});
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
