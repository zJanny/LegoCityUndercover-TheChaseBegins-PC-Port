#pragma once
// Draws the super brick count display of runtime/super_brick_display.hpp into the top screen canvas: the number and the spinning brick icon (the
// model hud_platebrickmodel = models/pickup_debris_small, spun and tilted as 0x27b3e4 does).
#include "frontend/canvas.hpp"
#include "frontend/resources.hpp"
#include "runtime/super_brick_display.hpp"

#include <vector>

namespace chase::frontend {

// STAND-IN HUD-S74: the icon is rasterised on the CPU with an orthographic projection, a fitted pixel scale and offset, the texture
// Pickup_Debris.tga and a fitted light factor; the original draws the model with the HUD camera and the fader's two lights (0x1fa43c).
// STAND-IN HUD-S75: the number takes the front-end font with a one pixel black shadow; the font 0x3757a0 and the outline / shadow calls 0x235cb0 and 0x235c9c
// were not identified, and the bounce scale of the number is not applied.
class SuperBrickDisplayDraw {
public:
    explicit SuperBrickDisplayDraw(Resources& resources) : resources_(resources) {}
    void draw(Canvas& top, const runtime::SuperBrickDisplayView& view);

private:
    struct Vertex {
        float x, y, z, u, v;
    };
    void load();
    void drawIcon(Canvas& top, float centreX, float centreY, float spin);

    Resources& resources_;
    bool loaded_ = false;
    std::vector<Vertex> vertices_;
    std::vector<unsigned> indices_;
    float centreY_ = 0;
};

}
