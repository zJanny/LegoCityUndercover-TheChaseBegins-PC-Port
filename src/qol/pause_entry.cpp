#include "qol/pause_entry.hpp"

#include <utility>

namespace qol {

const char* const pauseEntryLabel = "PC Settings";
const char* const pauseEntryPicture = "UI_StatusScreen_Options.png";

void PauseSettings::bind(chase::frontend::Resources& resources, StoresPorts ports) {
    ports.quitTab = !ports.makeGame;
    menu_ = std::make_unique<FrontMenu>(resources, std::move(ports));
}

void addPauseEntry(chase::frontend::PauseMenuSetup& setup) {
    setup.extraEntries.push_back({pauseEntryLabel, pauseEntryPicture});
}

chase::frontend::PauseMenuSetup withPauseEntry(chase::frontend::PauseMenuSetup setup, PauseSettings& settings,
    chase::frontend::Resources& resources, StoresPorts ports) {
    settings.bind(resources, std::move(ports));
    addPauseEntry(setup);
    return setup;
}

bool pauseEntryHolds(PauseSettings& settings, chase::frontend::PauseMenu& pause) {
    FrontMenu* menu = settings.menu();
    if (!menu) return false;
    if (menu->isOpen()) return true;
    if (pause.takeExtraChosen().empty()) return false;
    menu->open();
    return true;
}

bool pauseEntryConsumes(PauseSettings& settings, const SDL_Event& event, SDL_Window* window) {
    FrontMenu* menu = settings.menu();
    if (!menu || !menu->isOpen()) return false;
    int width = 0, height = 0;
    SDL_GetWindowSize(window, &width, &height);
    return menu->consumeOpen(event, width, height);
}

bool pauseEntryPresented(PauseSettings& settings, int width, int height) {
    FrontMenu* menu = settings.menu();
    return menu && menu->present(width, height);
}

}
