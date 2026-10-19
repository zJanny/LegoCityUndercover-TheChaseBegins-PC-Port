#include "game_screen_fade.hpp"

#include "frontend/pages.hpp"

namespace game_screen_fade {

void draw(const chase::runtime::ScreenFade& fade, chase::frontend::Canvas& top, chase::frontend::Canvas& bottom) {
    chase::frontend::drawBlackVeil(top, fade.opacityOn(chase::runtime::DisplayScreen::Top));
    chase::frontend::drawBlackVeil(bottom, fade.opacityOn(chase::runtime::DisplayScreen::Bottom));
}

}
