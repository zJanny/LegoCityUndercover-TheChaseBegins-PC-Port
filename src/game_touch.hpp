#pragma once
// chase-game glue: the left mouse button over the bottom screen is the 3DS touch screen (docs/controls.md). A left press that starts
// inside the bottom screen is a touch and does not reach the button bindings; it stays a touch until the button is released, and the
// position follows the mouse (clamped to the screen) while it is held.
#include "frontend/canvas.hpp"
#include "game_hud.hpp"
#include "runtime/pad.hpp"
#include <SDL.h>
#include <algorithm>

namespace game_touch {

class Tracker {
public:
    // True when the event belongs to the touch screen and must not be handed to the bindings.
    bool consume(const SDL_Event& event, SDL_Window* window) {
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) down_ = false;
        if (event.type == SDL_MOUSEMOTION && down_) {
            place(window, event.motion.x, event.motion.y, true);
            return false;
        }
        if ((event.type != SDL_MOUSEBUTTONDOWN && event.type != SDL_MOUSEBUTTONUP) || event.button.button != SDL_BUTTON_LEFT) return false;
        if (event.type == SDL_MOUSEBUTTONUP) {
            const bool wasDown = down_;
            down_ = false;
            return wasDown;
        }
        down_ = place(window, event.button.x, event.button.y, false);
        return down_;
    }
    void apply(chase::runtime::PadState& state) const {
        state.touch = down_;
        state.touchX = std::int16_t(x_);
        state.touchY = std::int16_t(y_);
    }

private:
    bool place(SDL_Window* window, int mouseX, int mouseY, bool clamp) {
        int width = 0, height = 0;
        SDL_GetWindowSize(window, &width, &height);
        const game_hud::Rect bottom = game_hud::screensFor(width, height).bottom;
        if (bottom.width <= 0 || bottom.height <= 0) return false;
        const int topEdge = height - (bottom.y + bottom.height);
        int x = (mouseX - bottom.x) * chase::frontend::kBottomWidth / bottom.width;
        int y = (mouseY - topEdge) * chase::frontend::kScreenHeight / bottom.height;
        const bool inside = x >= 0 && x < chase::frontend::kBottomWidth && y >= 0 && y < chase::frontend::kScreenHeight;
        if (!inside && !clamp) return false;
        x_ = std::clamp(x, 0, chase::frontend::kBottomWidth - 1);
        y_ = std::clamp(y, 0, chase::frontend::kScreenHeight - 1);
        return true;
    }

    bool down_ = false;
    int x_ = 0, y_ = 0;
};

}
