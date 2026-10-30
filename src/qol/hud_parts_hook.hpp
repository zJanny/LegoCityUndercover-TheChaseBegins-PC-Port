#pragma once

#include "frontend/canvas.hpp"
#include "frontend/hud.hpp"
#include "qol/hud_hook.hpp"
#include "render/hud_overlay.hpp"

#include <vector>

namespace qol {

struct HudPartDraw {
    HudPart part;
    chase::frontend::HudPart frontendPart;
    chase::render::PixelRect rect;
};

std::vector<HudPartDraw> hudPartDraws(int windowWidth, const chase::render::PixelRect& topRect, CameraKind camera);

template <class HudT, class CanvasT, class SinkT>
void drawHudPartsWith(HudT& hud, CanvasT& canvas, double spinSeconds, const std::vector<HudPartDraw>& draws, SinkT&& sink) {
    for (const HudPartDraw& draw : draws) {
        canvas.clearTransparent();
        hud.draw(canvas, spinSeconds, chase::frontend::HudPartSet(draw.frontendPart));
        sink(canvas, draw.rect);
    }
    canvas.clearTransparent();
}

bool drawHudParts(chase::frontend::Hud& hud, chase::frontend::Canvas& canvas, chase::render::HudOverlay& overlay, double spinSeconds,
                  int windowWidth, int windowHeight, const chase::render::PixelRect& topRect, CameraKind camera);

}
