// On foot / Vehicle switch of the Controls tab (QOLCTL-13). Spec: docs/design/qol-controls.md, the user answer of
// 2026-11-02 "On foot / Vehicle switch" (a switch at the top of the Controls tab chooses the set the list shows and edits:
// controls.pc.cfg or controls.pc.vehicle.cfg; rebind, Reset, conflict check and file write act on the chosen set; with PC
// controls off the tab edits controls.cfg and has no switch). Cases written by 20261102-qolmenu-B on 2026-11-02. Driven
// through the same menu code as the settings menu.
#include "input.hpp"
#include "qol/active_settings.hpp"
#include "qol/controls_file.hpp"
#include "qol/menu_stores.hpp"
#include "qol/pc_controls.hpp"
#include "qol/pc_vehicle_controls.hpp"
#include "qol/ui_menu.hpp"
#include "runtime/qol_controls_bindings.hpp"
#include "runtime/qol_pc_controls.hpp"

#include <SDL.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iterator>
#include <string>
#include <vector>

namespace {

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

template <class F>
bool throws(F&& call) {
    try {
        call();
    } catch (const std::exception&) {
        return true;
    }
    return false;
}

const char* const faithfulText = "Up key W hold\nA key Space hold\nA mouse Left hold\nWalk key Left_Alt hold\n";

std::string slurp(const std::string& path) {
    std::ifstream file(path);
    return std::string(std::istreambuf_iterator<char>(file), {});
}

void spit(const std::string& path, const std::string& text) {
    std::ofstream(path, std::ios::binary) << text;
}

struct Fixture {
    std::string directory, qolPath, controlsPath, pcPath;
    int controlsChanged = 0;
    chase::InputState input{{}};
    qol::StoresPorts ports;

    explicit Fixture(const std::string& name) {
        auto folder = std::filesystem::temp_directory_path() / ("chase-qol-controls-set-switch-" + name);
        std::filesystem::remove_all(folder);
        std::filesystem::create_directories(folder);
        directory = folder.string();
        qolPath = directory + "/qol.cfg";
        controlsPath = directory + "/controls.cfg";
        pcPath = directory + "/controls.pc.cfg";
        spit(controlsPath, faithfulText);
        spit(directory + "/controls.default.cfg", faithfulText);
        qol::setActiveSettings(qol::Settings{});
        ports.settingsPath = qolPath;
        ports.modes = {{800, 600}, {1920, 1080}};
        ports.desktop = {1920, 1080};
        ports.applyWindow = [](const qol::Settings&) {};
        ports.makeControls = [this]() { return chase::runtime::makeActiveControls(controlsPath); };
        ports.controlsChanged = [this]() {
            ++controlsChanged;
            input.rebind(chase::runtime::readActiveBindings(controlsPath));
        };
    }
};

float measure(std::string_view text, float pixels) { return static_cast<float>(text.size()) * pixels * 0.5f; }
float lineHeight(float pixels) { return pixels * 1.2f; }

using qol::ui::Key;

struct Menu {
    qol::MenuStores stores;
    qol::ui::UiMenu menu{measure, lineHeight};

