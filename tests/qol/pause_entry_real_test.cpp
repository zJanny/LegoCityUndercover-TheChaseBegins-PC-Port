// The "PC Settings" entry of the pause menu with the real texts and the settings menu behind it: choosing it opens the menu (same
// opening and stores as from the slot page), the pause screen and the game wait while it is open, Escape closes it and the pause screen
// is as it was (qol-settings.md §2026-10-28T09:32:07). Needs the extracted RomFS; exits with 77 without it. Written by
// 20261030-qolpanels-C, 2026-10-30.
#include "qol/pause_entry.hpp"

#include <filesystem>
#include <iostream>
#include <string>

namespace {
namespace fe = chase::frontend;
constexpr float kDt = 1.0f / 30;
int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    ++failures;
    std::cerr << "FAIL: " << what << "\n";
}

void run(fe::PauseMenu& menu, int updates, const fe::PauseInput& input = {}) {
    for (int i = 0; i < updates; ++i) menu.update(kDt, input);
}

SDL_Event key(Uint32 type, SDL_Keycode code) {
    SDL_Event event{};
    event.type = type;
    event.key.keysym.sym = code;
    return event;
}

void pauseWithEntry(fe::Resources& resources) {
    qol::PauseSettings settings;
    fe::PauseMenu menu(qol::withPauseEntry(fe::pauseMenuSetup(resources, "localisation/gametext_uk.loc", "localisation/systemtext_uk.loc"),
        settings, resources, {}));
    menu.open();
    run(menu, 25);
    require(menu.round().items().size() == 5 && menu.round().items()[4].text == "PC Settings" && menu.round().items()[0].text == "Resume",
        "the real pause menu has the four original entries and PC Settings last");
    require(!qol::pauseEntryHolds(settings, menu), "nothing is held before the entry is chosen");

    fe::PauseInput left;
    left.left = true;
    menu.update(kDt, left);
    run(menu, 16);
    fe::PauseInput a;
    a.a = true;
    menu.update(kDt, a);
    require(!settings.menu()->isOpen(), "choosing the entry only records it");
    require(qol::pauseEntryHolds(settings, menu) && settings.menu()->isOpen(), "the next update opens the settings menu and holds the game");
    require(settings.menu()->menu() && settings.menu()->menu()->screen().selectedTab == 0 && settings.menu()->menu()->screen().focusRow == 0,
        "it opens on the first row of its first tab like from the slot page");
    require(qol::pauseEntryHolds(settings, menu), "the pause screen waits while the menu is open");

    require(settings.menu()->consumeOpen(key(SDL_KEYDOWN, SDLK_DOWN), 960, 600) && settings.menu()->menu()->screen().focusRow == 1, "keys go to the menu");
    require(!settings.menu()->consumeOpen(key(SDL_KEYUP, SDLK_DOWN), 960, 600), "key releases pass on");
    require(settings.menu()->consumeOpen(key(SDL_KEYDOWN, SDLK_ESCAPE), 960, 600) && !settings.menu()->isOpen(), "Escape closes the menu");
    require(!qol::pauseEntryHolds(settings, menu), "the pause screen goes on");
    require(menu.isOpen() && !menu.closing() && menu.page() == fe::PausePage::Status && menu.round().selected() == 4,
        "the pause screen is as it was, PC Settings selected");
    require(menu.takeUnported().empty() && menu.takeExtraChosen().empty(), "nothing else was chosen");
}

}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: qol-pause-entry-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "qol-pause-entry-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        chase::assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        fe::Resources resources(archive);
        pauseWithEntry(resources);
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << "\n";
        return 1;
    }
    if (failures) {
        std::cerr << failures << " failure(s)\n";
        return 1;
    }
    std::cout << "qol-pause-entry-real-data: all checks passed\n";
    return 0;
}
