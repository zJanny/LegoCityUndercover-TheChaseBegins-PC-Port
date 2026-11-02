#include "qol/bottom_hook.hpp"

#include "qol/active_settings.hpp"

#include <algorithm>
#include <cmath>

namespace qol {

bool minimapOn(const Settings& settings) { return settings.enabled(Feature::WidenedView); }

bool bottomScreenHidden(const Settings& settings) { return minimapOn(settings); }

chase::render::PixelRect hiddenTopArea(int windowWidth, int windowHeight) { return {0, 0, std::max(windowWidth, 0), std::max(windowHeight, 0)}; }

std::optional<HiddenScreens> hiddenScreensFor(const Settings& settings, int windowWidth, int windowHeight, int topWidth, int screenHeight) {
    if (!bottomScreenHidden(settings) || windowWidth <= 0 || windowHeight <= 0 || topWidth <= 0 || screenHeight <= 0) return std::nullopt;
    const float scale = std::min(float(windowWidth) / topWidth, float(windowHeight) / screenHeight);
    const float width = scale * topWidth, height = scale * screenHeight;
    const int left = int(std::lround((float(windowWidth) - width) / 2)), right = int(std::lround((float(windowWidth) + width) / 2));
    const int bottom = int(std::lround((float(windowHeight) - height) / 2)), top = int(std::lround((float(windowHeight) + height) / 2));
    return HiddenScreens{{left, bottom, right - left, top - bottom}, {0, 0, 0, 0}};
}

std::optional<HiddenScreens> hiddenScreens(int windowWidth, int windowHeight, int topWidth, int screenHeight) {
    return hiddenScreensFor(activeSettings(), windowWidth, windowHeight, topWidth, screenHeight);
}

}