    explicit Menu(const qol::StoresPorts& ports) : stores(ports) {}
    void open() {
        stores.open();
        menu.setWindow(1920, 1080);
        menu.open(qol::ui::menuSetupFor(stores, {}));
    }
    qol::MenuEffects press(qol::ui::Key key) {
        const qol::MenuEffects effects = menu.handle(qol::ui::keyEvent(key), 0);
        stores.commit(effects);
        return effects;
    }
    qol::MenuEffects input(const std::string& code) {
        const qol::MenuEffects effects = menu.handle(qol::ui::promptInput(qol::ui::InputSource::Key, code), 0);
        stores.commit(effects);
        return effects;
    }
    void go(const std::string& title) {
        for (int i = 0; i < 4 && menu.tabTitles()[static_cast<std::size_t>(menu.screen().selectedTab)] != title; ++i) press(qol::ui::Key::E);
    }
    std::string focused() const {
        const int row = menu.screen().focusRow;
        return row < 0 ? std::string() : menu.screen().rows[static_cast<std::size_t>(row)].label;
    }
    void focus(const std::string& label) {
        if (menu.screen().focusRow < 0) press(Key::Down);
        for (int i = 0; i < 30 && focused() != label; ++i) press(Key::Down);
        for (int i = 0; i < 30 && focused() != label; ++i) press(focused().empty() ? Key::Down : Key::Up);
    }
    std::vector<std::string> keysOf(const std::string& action) {
        const auto& tab = stores.controls()->tab();
        return tab.rowKeys(*tab.rowOf(action));
    }
    void toggleOnQolTab() {
        go("Quality of life");
        focus("PC controls");
        press(qol::ui::Key::Enter);
    }
};


std::string firstRowLabel(Menu& m) { return m.menu.screen().rows.front().label; }

std::string firstRowValue(Menu& m) { return m.menu.screen().rows.front().value; }

void originalHasNoSwitch() {
    Fixture f("original");
    Menu m(f.ports);
    m.open();
    m.go("Controls");
    require(firstRowLabel(m) != "Controls for", "PC controls off: the Controls tab has no set switch");
    require(m.menu.helpText().find("controls.pc") == std::string::npos, "the hint names no PC file");
    m.press(Key::Down);
    m.press(Key::Right);
    require(!std::filesystem::exists(f.pcPath) && !std::filesystem::exists(f.directory + "/controls.pc.vehicle.cfg"), "no PC file is written");
    require(slurp(f.controlsPath) == faithfulText, "controls.cfg untouched");
}

void switchOperatesWithEveryDevice() {
    Fixture f("devices");
    Menu m(f.ports);
    m.open();
    m.toggleOnQolTab();
    m.go("Controls");
    require(firstRowLabel(m) == "Controls for" && firstRowValue(m) == "On foot", "PC controls on: the switch is the top row, on foot");
    require(m.menu.helpText().find("controls.pc.cfg") != std::string::npos, "the hint names the on-foot file");
    m.focus("Controls for");
    m.press(Key::Right);
    require(firstRowValue(m) == "Vehicle", "keyboard Right selects the vehicle set");
    require(m.menu.helpText().find("controls.pc.vehicle.cfg") != std::string::npos, "the hint names the vehicle file");
    m.press(Key::PadLeft);
    require(firstRowValue(m) == "On foot", "pad Left goes back");
    m.press(Key::PadRight);
    require(firstRowValue(m) == "Vehicle", "pad Right selects the vehicle set");
    qol::ui::Action click;
    click.kind = qol::ui::ActionKind::ValueChanged;
    click.index = 0;
    click.direction = 1;
    qol::ui::ControlsPage page(*m.stores.controls());
    require(page.handle(click).event == qol::ui::ControlsPageEvent::SetChanged && m.stores.controls()->set() == qol::ControlsSet::OnFoot,
        "a value change on the row (what a click on it sends) selects the other set");
}

void bindA(Menu& m, const std::string& key) {
    m.focus("A");
    m.press(Key::Enter);
    m.input(key);
}

void eachSetKeepsItsOwnFile() {
    Fixture f("files");
    Menu m(f.ports);
    m.open();
    m.toggleOnQolTab();
    m.go("Controls");
    const std::string vehiclePath = f.directory + "/controls.pc.vehicle.cfg";
    require(m.keysOf("A") == std::vector<std::string>{"key E", "key Return"}, "on foot lists the on-foot keys");
    m.focus("Controls for");
    m.press(Key::Right);
    require(m.keysOf("A") == std::vector<std::string>{"key W", "key Up"}, "vehicle lists the vehicle keys");
    require(m.keysOf("B") == std::vector<std::string>{"key S", "key Down", "key Space"}, "B brakes in the vehicle set");
    const std::string onFoot = slurp(f.pcPath);

    bindA(m, "F5");
    require(slurp(vehiclePath).find("A key F5 hold") != std::string::npos, "a rebind in the vehicle set is written to the vehicle file");
    require(slurp(f.pcPath) == onFoot && slurp(f.controlsPath) == faithfulText, "the other files are untouched");

    m.focus("Controls for");
    m.press(Key::Left);
    require(m.keysOf("A") == std::vector<std::string>{"key E", "key Return"}, "back on foot the list is unchanged");
    const std::string vehicle = slurp(vehiclePath);

    m.focus("Reset to defaults");
    m.press(Key::Enter);
    m.press(Key::Left);
    m.press(Key::Enter);
    require(slurp(vehiclePath) == vehicle, "Reset on foot leaves the vehicle file alone");

    m.focus("Controls for");
    m.press(Key::Right);
    require(m.keysOf("A").front() == "key F5", "the vehicle rebinding is still there");
    m.focus("Reset to defaults");
    m.press(Key::Enter);
    m.press(Key::Left);
    m.press(Key::Enter);
    require(m.stores.controls()->tab().bindings() == qol::pcVehicleDefaults(), "Reset in the vehicle set restores the vehicle defaults");
    require(slurp(vehiclePath) != vehicle && slurp(f.pcPath) == onFoot, "only the vehicle file changed");
}

void conflictCheckIsPerSet() {
    Fixture f("conflict");
    Menu m(f.ports);
    m.open();
    m.toggleOnQolTab();
    m.go("Controls");
    m.focus("Controls for");
    m.press(Key::Right);
    bindA(m, "H");
    require(m.menu.modal() == qol::ui::Modal::SwapDialog, "H is Y (horn) in the vehicle set: a conflict asks swap or cancel");
    m.press(Key::Escape);
    m.press(Key::Escape);
    m.focus("Controls for");
    m.press(Key::Left);
    bindA(m, "H");
    require(m.menu.modal() == qol::ui::Modal::None, "H is free on foot: no conflict");
}

void switchIsLockedWhileAsking() {
    Fixture f("locked");
    Menu m(f.ports);
    m.open();
    m.toggleOnQolTab();
    m.go("Controls");
    m.focus("A");
    m.press(Key::Enter);
    require(m.menu.promptOpen(), "the prompt is open");
    m.stores.controls()->selectSet(qol::ControlsSet::Vehicle);
    require(m.stores.controls()->set() == qol::ControlsSet::OnFoot, "the set cannot change while the prompt waits");
}

void setSurvivesToggle() {
    Fixture f("survives");
    Menu m(f.ports);
    m.open();
    m.toggleOnQolTab();
    m.go("Controls");
    m.focus("Controls for");
    m.press(Key::Right);
    m.toggleOnQolTab();
    require(!qol::enabled(qol::Feature::PcControls) && !m.stores.controls()->hasSets(), "PC controls off: no switch");
    m.toggleOnQolTab();
    require(m.stores.controls()->hasSets() && m.stores.controls()->set() == qol::ControlsSet::Vehicle, "switching on again keeps the chosen set");
}

}

int main() {
    originalHasNoSwitch();
    switchOperatesWithEveryDevice();
    eachSetKeepsItsOwnFile();
    conflictCheckIsPerSet();
    switchIsLockedWhileAsking();
    setSurvivesToggle();
    if (failures != 0) {
        std::cerr << failures << " failure(s)\n";
        return 1;
    }
    return 0;
}
