#pragma once

#include "qol/pc_mouse_camera.hpp"

#include <SDL.h>

namespace qol {

class MouseCameraSdl {
public:
    ~MouseCameraSdl();
    void event(const SDL_Event& event, SDL_Window* window, const MouseScreen& screen);
    void frame(SDL_Window* window, bool menuOpen, const MouseScreen& screen);
    float take(float dt) { return camera_.take(dt); }
    const MouseCamera& camera() const { return camera_; }

private:
    void apply(bool captured);

    MouseCamera camera_;
    bool relative_ = false;
};

}
