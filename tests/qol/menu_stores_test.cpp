// Stores behind the open settings menu (src/qol/menu_stores.*). Spec: docs/design/qol-settings.md entries 2026-10-28T22:20:41
// (window_mode and resolution in config/qol.cfg), 2026-10-29T11:06:51 (the controls file is written right after each change),
// 2026-10-29T18:06:35 (menu opened from the slot page), 2026-10-28T09:53:33 (master states). Cases written by
// 20261030-qolstores-B on 2026-10-30; display modes, sizes and key names are test fixtures.
#include "input.hpp"
#include "qol/active_settings.hpp"
#include "qol/controls_file.hpp"
#include "qol/menu_stores.hpp"
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

void go(qol::MenuModel& menu, qol::Tab tab) {
    menu.handle(qol::tabClick(static_cast<std::size_t>(tab)));
}

qol::Result commit(qol::MenuStores& stores, qol::Result result) {
    stores.commit(result);
    return result;
}

void originalModeFirst() {
    Fixture f("original");
    qol::MenuStores stores(f.ports);
    qol::MenuModel menu(stores.settings());
    stores.open(menu);
    menu.open();
    for (qol::Key key : {qol::Key::Down, qol::Key::Right, qol::Key::PageDown, qol::Key::Down, qol::Key::PageUp, qol::Key::Up})
        commit(stores, menu.handle(qol::eventForKey(key)));
    commit(stores, menu.handle(qol::eventForKey(qol::Key::Escape)));
    require(!menu.isOpen(), "closed");
    require(!std::filesystem::exists(f.qolPath), "navigating the open menu writes no qol.cfg");
    require(qol::activeSettings().master() == qol::Master::Original && !qol::enabled(qol::Feature::WidenedView),
        "the active store stays Original");
    require(f.applied.empty() && f.controlsChanged == 0, "no window change and no controls change");
    require(slurp(f.controlsPath) == controlsText, "controls.cfg untouched");
}

void masterReachesFileAndActiveStore() {
    Fixture f("master");
    qol::MenuStores stores(f.ports);
    qol::MenuModel menu(stores.settings());
    stores.open(menu);
    menu.open();
    go(menu, qol::Tab::QualityOfLife);
    require(commit(stores, menu.handle(qol::eventForKey(qol::Key::Enter))) == qol::Result::MasterCycled, "master row cycles");
    require(qol::activeSettings().master() == qol::Master::Improved && qol::enabled(qol::Feature::WidenedView),
        "Improved reaches the active store at once");
    require(qol::loadSettings(f.qolPath).master() == qol::Master::Improved, "Improved is in qol.cfg");
    require(f.applied.empty(), "a master change does not touch the window");

    require(commit(stores, menu.handle(qol::rowClick(1))) == qol::Result::SwitchedToCustom, "feature row switches to Custom");
    require(qol::activeSettings().master() == qol::Master::Custom && !qol::enabled(qol::Feature::WidenedView),
        "the toggled feature is off in Custom");
    const qol::Settings saved = qol::loadSettings(f.qolPath);
    require(saved.master() == qol::Master::Custom && saved.mixSet() && !saved.customSwitch(qol::Feature::WidenedView),
        "Custom and its mix are in qol.cfg");

    qol::MenuStores again(f.ports);
    qol::MenuModel reopened(again.settings());
    again.open(reopened);
    require(again.settings().master() == qol::Master::Custom, "a reopened menu reads the active store");
}

void displayReachesWindow() {
    Fixture f("display");
    qol::MenuStores stores(f.ports);
    qol::MenuModel menu(stores.settings());
    stores.open(menu);
    menu.open();
    go(menu, qol::Tab::Display);
    require(commit(stores, menu.handle(qol::eventForKey(qol::Key::Enter))) == qol::Result::DisplayChanged, "window mode row");
    require(f.applied.size() == 1 && f.applied[0].windowMode() == qol::WindowMode::Windowed, "the window gets the new mode");
    require(qol::activeSettings().windowMode() == qol::WindowMode::Windowed, "active store has the window mode");
    require(slurp(f.qolPath).find("window_mode windowed") != std::string::npos, "window_mode in qol.cfg");

    require(commit(stores, menu.handle(qol::rowClick(1))) == qol::Result::DisplayChanged, "resolution row");
    require(f.applied.size() == 2 && f.applied[1].resolution() && *f.applied[1].resolution() == qol::Resolution{800, 600},
        "the window gets the step after the desktop size (wrapping to the smallest mode)");
    require(slurp(f.qolPath).find("resolution 800x600") != std::string::npos, "resolution in qol.cfg");
    require(qol::activeSettings().master() == qol::Master::Original, "display leaves the master alone");
}

