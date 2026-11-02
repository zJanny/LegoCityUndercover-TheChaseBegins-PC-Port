#pragma once

#include "frontend/canvas.hpp"
#include "qol/active_settings.hpp"
#include "qol/bottom_hook.hpp"
#include "render/hud_overlay.hpp"

#include <optional>

namespace qol {

constexpr int minimapHeightDivisor = 5;
constexpr int countersBottomOriginal = 62;
constexpr int minimapMarginOriginal = 4;

struct MinimapPanels {
    bool scanner = false;
    bool dialogBox = false;
    bool speechBox = false;
    bool sceneChange = false;
    bool fastTravelMap = false;
    bool movie = false;
    bool any() const { return scanner || dialogBox || speechBox || sceneChange || fastTravelMap || movie; }
};

struct MinimapPlacement {
    int diameter = 0;
    chase::render::PixelRect rect;
};

std::optional<MinimapPlacement> minimapPlacement(int windowWidth, int windowHeight);

void roundMinimapCanvas(const chase::frontend::Canvas& map, chase::frontend::Canvas& round);

template <class SinkT>
bool drawRoundMinimapWith(const Settings& settings, const chase::frontend::Canvas& map, chase::frontend::Canvas& round, const MinimapPanels& panels,
                          int windowWidth, int windowHeight, SinkT&& sink) {
    if (!minimapOn(settings) || panels.any()) return false;
    const std::optional<MinimapPlacement> placement = minimapPlacement(windowWidth, windowHeight);
    if (!placement) return false;
    roundMinimapCanvas(map, round);
    sink(round, placement->rect);
    return true;
}

bool drawRoundMinimap(const chase::frontend::Canvas& map, chase::render::HudOverlay& overlay, const MinimapPanels& panels, int windowWidth, int windowHeight);

}
