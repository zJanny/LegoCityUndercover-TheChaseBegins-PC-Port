// Port side of the Game tab in the open settings menu (src/runtime/qol_game_wiring.*) over the real Options page, options block and save.
// Spec: docs/design/qol-settings.md entry 2026-10-28T09:32:07 (same values, same save, in sync with the original Options page) and
// docs/design/qol-settings-drawing.md entry 2026-10-30T21:56:04 (Left and Right step the volumes, Enter flips Invert camera, answer 4:
// the Language applies on close; driven through the menu controller by 20261101-qolui3-F on 2026-11-01).
// Options block: recorded from a save state by 20261028-uiport-servant on 2026-10-28, paused on the Options page after mission 1-3,
// slot 1 in use, sound effects 5, music 5, Invert camera on, language 1 (as pinned in tests/qol/game_options_test.cpp). The block
// Language applies by the port's LanguagesPage (a, then update with the list closed) written by 20261112-qolpc-B on 2026-11-12; the mixer
// starts at the slot volumes of the same block (QOLSET-61). 8a 01 01 23 0a 05 08 05 holds the volumes of slot 1 at +5 (sound 5) and +7 (music 5). Cases written by 20261030-qolpanels-A, 2026-10-30.
#include "qol/active_settings.hpp"
#include "qol/menu_stores.hpp"
#include "qol/ui_menu.hpp"
#include "runtime/qol_game_wiring.hpp"
#include "save/save.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
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

using qol::ui::Key;

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

float measure(std::string_view text, float pixels) { return static_cast<float>(text.size()) * pixels * 0.5f; }
float lineHeight(float pixels) { return pixels * 1.2f; }

const std::vector<std::string> names{"English", "French", "Italian", "German", "Spanish", "Dutch", "Danish", "Portuguese", "Russian"};

struct Menu {
    qol::MenuStores stores;
    qol::ui::UiMenu menu{measure, lineHeight};

    explicit Menu(const qol::StoresPorts& ports) : stores(ports) {}
    void open() {
        stores.open();
        menu.setWindow(1920, 1080);
        menu.open(qol::ui::menuSetupFor(stores, names));
    }
    qol::MenuEffects step(Key key) {
        const qol::MenuEffects effects = menu.handle(qol::ui::keyEvent(key), 0);
        stores.commit(effects);
        return effects;
    }
};

void readsTheRecordedBlock() {
    Fixture f("reads");
    qol::MenuStores stores(f.ports);
    stores.open();
    const qol::GameOptions* game = stores.game();
    require(game && game->sfxVolume() == 5 && game->musicVolume() == 5 && game->invertCamera() && game->language() == 1,
        "the Game tab shows the recorded options");
}

void editsReachMixerAndSave() {
    Fixture f("edits");
    Menu m(f.ports);
    m.open();
    require(m.step(Key::Right).gameChanged && m.stores.game()->sfxVolume() == 6, "SFX volume 6");
    require(!f.gains.calls.empty() && f.gains.calls.back().first && *f.gains.calls.back().first == 0.6f, "the sound effects gain is 0.6");
    m.step(Key::Down);
    for (int i = 0; i < 5; ++i) m.step(Key::Left);
    require(m.stores.game()->musicVolume() == 0, "music volume 0");
    require(f.gains.calls.back().second && *f.gains.calls.back().second == 0.0f, "the music gain is 0");
    m.step(Key::Down);
    m.step(Key::Enter);
    require(!m.stores.game()->invertCamera(), "Invert camera flips off");
    require(!std::filesystem::exists(f.saveFile), "nothing is saved while the menu is open");
    m.step(Key::Escape);
    const chase::save::LoadResult saved = chase::save::load(f.saveFile);
    require(saved.found && saved.status == chase::save::Status::Ok && saved.image, "the options are saved on leaving");
    if (saved.image) {
        require(saved.image->soundVolume(1) == 6 && saved.image->musicVolume(1) == 0 && !saved.image->invertCamera() && saved.image->language() == 1,
            "the saved block holds the edits of slot 1");
    }
}

void leavingWithoutChangeSavesNothing() {
    Fixture f("nochange");
    Menu m(f.ports);
    m.open();
    f.gains.calls.clear();
    m.step(Key::Down);
    m.step(Key::Escape);
    require(!std::filesystem::exists(f.saveFile) && f.gains.calls.empty(), "no change, no save and no gain");
}

void mixerStartsAtTheSlotVolumes() {
    Fixture f("start");
    Menu m(f.ports);
    m.open();
    require(f.gains.calls.size() == 1 && f.gains.calls[0].first && *f.gains.calls[0].first == 0.5f && f.gains.calls[0].second && *f.gains.calls[0].second == 0.5f,
        "opening the menu sets the mixer to the volumes of slot 1 (sound 5, music 5), as the Game tab shows them (QOLSET-61)");
    Fixture g("start-same");
    chase::runtime::QolGameSetup setup;
    setup.loadFile = g.loadFile;
    setup.saveFile = g.saveFile;
    setup.soundVolume = 5;
    setup.musicVolume = 5;
    int calls = 0;
    setup.setGains = [&calls](std::optional<float>, std::optional<float>) { ++calls; };
    auto port = chase::runtime::qolGamePortFactory(setup)();
    require(calls == 0, "volumes already at the slot's change nothing");
    setup.loadFile.clear();
    setup.soundVolume = 7;
    setup.musicVolume = 3;
    port = chase::runtime::qolGamePortFactory(setup)();
    require(calls == 0, "no slot in use: the mixer stays at the start volumes");
}

void languageOnCloseAppliesThroughTheLanguagesPage() {
    Fixture f("language");
    Menu m(f.ports);
    m.open();
    for (int i = 0; i < 3; ++i) m.step(Key::Down);
    m.step(Key::Right);
    require(m.stores.game()->language() == 1 && !std::filesystem::exists(f.saveFile), "the language stays until the menu closes (answer 4)");
    const qol::MenuEffects closed = m.step(Key::Escape);
    require(closed.language == 2, "the chosen language is handed over on close (answer 4)");
    require(m.stores.game()->language() == 2, "the Languages page puts French into the options");
    const chase::save::LoadResult saved = chase::save::load(f.saveFile);
    require(saved.found && saved.status == chase::save::Status::Ok && saved.image && saved.image->language() == 2 && saved.image->soundVolume(1) == 5,
        "the language is saved and the other options are kept");
    chase::runtime::QolGameSetup reopened;
    reopened.loadFile = f.saveFile;
    reopened.saveFile = f.saveFile;
    require(chase::runtime::qolGamePortFactory(reopened)()->language() == 2, "the menu opened on the saved file shows the new language");
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
    require(port->sfxVolume() == 7 && port->musicVolume() == 3 && !port->invertCamera() && port->language() == 4 && port->startLanguage() == 4,
        "no save: the reset defaults of the save image with the start volumes and language");
}

void rejections() {
    Fixture f("reject");
    chase::runtime::QolGameSetup wrongLanguage;
    wrongLanguage.loadFile = f.loadFile;
    wrongLanguage.saveFile = f.saveFile;
    bool wrong = false;
    try {
        chase::runtime::qolGamePortFactory(wrongLanguage)()->chooseLanguage(10);
    } catch (const std::out_of_range&) {
        wrong = true;
    }
    require(wrong, "a language outside 1 to 9 is rejected");
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
    mixerStartsAtTheSlotVolumes();
    languageOnCloseAppliesThroughTheLanguagesPage();
    freshSaveUsesTheDefaults();
    rejections();
    return failures == 0 ? 0 : 1;
}
