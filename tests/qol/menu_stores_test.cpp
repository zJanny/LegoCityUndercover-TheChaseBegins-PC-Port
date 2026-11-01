// Stores behind the open settings menu (src/qol/menu_stores.*). Spec: docs/design/qol-settings.md entries 2026-10-28T22:20:41
// (window_mode and resolution in config/qol.cfg), 2026-10-29T11:06:51 (the controls file is written right after each change),
// 2026-10-29T18:06:35 (menu opened from the slot page), 2026-10-28T09:53:33 (master states). Cases written by
// 20261030-qolstores-B on 2026-10-30, driven through the new menu controller by 20261101-qolui3-F on 2026-11-01 (design
// qol-settings-drawing.md entry 2026-10-30T21:56:04: keys, display confirm, Swap dialog); display modes, sizes and key names are test fixtures.
#include "input.hpp"
#include "qol/active_settings.hpp"
#include "qol/controls_file.hpp"
#include "qol/menu_stores.hpp"
#include "qol/ui_menu.hpp"
#include "runtime/qol_controls_bindings.hpp"

#include <SDL.h>
#include <filesystem>
#include <fstream>
#include <iostream>
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

const char* const controlsText = "Up key W hold\nA key Space hold\nWalk key Left_Alt hold\n";

std::string slurp(const std::string& path) {
    std::ifstream file(path);
    return std::string(std::istreambuf_iterator<char>(file), {});
}

void spit(const std::string& path, const std::string& text) {
    std::ofstream(path, std::ios::binary) << text;
}

struct Fixture {
    std::string directory, qolPath, controlsPath;
    std::vector<qol::Settings> applied;
    int controlsChanged = 0;
    chase::InputState input{{}};
    qol::StoresPorts ports;

