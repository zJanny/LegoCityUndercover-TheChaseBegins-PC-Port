#include "qol/pc_mouse_camera_sdl.hpp"

#include "qol/active_settings.hpp"

namespace qol {

MouseCameraSdl::~MouseCameraSdl() { apply(false); }

void MouseCameraSdl::apply(bool captured) {
    if (captured == relative_) return;
    relative_ = captured;
    SDL_SetRelativeMouseMode(captured ? SDL_TRUE : SDL_FALSE);
}

void MouseCameraSdl::event(const SDL_Event& event, SDL_Window* window, const MouseScreen& screen) {
    if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
        camera_.update(false, false, {}, screen);
        apply(false);
        return;
    }
    if (event.type != SDL_MOUSEMOTION || !camera_.captured()) return;
    camera_.motion(event.motion.xrel, event.motion.yrel, screen);
    if (const std::optional<PixelPoint> release = camera_.takeRelease()) {
        apply(false);
        SDL_WarpMouseInWindow(window, release->x, release->y);
    }
}

void MouseCameraSdl::frame(SDL_Window* window, bool menuOpen, const MouseScreen& screen) {
    const bool focused = (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0;
    const bool wanted = enabled(Feature::PcControls) && !menuOpen && focused;
    PixelPoint real;
    const Uint32 buttons = SDL_GetMouseState(&real.x, &real.y);
    camera_.update(wanted, buttons != 0, real, screen);
    apply(camera_.captured());
}

}
