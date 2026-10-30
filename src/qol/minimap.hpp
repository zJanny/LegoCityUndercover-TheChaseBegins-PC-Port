#pragma once

#include "frontend/canvas.hpp"
#include "qol/active_settings.hpp"
#include "qol/bottom_hook.hpp"
#include "render/hud_overlay.hpp"

#include <optional>

namespace qol {

constexpr int minimapHeightDivisor = 5;

struct MinimapPlacement {
    int diameter = 0;
    chase::render::PixelRect rect;
};

std::optional<MinimapPlacement> minimapPlacement(int windowWidth, int windowHeight);

void roundMinimapCanvas(const chase::frontend::Canvas& map, chase::frontend::Canvas& round);

template <class SinkT>
bool drawRoundMinimapWith(const Settings& settings, const chase::frontend::Canvas& map, chase::frontend::Canvas& round, bool panelOpen,
                          int windowWidth, int windowHeight, SinkT&& sink) {
    if (!minimapOn(settings) || panelOpen) return false;
    const std::optional<MinimapPlacement> placement = minimapPlacement(windowWidth, windowHeight);
    if (!placement) return false;
    roundMinimapCanvas(map, round);
    sink(round, placement->rect);
    return true;
}

bool drawRoundMinimap(const chase::frontend::Canvas& map, chase::render::HudOverlay& overlay, bool panelOpen, int windowWidth, int windowHeight);

}