    explicit Fixture(const std::string& name) {
        auto folder = std::filesystem::temp_directory_path() / ("chase-qol-menu-stores-" + name);
        std::filesystem::remove_all(folder);
        std::filesystem::create_directories(folder);
        directory = folder.string();
        qolPath = directory + "/qol.cfg";
        controlsPath = directory + "/controls.cfg";
        spit(controlsPath, controlsText);
        spit(directory + "/controls.default.cfg", controlsText);
        qol::setActiveSettings(qol::Settings{});
        ports.settingsPath = qolPath;
        ports.modes = {{800, 600}, {1280, 720}, {1920, 1080}};
        ports.desktop = {1920, 1080};
        ports.applyWindow = [this](const qol::Settings& settings) { applied.push_back(settings); };
        ports.makeControls = [this]() -> std::unique_ptr<qol::ControlsPort> {
            return std::make_unique<chase::runtime::QolControlsBindings>(controlsPath);
        };
        ports.controlsChanged = [this]() {
            ++controlsChanged;
            std::ifstream file(controlsPath);
            input.rebind(chase::readBindings(file, chase::runtime::sdlKeyResolver));
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
};

using qol::ui::Key;

void originalModeFirst() {
    Fixture f("original");
    Menu m(f.ports);
    m.open();
    for (Key key : {Key::Down, Key::E, Key::Down, Key::E, Key::Up, Key::Q, Key::Q})
        m.press(key);
    m.press(Key::Escape);
    require(!m.menu.isOpen(), "closed");
    require(!std::filesystem::exists(f.qolPath), "navigating the open menu writes no qol.cfg");
    require(qol::activeSettings().master() == qol::Master::Original && !qol::enabled(qol::Feature::WidenedView),
        "the active store stays Original");
    require(f.applied.empty() && f.controlsChanged == 0, "no window change and no controls change");
    require(slurp(f.controlsPath) == controlsText, "controls.cfg untouched");
}

void masterReachesFileAndActiveStore() {
    Fixture f("master");
    Menu m(f.ports);
    m.open();
    m.go("Quality of life");
    require(m.press(Key::Right).settingsChanged, "the preset selector steps");
    require(qol::activeSettings().master() == qol::Master::Improved && qol::enabled(qol::Feature::WidenedView),
        "Improved reaches the active store at once");
    require(qol::loadSettings(f.qolPath).master() == qol::Master::Improved, "Improved is in qol.cfg");
    require(f.applied.empty(), "a master change does not touch the window");

    m.press(Key::Down);
    require(m.press(Key::Enter).settingsChanged, "a feature row switches to Custom");
    require(qol::activeSettings().master() == qol::Master::Custom && !qol::enabled(qol::Feature::WidenedView),
        "the toggled feature is off in Custom");
    const qol::Settings saved = qol::loadSettings(f.qolPath);
    require(saved.master() == qol::Master::Custom && saved.mixSet() && !saved.customSwitch(qol::Feature::WidenedView),
        "Custom and its mix are in qol.cfg");

    Menu again(f.ports);
    again.open();
    require(again.stores.settings().master() == qol::Master::Custom, "a reopened menu reads the active store");
}

void displayReachesWindow() {
    Fixture f("display");
    Menu m(f.ports);
    m.open();
    m.go("Display");
    require(m.press(Key::Right).windowChanged, "window mode row");
    require(f.applied.size() == 1 && f.applied[0].windowMode() == qol::WindowMode::Windowed, "the window gets the new mode");
    require(qol::activeSettings().windowMode() == qol::WindowMode::Windowed, "active store has the window mode");
    require(slurp(f.qolPath).find("window_mode windowed") != std::string::npos, "window_mode in qol.cfg");
    m.press(Key::Enter);

    m.press(Key::Down);
    m.press(Key::Enter);
    m.press(Key::Up);
    m.press(Key::Up);
    require(m.press(Key::Enter).windowChanged, "a size from the resolution list");
    require(f.applied.size() == 2 && f.applied[1].resolution() && *f.applied[1].resolution() == qol::Resolution{800, 600},
        "the window gets the chosen size");
    require(slurp(f.qolPath).find("resolution 800x600") != std::string::npos, "resolution in qol.cfg");
    require(m.press(Key::Escape).windowChanged && f.applied.size() == 3 && !f.applied[2].resolution(),
        "Esc in Keep these settings? reverts the window");
    require(slurp(f.qolPath).find("resolution") == std::string::npos, "the revert is in qol.cfg");
    require(qol::activeSettings().master() == qol::Master::Original, "display leaves the master alone");
}

void controlsTabUsesBindingsAndFile() {
    Fixture f("controls");
    Menu m(f.ports);
    m.open();
    m.go("Controls");
    require(m.menu.screen().rows.size() > 1 && m.menu.screen().rows[0].label == "Up", "the Controls tab has the bindings rows");
    m.press(Key::Right);
    m.press(Key::Enter);
    require(m.menu.promptOpen(), "waiting for an input");
    require(m.input("F5").controlsChanged, "bound");
    require(slurp(f.controlsPath) == "Up key W hold\nA key Space hold\nWalk key Left_Alt hold\nUp key F5 hold\n",
        "controls.cfg written right after the change");
    require(f.controlsChanged == 1, "the running input is told once");
    f.input.event(chase::Source::Key, SDLK_F5, true);
    require(f.input.active(chase::Action::Up), "the running input uses the new binding");

    for (int i = 0; i < 20 && m.menu.screen().rows[static_cast<std::size_t>(m.menu.screen().focusRow)].label != "A"; ++i) m.press(Key::Down);
    m.press(Key::Left);
    m.press(Key::Enter);
    m.input("F5");
    require(m.menu.modal() == qol::ui::Modal::SwapDialog, "a key of another action asks to swap");
    m.press(Key::Left);
    require(m.press(Key::Enter).controlsChanged && f.controlsChanged == 2, "a confirmed swap tells the running input");
    require(slurp(f.controlsPath) == "Up key W hold\nA key F5 hold\nWalk key Left_Alt hold\nUp key Space hold\n",
        "the swap is written to controls.cfg (QOLSET-93)");
    require(!std::filesystem::exists(f.qolPath), "a controls change writes no qol.cfg");
}

void rejections() {
    for (const char* name : {"bad-modes", "bad-desktop", "missing-controls", "bad-controls"}) {
        Fixture f(name);
        if (std::string(name) == "bad-modes") f.ports.modes = {{399, 240}};
        if (std::string(name) == "bad-desktop") f.ports.desktop = {300, 200};
        if (std::string(name) == "missing-controls") std::filesystem::remove(f.controlsPath);
        if (std::string(name) == "bad-controls") spit(f.controlsPath, "Up key W hold\nNonsense line\n");
        qol::MenuStores stores(f.ports);
        require(throws([&] { stores.open(); }), std::string("the open stops: ") + name);
    }
    {
        Fixture f("unwritable");
        f.ports.settingsPath = f.directory + "/missing-folder/qol.cfg";
        Menu m(f.ports);
        m.open();
        m.go("Quality of life");
        const qol::MenuEffects effects = m.menu.handle(qol::ui::keyEvent(Key::Right), 0);
        require(throws([&] { m.stores.commit(effects); }), "a write error stops with an error");
        require(qol::activeSettings().master() == qol::Master::Original, "a failed write leaves the active store alone");
    }
    {
        Fixture f("malformed-qol");
        spit(f.qolPath, "master sideways\n");
        require(throws([&] { qol::loadSettings(f.qolPath); }), "a malformed qol.cfg is refused");
    }
    {
        Fixture f("unconfigured");
        Menu m(qol::StoresPorts{});
        m.open();
        require(m.menu.tabTitles() == std::vector<std::string>{"Quality of life"}, "without ports only the Quality of life tab");
        m.press(Key::Right);
        require(qol::activeSettings().master() == qol::Master::Original && !std::filesystem::exists(f.qolPath),
            "without ports the menu works on its copy only");
    }
}

}

int main() {
    originalModeFirst();
    masterReachesFileAndActiveStore();
    displayReachesWindow();
    controlsTabUsesBindingsAndFile();
    rejections();
    if (failures) {
        std::cerr << failures << " failure(s)\n";
        return 1;
    }
    std::cout << "menu stores: ok\n";
    return 0;
}
