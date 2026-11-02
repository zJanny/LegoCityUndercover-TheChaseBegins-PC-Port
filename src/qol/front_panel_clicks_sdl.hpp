#pragma once

#include "frontend/frontend.hpp"
#include "qol/front_panel_clicks.hpp"

#include <SDL.h>

namespace qol {

class FrontClicks {
public:
    bool consume(const SDL_Event& event, int windowWidth, int windowHeight, const chase::frontend::FrontEnd& front);
    void apply(chase::frontend::FrontInput& pad);

private:
    FrontClickQueue queue_;
};

std::vector<ClickTarget> clickTargets(const chase::frontend::PageTargets& targets);

inline bool frontClicksConsume(FrontClicks& clicks, const SDL_Event& event, SDL_Window* window, const chase::frontend::FrontEnd& front) {
    int width = 0, height = 0;
    SDL_GetWindowSize(window, &width, &height);
    return clicks.consume(event, width, height, front);
}

inline void applyFrontClicks(FrontClicks& clicks, chase::frontend::FrontInput& pad) {
    clicks.apply(pad);
}

}
