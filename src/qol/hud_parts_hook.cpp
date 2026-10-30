#include "qol/hud_parts_hook.hpp"

#include "qol/active_settings.hpp"

namespace qol {

std::vector<HudPartDraw> hudPartDraws(int windowWidth, const chase::render::PixelRect& topRect, CameraKind camera) {
    if (!enabled(Feature::WidenedView)) return {};
    using chase::frontend::HudPart;
    const std::pair<qol::HudPart, HudPart> mapping[] = {{qol::HudPart::Hearts, HudPart::Hearts},
                                                        {qol::HudPart::Portrait, HudPart::Portrait},
                                                        {qol::HudPart::Studs, HudPart::Studs},
                                                        {qol::HudPart::Counters, HudPart::SuperBricks},
                                                        {qol::HudPart::Ability, HudPart::CameraLock}};
    std::vector<HudPartDraw> draws;
    for (const auto& [part, frontendPart] : mapping) draws.push_back({part, frontendPart, hudPartRect(part, windowWidth, topRect, camera)});
    return draws;
}

bool drawHudParts(chase::frontend::Hud& hud, chase::frontend::Canvas& canvas, chase::render::HudOverlay& overlay, double spinSeconds,
                  int windowWidth, int windowHeight, const chase::render::PixelRect& topRect, CameraKind camera) {
    const std::vector<HudPartDraw> draws = hudPartDraws(windowWidth, topRect, camera);
    if (draws.empty()) return false;
    drawHudPartsWith(hud, canvas, spinSeconds, draws, [&](const chase::frontend::Canvas& drawn, const chase::render::PixelRect& rect) {
        overlay.drawTop({drawn.width(), drawn.height(), drawn.rgba().data()}, rect, windowWidth, windowHeight);
    });
    return true;
}

}
