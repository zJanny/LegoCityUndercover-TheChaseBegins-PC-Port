#include "qol/front_panel.hpp"

#include "qol/active_settings.hpp"

#include <algorithm>

namespace qol {

std::optional<FrontPanels> frontPanelsFor(const Settings& settings, int windowWidth, int windowHeight, int topWidth, int screenHeight) {
    if (!bottomScreenHidden(settings) || windowWidth <= 0 || windowHeight <= 0 || topWidth <= 0 || screenHeight <= 0) return std::nullopt;
    const float scale = std::min(float(windowWidth) / topWidth, float(windowHeight) / screenHeight);
    const float width = scale * topWidth, height = scale * screenHeight;
    return FrontPanels{{(float(windowWidth) - width) / 2, (float(windowHeight) - height) / 2, width, height},
        menuPageRect(windowWidth, windowHeight)};
}

std::optional<FrontPanels> frontPanels(int windowWidth, int windowHeight, int topWidth, int screenHeight) {
    return frontPanelsFor(activeSettings(), windowWidth, windowHeight, topWidth, screenHeight);
}

void darken(std::uint8_t* rgba, std::size_t pixelCount, float factor) {
    const float clamped = std::clamp(factor, 0.0f, 1.0f);
    for (std::size_t i = 0; i < pixelCount; ++i)
        for (int channel = 0; channel < 3; ++channel) rgba[i * 4 + channel] = std::uint8_t(rgba[i * 4 + channel] * clamped);
}

ClickAction classifyClick(const std::vector<ClickTarget>& targets, float x, float y) {
    for (const ClickTarget& target : targets) {
        if (!contains(HitArea{target.box, false, std::nullopt, std::nullopt}, x, y)) continue;
        if (target.isRow) return {target.row, FrontButton::A};
        if (target.button != FrontButton::None) return {std::nullopt, target.button};
    }
    return {};
}

}