void controlsTabUsesBindingsAndFile() {
    Fixture f("controls");
    qol::MenuStores stores(f.ports);
    qol::MenuModel menu(stores.settings());
    stores.open(menu);
    menu.open();
    go(menu, qol::Tab::Controls);
    require(!menu.controlsRows().empty(), "the Controls tab has the bindings rows");
    std::size_t up = 0;
    for (std::size_t i = 0; i < menu.controlsRows().size(); ++i)
        if (menu.controlsRows()[i].action == "Up") up = i;
    require(commit(stores, menu.handle(qol::slotClick(up, 1))) == qol::Result::ControlsWaiting, "waiting for an input");
    require(commit(stores, menu.assignInput("key", "F5")) == qol::Result::ControlsChanged, "bound");
    require(slurp(f.controlsPath) == "Up key W hold\nA key Space hold\nWalk key Left_Alt hold\nUp key F5 hold\n",
        "controls.cfg written right after the change");
    require(f.controlsChanged == 1, "the running input is told once");
    f.input.event(chase::Source::Key, SDLK_F5, true);
    require(f.input.active(chase::Action::Up), "the running input uses the new binding");
    require(!std::filesystem::exists(f.qolPath), "a controls change writes no qol.cfg");
}

void rejections() {
    {
        Fixture f("bad-modes");
        f.ports.modes = {{399, 240}};
        qol::MenuStores stores(f.ports);
        qol::MenuModel menu(stores.settings());
        require(throws([&] { stores.open(menu); }), "display modes below the minimum stop the open");
    }
    {
        Fixture f("bad-desktop");
        f.ports.desktop = {300, 200};
        qol::MenuStores stores(f.ports);
        qol::MenuModel menu(stores.settings());
        require(throws([&] { stores.open(menu); }), "a tiny desktop stops the open");
    }
    {
        Fixture f("missing-controls");
        std::filesystem::remove(f.controlsPath);
        qol::MenuStores stores(f.ports);
        qol::MenuModel menu(stores.settings());
        require(throws([&] { stores.open(menu); }), "a missing controls file stops the open");
    }
    {
        Fixture f("bad-controls");
        spit(f.controlsPath, "Up key W hold\nNonsense line\n");
        qol::MenuStores stores(f.ports);
        qol::MenuModel menu(stores.settings());
        require(throws([&] { stores.open(menu); }), "a malformed controls file stops the open");
    }
    {
        Fixture f("unwritable");
        f.ports.settingsPath = f.directory + "/missing-folder/qol.cfg";
        qol::MenuStores stores(f.ports);
        qol::MenuModel menu(stores.settings());
        stores.open(menu);
        menu.open();
        go(menu, qol::Tab::QualityOfLife);
        const qol::Result result = menu.handle(qol::eventForKey(qol::Key::Enter));
        require(throws([&] { stores.commit(result); }), "a write error stops with an error");
        require(qol::activeSettings().master() == qol::Master::Original, "a failed write leaves the active store alone");
    }
    {
        Fixture f("malformed-qol");
        spit(f.qolPath, "master sideways\n");
        require(throws([&] { qol::loadSettings(f.qolPath); }), "a malformed qol.cfg is refused");
    }
    {
        Fixture f("unconfigured");
        qol::MenuStores stores(qol::StoresPorts{});
        qol::MenuModel menu(stores.settings());
        stores.open(menu);
        menu.open();
        go(menu, qol::Tab::QualityOfLife);
        commit(stores, menu.handle(qol::eventForKey(qol::Key::Enter)));
        require(qol::activeSettings().master() == qol::Master::Original && !std::filesystem::exists(f.qolPath),
            "without ports the menu works on its copy only");
    }
    require(!qol::changesSettings(qol::Result::TabChanged) && !qol::changesSettings(qol::Result::ControlsChanged) &&
            !qol::changesSettings(qol::Result::Ignored),
        "navigation results change no setting");
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
