// Game tab of the open settings menu (MenuModel with a GameOptions, MenuStores with a GamePort).
// Spec: docs/design/qol-settings-drawing.md entry 2026-10-29T12:11:55 (Q8: Left and Right step the volume rows and flip Invert camera,
// the Language row ignores them and is an activation) and docs/design/qol-settings.md entry 2026-10-28T09:32:07 (Game tab mirrors the
// original options). Values 0..10 from src/runtime/options_page.hpp (kOptionsMaxVolume). Cases written by 20261030-qolpanels-A on 2026-10-30.
// Original mode first: without a GameOptions the Game rows do nothing, as in qol-menu-model.
#include "qol/active_settings.hpp"
#include "qol/game_port.hpp"
#include "qol/menu_model.hpp"
#include "qol/menu_stores.hpp"

#include <filesystem>
#include <iostream>
#include <string>

namespace {

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

struct Fake : qol::GamePort {
    int sfx = 10, music = 8, lang = 1;
    bool invert = false;
    int languagePages = 0, appliedCalls = 0, leftCalls = 0;
    int sfxVolume() const override { return sfx; }
    int musicVolume() const override { return music; }
    bool invertCamera() const override { return invert; }
    int language() const override { return lang; }
    void setSfxVolume(int v) override { sfx = v; }
    void setMusicVolume(int v) override { music = v; }
    void setInvertCamera(bool v) override { invert = v; }
    void openLanguages() override { ++languagePages; }
    void applied() override { ++appliedCalls; }
    void left() override { ++leftCalls; }
};

using qol::Key;
using qol::Result;

Result press(qol::MenuModel& menu, Key key) { return menu.handle(qol::eventForKey(key)); }

void goToRow(qol::MenuModel& menu, std::size_t row) {
    while (menu.row() != row) press(menu, Key::Down);
}

void originalModeFirst() {
    qol::Settings settings;
    qol::MenuModel menu(settings);
    menu.open();
    for (Key key : {Key::Right, Key::Left, Key::Enter}) require(press(menu, key) == Result::Ignored, "Game rows do nothing without a GameOptions");
    goToRow(menu, 2);
    require(press(menu, Key::Right) == Result::Ignored && press(menu, Key::Enter) == Result::Ignored, "Invert camera does nothing without a GameOptions");
}

void gameTabEdits() {
    qol::Settings settings;
    qol::MenuModel menu(settings);
    Fake fake;
    menu.setGame(&fake);
    menu.open();
    require(press(menu, Key::Left) == Result::GameChanged && fake.sfx == 9, "Left lowers the SFX volume");
    require(press(menu, Key::Right) == Result::GameChanged && fake.sfx == 10, "Right raises it");
    require(press(menu, Key::Right) == Result::Ignored && fake.sfx == 10, "stops at 10");
    for (int i = 0; i < 10; ++i) press(menu, Key::Left);
    require(fake.sfx == 0 && press(menu, Key::Left) == Result::Ignored && fake.sfx == 0, "stops at 0");
    require(press(menu, Key::Enter) == Result::Ignored, "Enter does nothing on a volume row");
    press(menu, Key::Down);
    require(press(menu, Key::PadRight) == Result::GameChanged && fake.music == 9, "pad Right steps the music volume");
    press(menu, Key::Down);
    require(press(menu, Key::Right) == Result::GameChanged && fake.invert, "Right flips Invert camera on");
    require(press(menu, Key::Left) == Result::GameChanged && !fake.invert, "Left flips it off");
    require(press(menu, Key::Enter) == Result::GameChanged && fake.invert, "Enter flips it too");
    require(menu.handle(qol::rowClick(2)) == Result::GameChanged && !fake.invert, "a click on the row flips it");
    press(menu, Key::Down);
    require(press(menu, Key::Right) == Result::Ignored && press(menu, Key::Left) == Result::Ignored && fake.lang == 1, "the Language row ignores Left and Right");
    require(press(menu, Key::Enter) == Result::LanguagesRequested && fake.languagePages == 1 && fake.lang == 1, "Enter on Language requests the Languages page");
    require(menu.handle(qol::rowClick(3)) == Result::LanguagesRequested && fake.languagePages == 2, "a click on Language requests it too");
}

void otherTabsKeepTheirKeys() {
    qol::Settings settings;
    qol::MenuModel menu(settings);
    Fake fake;
    menu.setGame(&fake);
    menu.open();
    press(menu, Key::PageDown);
    require(menu.tab() == qol::Tab::Display, "Display tab");
    press(menu, Key::Right);
    require(fake.sfx == 10 && fake.music == 8 && !fake.invert, "Left and Right on another tab leave the Game values");
}

struct Ports {
    Fake* fake = nullptr;
    qol::StoresPorts ports;
    std::string directory;

    Ports() {
        auto folder = std::filesystem::temp_directory_path() / "chase-qol-menu-game";
        std::filesystem::remove_all(folder);
        std::filesystem::create_directories(folder);
        directory = folder.string();
        qol::setActiveSettings(qol::Settings{});
        ports.settingsPath = directory + "/qol.cfg";
        ports.modes = {{800, 600}};
        ports.desktop = {800, 600};
        ports.makeGame = [this]() -> std::unique_ptr<qol::GamePort> {
            auto made = std::make_unique<Fake>();
            fake = made.get();
            return made;
        };
    }
};

void storesApplyAndLeave() {
    Ports f;
    qol::MenuStores stores(f.ports);
    qol::MenuModel menu(stores.settings());
    stores.open(menu);
    menu.open();
    require(f.fake && stores.game() == f.fake, "the stores own the port");
    stores.commit(press(menu, Key::Left));
    require(f.fake->sfx == 9 && f.fake->appliedCalls == 1, "a change reaches the port and is applied");
    stores.commit(press(menu, Key::Down));
    stores.commit(press(menu, Key::Down));
    stores.commit(press(menu, Key::Down));
    stores.commit(press(menu, Key::Enter));
    require(f.fake->languagePages == 1 && f.fake->appliedCalls == 2, "the Language request is applied");
    stores.commit(press(menu, Key::Escape));
    require(f.fake->leftCalls == 1, "closing the menu tells the port");
    stores.open(menu);
    require(stores.game() == f.fake, "the port outlives one opening");
}

void storesWithoutPort() {
    Ports f;
    f.ports.makeGame = nullptr;
    qol::MenuStores stores(f.ports);
    qol::MenuModel menu(stores.settings());
    stores.open(menu);
    menu.open();
    require(stores.game() == nullptr, "no port, no Game tab");
    require(press(menu, Key::Right) == Result::Ignored, "the Game rows stay inert");
    stores.commit(press(menu, Key::Escape));
}

}

int main() {
    originalModeFirst();
    gameTabEdits();
    otherTabsKeepTheirKeys();
    storesApplyAndLeave();
    storesWithoutPort();
    return failures == 0 ? 0 : 1;
}
