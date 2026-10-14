#pragma once
// Two-screen overlay of chase-game: the top and bottom front-end canvases drawn as textured quads over the scene.
#include "render/window.hpp"
#include <memory>

namespace chase::render {

class HudOverlayImpl;

class HudOverlay {
public:
    HudOverlay(int topWidth, int bottomWidth, int screenHeight);
    ~HudOverlay();
    HudOverlay(const HudOverlay&) = delete;
    HudOverlay& operator=(const HudOverlay&) = delete;

    void draw(const Pixels& top, const Pixels& bottom, const PixelRect& topRect, const PixelRect& bottomRect, int windowWidth, int windowHeight);

private:
    std::unique_ptr<HudOverlayImpl> impl_;
};

}
