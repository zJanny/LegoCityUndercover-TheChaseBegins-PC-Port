#include "qol/minimap.hpp"

#include "qol/view_layout.hpp"

#include <algorithm>
#include <cmath>

namespace qol {

namespace {

constexpr float mapCentreX = 160.0f;
constexpr float roundRadius = 120.0f;

}

std::optional<MinimapPlacement> minimapPlacement(int windowWidth, int windowHeight) {
    if (windowWidth <= 0 || windowHeight <= 0) return std::nullopt;
    const int diameter = std::max(1, int(std::lround(double(windowHeight) / minimapHeightDivisor)));
    const double scale = double(diameter) / (2 * roundRadius);
    const int canvasWidth = chase::frontend::kTopWidth;
    const int width = int(std::lround(canvasWidth * scale));
    const int rightOfCircle = int(std::lround((canvasWidth / 2 + roundRadius) * scale));
    const HudAnchor anchor = hudAnchor(layoutInArea(windowWidth, windowHeight), Corner::TopRight);
    const double belowCounters = hudPosition(anchor, 0, countersBottomOriginal + minimapMarginOriginal).y;
    const int top = std::clamp(int(std::lround(belowCounters)), 0, std::max(0, windowHeight - diameter));
    return MinimapPlacement{diameter, {windowWidth - rightOfCircle, windowHeight - top - diameter, width, diameter}};
}

void roundMinimapCanvas(const chase::frontend::Canvas& map, chase::frontend::Canvas& round) {
    round.clearTransparent();
    const int offset = round.width() / 2 - int(mapCentreX);
    const float centreY = float(round.height()) / 2;
    for (int y = 0; y < round.height() && y < map.height(); ++y)
        for (int x = 0; x < round.width(); ++x) {
            const int sourceX = x - offset;
            if (sourceX < 0 || sourceX >= map.width()) continue;
            const float dx = float(x) + 0.5f - float(round.width()) / 2, dy = float(y) + 0.5f - centreY;
            const float cover = std::clamp(roundRadius - std::sqrt(dx * dx + dy * dy) + 0.5f, 0.0f, 1.0f);
            if (cover <= 0) continue;
            const std::uint8_t* from = map.pixel(sourceX, y);
            std::uint8_t* to = round.pixel(x, y);
            to[0] = from[0];
            to[1] = from[1];
            to[2] = from[2];
            to[3] = std::uint8_t(std::lround(float(from[3]) * cover));
        }
}

bool drawRoundMinimap(const chase::frontend::Canvas& map, chase::render::HudOverlay& overlay, const MinimapPanels& panels, int windowWidth, int windowHeight) {
    static chase::frontend::Canvas round(chase::frontend::kTopWidth, chase::frontend::kScreenHeight);
    return drawRoundMinimapWith(activeSettings(), map, round, panels, windowWidth, windowHeight,
                                [&](const chase::frontend::Canvas& drawn, const chase::render::PixelRect& rect) {
                                    overlay.drawTop({drawn.width(), drawn.height(), drawn.rgba().data()}, rect, windowWidth, windowHeight);
                                });
}

}
