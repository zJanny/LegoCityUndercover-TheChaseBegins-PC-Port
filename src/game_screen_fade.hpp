#pragma once
// Draws the screen fade (runtime/screen_fade.hpp) over the two screens of chase-game.
#include "frontend/canvas.hpp"
#include "runtime/screen_fade.hpp"

namespace game_screen_fade {

void draw(const chase::runtime::ScreenFade& fade, chase::frontend::Canvas& top, chase::frontend::Canvas& bottom);

}
