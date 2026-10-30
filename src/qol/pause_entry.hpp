#pragma once

#include "frontend/pause_menu.hpp"
#include "frontend/resources.hpp"
#include "qol/menu_stores.hpp"
#include "qol/menu_window_sdl.hpp"

#include <SDL.h>
#include <memory>

namespace qol {

extern const char* const pauseEntryLabel;
extern const char* const pauseEntryPicture;

class PauseSettings {
public:
    void bind(chase::frontend::Resources& resources, StoresPorts ports);
    FrontMenu* menu() { return menu_.get(); }

private:
    std::unique_ptr<FrontMenu> menu_;
};

void addPauseEntry(chase::frontend::PauseMenuSetup& setup);
chase::frontend::PauseMenuSetup withPauseEntry(chase::frontend::PauseMenuSetup setup, PauseSettings& settings,
    chase::frontend::Resources& resources, StoresPorts ports);
bool pauseEntryHolds(PauseSettings& settings, chase::frontend::PauseMenu& pause);
bool pauseEntryConsumes(PauseSettings& settings, const SDL_Event& event, SDL_Window* window);
bool pauseEntryPresented(PauseSettings& settings, int width, int height);

}
