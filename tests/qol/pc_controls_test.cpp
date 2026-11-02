// "PC controls" toggle and on-foot default set (QOLCTL-8, QOLCTL-2). Spec: docs/design/qol-controls.md entry
// 2026-11-01T13:50:47 (own toggle, on in Improved and off in Original; on-foot layout; default set, rebindable, Reset restores it) and
// the user's answers of 2026-11-01 recorded there (separate file controls.pc.cfg; Return = A second, Keypad Enter = Start second;
// Escape is not in the table, QOLCTL-1). Cases written by 20261101-qolctl-A on 2026-11-01. Driven through the same menu code as
// the settings menu (tests/qol/menu_stores_test.cpp).
#include "input.hpp"
#include "qol/active_settings.hpp"
#include "qol/controls_file.hpp"
#include "qol/menu_stores.hpp"
#include "qol/pc_controls.hpp"
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
        auto folder = std::filesystem::temp_directory_path() / ("chase-qol-pc-controls-" + name);
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
    void focus(const std::string& label) {
        for (int i = 0; i < 30 && menu.screen().rows[static_cast<std::size_t>(menu.screen().focusRow)].label != label; ++i)
            press(qol::ui::Key::Down);
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

using qol::ui::Key;

void originalModeFirst() {
    Fixture f("original");
    Menu m(f.ports);
    m.open();
    for (Key key : {Key::Down, Key::E, Key::Down, Key::E, Key::Up, Key::Q, Key::Q}) m.press(key);
    m.press(Key::Escape);
    require(!m.menu.isOpen(), "closed");
    require(!qol::enabled(qol::Feature::PcControls), "PC controls is off in Original");
    require(!std::filesystem::exists(f.pcPath) && !std::filesystem::exists(f.qolPath), "navigating writes neither controls.pc.cfg nor qol.cfg");
    require(f.controlsChanged == 0, "the running input is not told");
    require(slurp(f.controlsPath) == faithfulText, "controls.cfg untouched");
    const auto active = qol::activeControls(f.controlsPath, chase::runtime::sdlKeyResolver, chase::runtime::sdlKeyNamer);
    require(active.path == f.controlsPath && !active.defaults, "Original reads controls.cfg with its shipped defaults");
    require(chase::runtime::readActiveBindings(f.controlsPath).size() == 4, "the faithful file is the active mapping");
    require(!std::filesystem::exists(f.pcPath), "reading the active mapping in Original creates no file");
}

void masterStates() {
    qol::Settings original;
    require(!original.enabled(qol::Feature::PcControls), "Original: off");
    qol::Settings improved;
    improved.setMaster(qol::Master::Improved);
    require(improved.enabled(qol::Feature::PcControls), "Improved: on");
    qol::Settings custom;
    custom.setSwitch(qol::Feature::PcControls, false);
    custom.setMaster(qol::Master::Custom);
    require(!custom.enabled(qol::Feature::PcControls), "Custom: its own mix (off)");
    custom.setSwitch(qol::Feature::PcControls, true);
    require(custom.enabled(qol::Feature::PcControls), "Custom: its own mix (on)");
    std::istringstream text("master Custom\nwidened_view off\npc_controls on\n");
    require(qol::readSettings(text).enabled(qol::Feature::PcControls), "pc_controls on is read from qol.cfg");
    std::istringstream old("master Custom\nwidened_view on\n");
    require(!qol::readSettings(old).enabled(qol::Feature::PcControls), "a qol.cfg without the line keeps it off");
}

std::vector<std::string> lines(const std::vector<qol::BindingEntry>& entries) {
    std::vector<std::string> out;
    for (const auto& entry : entries) out.push_back(entry.action + " " + entry.source + " " + entry.code + " " + entry.mode);
    return out;
}

void defaultSetMatchesTheSpec() {
    const std::vector<std::string> expected{
        "Up key W hold", "Up key Up hold", "Down key S hold", "Down key Down hold",
        "Left key A hold", "Left key Left hold", "Right key D hold", "Right key Right hold",
        "A key E hold", "A key Return hold", "B key Space hold", "X key F hold",
        "Y mouse Left hold", "Y key R hold",
        "L key Z hold", "L mouse Middle hold", "R key C hold", "R mouse Middle hold",
        "Start key Keypad_Enter hold", "Select key Tab hold",
        "Walk key Left_Ctrl hold", "Walk key Left_Alt hold",
        "DebugOverlay key F3 toggle", "Fullscreen key F11 hold"};
    const auto set = qol::pcControlsDefaults();
    require(lines(set) == expected, "the on-foot set is the approved layout");
    for (const auto& entry : set) require(entry.code != "Escape", "Escape is not bound by this set");
    std::vector<std::string> inputs;
    for (const auto& entry : set) {
        const std::string input = entry.source + " " + entry.code;
        if (input != "mouse Middle") require(std::find(inputs.begin(), inputs.end(), input) == inputs.end(), "no input twice: " + input);
        inputs.push_back(input);
    }
    const std::string text = qol::formatBindingEntries(set, chase::runtime::sdlKeyResolver);
    std::istringstream stream(text);
    require(qol::readBindingEntries(stream, chase::runtime::sdlKeyResolver, chase::runtime::sdlKeyNamer) == set,
        "the set is writable and reads back unchanged");
}

void inputStateOfTheSet() {
    Fixture f("input");
    qol::Settings improved;
    improved.setMaster(qol::Master::Improved);
    qol::setActiveSettings(improved);
    chase::InputState input(chase::runtime::readActiveBindings(f.controlsPath));
    using chase::Action;
    using chase::Source;
    const auto press = [&](Source source, int code) {
        input.reset();
        input.event(source, code, true);
    };
    press(Source::Key, SDLK_SPACE);
    require(input.active(Action::B) && !input.active(Action::A), "Space is B");
    press(Source::Key, SDLK_e);
    require(input.active(Action::A), "E is A");
    press(Source::Key, SDLK_RETURN);
    require(input.active(Action::A) && !input.active(Action::Start), "Return is A, not Start");
    press(Source::Key, SDLK_KP_ENTER);
    require(input.active(Action::Start), "Keypad Enter is Start");
    press(Source::Mouse, SDL_BUTTON_LEFT);
    require(input.active(Action::Y) && !input.active(Action::A), "left mouse is Y");
    press(Source::Key, SDLK_r);
    require(input.active(Action::Y), "R is Y");
    press(Source::Key, SDLK_f);
    require(input.active(Action::X), "F is X");
    press(Source::Key, SDLK_LCTRL);
    require(input.active(Action::Walk), "Left Ctrl is Walk");
    press(Source::Key, SDLK_LALT);
    require(input.active(Action::Walk), "Left Alt is Walk");
    press(Source::Key, SDLK_TAB);
    require(input.active(Action::Select), "Tab is Select");
    press(Source::Key, SDLK_z);
    require(input.active(Action::L) && !input.active(Action::R), "Z turns the camera left");
    press(Source::Key, SDLK_c);
    require(input.active(Action::R) && !input.active(Action::L), "C turns the camera right");
    press(Source::Mouse, SDL_BUTTON_MIDDLE);
    require(input.active(Action::L) && input.active(Action::R), "middle mouse presses L and R together (recentre)");
    press(Source::Mouse, SDL_BUTTON_RIGHT);
    require(!input.active(Action::A) && !input.active(Action::B) && !input.active(Action::Y), "right mouse is unused");
    press(Source::Key, SDLK_w);
    require(input.active(Action::Up), "W is Up");
    press(Source::Key, SDLK_RIGHT);
    require(input.active(Action::Right), "the arrows move too");
    press(Source::Key, SDLK_q);
    require(!input.active(Action::B), "Q is not B in the PC set");
}

void toggleThroughTheMenu() {
    Fixture f("toggle");
    Menu m(f.ports);
    m.open();
    m.go("Controls");
    require(m.keysOf("A") == std::vector<std::string>{"key Space", "mouse Left"}, "the Controls tab starts on the faithful file");

    m.toggleOnQolTab();
    require(qol::enabled(qol::Feature::PcControls) && qol::activeSettings().master() == qol::Master::Custom,
        "the row switches PC controls on (Custom mix)");
    require(slurp(f.qolPath).find("pc_controls on") != std::string::npos, "pc_controls on is in qol.cfg");
    require(std::filesystem::exists(f.pcPath), "controls.pc.cfg is created from the built-in set");
    require(f.controlsChanged == 1, "the running input is told once");
    f.input.event(chase::Source::Key, SDLK_SPACE, true);
    require(f.input.active(chase::Action::B) && !f.input.active(chase::Action::A), "the running input uses the PC set at once");
    f.input.reset();
    require(slurp(f.controlsPath) == faithfulText, "controls.cfg is not touched by the toggle");

    m.go("Controls");
    require(m.keysOf("A") == std::vector<std::string>{"key E", "key Return"}, "the Controls tab shows the PC set");
    require(m.keysOf("B") == std::vector<std::string>{"key Space"}, "B is Space in the Controls tab");

    m.focus("Up");
    m.press(Key::Right);
    m.press(Key::Enter);
    m.input("F5");
    require(slurp(f.pcPath).find("Up key F5 hold") != std::string::npos, "a rebind is written to controls.pc.cfg");
    require(slurp(f.controlsPath) == faithfulText, "a rebind with PC controls on leaves controls.cfg alone");
    std::string rebound = slurp(f.pcPath);

    m.focus("Reset all");
    m.press(Key::Enter);
    m.press(Key::Left);
    m.press(Key::Enter);
    require(m.stores.controls()->tab().bindings() == qol::pcControlsDefaults(), "Reset to defaults restores the PC set");
    require(slurp(f.pcPath) != rebound, "the reset is written to controls.pc.cfg");
    auto* controls = m.stores.controls();
    controls->activate(*controls->tab().rowOf("Up"), 0);
    controls->assignInput("key", "F5");
    require(slurp(f.pcPath).find("Up key F5 hold") != std::string::npos, "a rebind again after the reset");
    rebound = slurp(f.pcPath);

    m.toggleOnQolTab();
    require(!qol::enabled(qol::Feature::PcControls), "the row switches PC controls off again");
    require(f.controlsChanged >= 2, "the running input is told about the switch back");
    f.input.event(chase::Source::Key, SDLK_SPACE, true);
    require(f.input.active(chase::Action::A) && !f.input.active(chase::Action::B), "the faithful mapping is back");
    f.input.reset();
    m.go("Controls");
    require(m.keysOf("A") == std::vector<std::string>{"key Space", "mouse Left"}, "the Controls tab shows the faithful file again");
    require(slurp(f.pcPath) == rebound && slurp(f.controlsPath) == faithfulText, "both files keep their own content");
    m.focus("Reset all");
    m.press(Key::Enter);
    m.press(Key::Left);
    m.press(Key::Enter);
    require(m.stores.controls()->tab().bindings().size() == 4 && slurp(f.controlsPath) == faithfulText,
        "Reset to defaults with PC controls off restores the shipped file");
    require(slurp(f.pcPath) == rebound, "a reset with PC controls off leaves controls.pc.cfg alone");

    m.toggleOnQolTab();
    require(m.keysOf("Up").front() == "key F5", "switching on again keeps the rebinding of controls.pc.cfg");
}

void presetSwitchesTheSet() {
    Fixture f("preset");
    Menu m(f.ports);
    m.open();
    m.go("Quality of life");
    m.press(Key::Right);
    require(qol::activeSettings().master() == qol::Master::Improved && qol::enabled(qol::Feature::PcControls),
        "Improved turns PC controls on");
    require(std::filesystem::exists(f.pcPath) && f.controlsChanged == 1, "and applies the PC set");
    m.press(Key::Right);
    require(qol::activeSettings().master() == qol::Master::Original && !qol::enabled(qol::Feature::PcControls),
        "Original turns it off");
    require(f.controlsChanged == 2, "and applies the faithful file");
}

void startupAndRejections() {
    Fixture f("startup");
    qol::Settings improved;
    improved.setMaster(qol::Master::Improved);
    qol::setActiveSettings(improved);
    const auto bindings = chase::runtime::readActiveBindings(f.controlsPath);
    require(std::filesystem::exists(f.pcPath) && bindings.size() == qol::pcControlsDefaults().size(), "the first start in Improved writes and reads the set");
    spit(f.pcPath, "Up key W hold\nNonsense line\n");
    require(throws([&] { chase::runtime::readActiveBindings(f.controlsPath); }), "a malformed controls.pc.cfg stops with an error");
    require(throws([&] { chase::runtime::makeActiveControls(f.controlsPath); }), "and the Controls tab does not open on it");
    spit(f.pcPath, "");
    require(throws([&] { chase::runtime::readActiveBindings(f.controlsPath); }), "an empty controls.pc.cfg stops with an error");
}

}

int main() {
    originalModeFirst();
    masterStates();
    defaultSetMatchesTheSpec();
    inputStateOfTheSet();
    toggleThroughTheMenu();
    presetSwitchesTheSet();
    startupAndRejections();
    if (failures != 0) {
        std::cerr << failures << " failure(s)\n";
        return 1;
    }
    return 0;
}
