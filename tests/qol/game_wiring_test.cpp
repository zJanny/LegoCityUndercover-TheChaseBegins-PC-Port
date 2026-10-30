// Port side of the Game tab in the open settings menu (src/runtime/qol_game_wiring.*) over the real Options page, options block and save.
// Spec: docs/design/qol-settings.md entry 2026-10-28T09:32:07 (same values, same save, in sync with the original Options page) and
// docs/design/qol-settings-drawing.md entry 2026-10-29T12:11:55 (Left and Right step the volumes, Invert camera flips, Language activates).
// Options block: recorded from a save state by 20261028-uiport-servant on 2026-10-28, paused on the Options page after mission 1-3,
// slot 1 in use, sound effects 5, music 5, Invert camera on, language 1 (as pinned in tests/qol/game_options_test.cpp). The block
// 8a 01 01 23 0a 05 08 05 holds the volumes of slot 1 at +5 (sound 5) and +7 (music 5). Cases written by 20261030-qolpanels-A, 2026-10-30.
#include "qol/active_settings.hpp"
#include "qol/menu_model.hpp"
#include "qol/menu_stores.hpp"
#include "runtime/qol_game_wiring.hpp"
#include "save/save.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace {

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

constexpr std::uint8_t recorded[8] = {0x8a, 0x01, 0x01, 0x23, 0x0a, 0x05, 0x08, 0x05};

using qol::Key;
using qol::Result;

struct Gains {
    std::vector<std::pair<std::optional<float>, std::optional<float>>> calls;
};

struct Fixture {
    std::string directory, loadFile, saveFile;
    Gains gains;
    qol::StoresPorts ports;

    explicit Fixture(const std::string& name) {
        auto folder = std::filesystem::temp_directory_path() / ("chase-qol-game-wiring-" + name);
        std::filesystem::remove_all(folder);
        std::filesystem::create_directories(folder);
        directory = folder.string();
        loadFile = directory + "/load.dat";
        saveFile = directory + "/save.dat";
        chase::save::SaveImage image;
        image.resetOptions(1);
        std::copy(recorded, recorded + 8, image.options());
        chase::save::store(loadFile, image);
        qol::setActiveSettings(qol::Settings{});
        ports.settingsPath = directory + "/qol.cfg";
        ports.modes = {{800, 600}};
        ports.desktop = {800, 600};
        chase::runtime::QolGameSetup setup;
        setup.loadFile = loadFile;
        setup.saveFile = saveFile;
        setup.setGains = [this](std::optional<float> sound, std::optional<float> music) { gains.calls.push_back({sound, music}); };
        ports.makeGame = chase::runtime::qolGamePortFactory(setup);
    }
};

Result step(qol::MenuStores& stores, qol::MenuModel& menu, Key key) {
    const Result result = menu.handle(qol::eventForKey(key));
    stores.commit(result);
    return result;
}

void readsTheRecordedBlock() {
    Fixture f("reads");
    qol::MenuStores stores(f.ports);
    qol::MenuModel menu(stores.settings());
    stores.open(menu);
    const qol::GameOptions* game = stores.game();
    require(game && game->sfxVolume() == 5 && game->musicVolume() == 5 && game->invertCamera() && game->language() == 1,
        "the Game tab shows the recorded options");
}

void editsReachMixerAndSave() {
    Fixture f("edits");
    qol::MenuStores stores(f.ports);
    qol::MenuModel menu(stores.settings());
    stores.open(menu);
    menu.open();
    require(step(stores, menu, Key::Right) == Result::GameChanged && stores.game()->sfxVolume() == 6, "SFX volume 6");
    require(!f.gains.calls.empty() && f.gains.calls.back().first && *f.gains.calls.back().first == 0.6f, "the sound effects gain is 0.6");
    step(stores, menu, Key::Down);
    for (int i = 0; i < 5; ++i) step(stores, menu, Key::Left);
    require(stores.game()->musicVolume() == 0, "music volume 0");
    require(f.gains.calls.back().second && *f.gains.calls.back().second == 0.0f, "the music gain is 0");
    step(stores, menu, Key::Down);
    step(stores, menu, Key::Right);
    require(!stores.game()->invertCamera(), "Invert camera flips off");
    require(!std::filesystem::exists(f.saveFile), "nothing is saved while the menu is open");
    step(stores, menu, Key::Escape);
    const chase::save::LoadResult saved = chase::save::load(f.saveFile);
    require(saved.found && saved.status == chase::save::Status::Ok && saved.image, "the options are saved on leaving");
    if (saved.image) {
        require(saved.image->soundVolume(1) == 6 && saved.image->musicVolume(1) == 0 && !saved.image->invertCamera() && saved.image->language() == 1,
            "the saved block holds the edits of slot 1");
    }
}

void leavingWithoutChangeSavesNothing() {
    Fixture f("nochange");
    qol::MenuStores stores(f.ports);
    qol::MenuModel menu(stores.settings());
    stores.open(menu);
    menu.open();
    step(stores, menu, Key::Down);
    step(stores, menu, Key::Escape);
    require(!std::filesystem::exists(f.saveFile) && f.gains.calls.empty(), "no change, no save and no gain");
}

void languageRowRequestsThePage() {
    Fixture f("language");
    qol::MenuStores stores(f.ports);
    qol::MenuModel menu(stores.settings());
    stores.open(menu);
    menu.open();
    for (int i = 0; i < 3; ++i) step(stores, menu, Key::Down);
    require(step(stores, menu, Key::Enter) == Result::LanguagesRequested, "Enter on the Language row requests the Languages page");
    require(stores.game()->language() == 1 && f.gains.calls.empty(), "the language and the gains stay");
}

void freshSaveUsesTheDefaults() {
    Fixture f("fresh");
    std::filesystem::remove(f.loadFile);
    chase::runtime::QolGameSetup setup;
    setup.saveFile = f.saveFile;
    setup.language = 4;
    setup.soundVolume = 7;
    setup.musicVolume = 3;
    auto port = chase::runtime::qolGamePortFactory(setup)();
    require(port->sfxVolume() == 7 && port->musicVolume() == 3 && !port->invertCamera() && port->language() == 4,
        "no save: the reset defaults of the save image with the start volumes and language");
}

void rejections() {
    Fixture f("reject");
    chase::runtime::QolGameSetup setup;
    setup.loadFile = f.directory + "/missing.dat";
    bool threw = false;
    try {
        chase::runtime::qolGamePortFactory(setup)();
    } catch (const std::exception& error) {
        threw = std::string(error.what()).find("save file not found") != std::string::npos;
    }
    require(threw, "a missing --front-save file stops with its name");
    Fixture g("reject-bad");
    std::filesystem::copy_file(g.loadFile, g.directory + "/bad.dat");
    std::filesystem::resize_file(g.directory + "/bad.dat", 10);
    setup.loadFile = g.directory + "/bad.dat";
    threw = false;
    try {
        chase::runtime::qolGamePortFactory(setup)();
    } catch (const std::exception& error) {
        threw = std::string(error.what()).find("not a valid save file") != std::string::npos;
    }
    require(threw, "an invalid --front-save file stops with its name");
}

}

int main() {
    readsTheRecordedBlock();
    editsReachMixerAndSave();
    leavingWithoutChangeSavesNothing();
    languageRowRequestsThePage();
    freshSaveUsesTheDefaults();
    rejections();
    return failures == 0 ? 0 : 1;
}
