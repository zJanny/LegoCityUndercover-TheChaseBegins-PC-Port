#pragma once

#include "qol/settings.hpp"
#include "render/window.hpp"

#include <optional>

namespace qol {

struct HiddenScreens {
    chase::render::PixelRect top;
    chase::render::PixelRect bottom;
};

bool minimapOn(const Settings& settings);
bool bottomScreenHidden(const Settings& settings);

std::optional<HiddenScreens> hiddenScreensFor(const Settings& settings, int windowWidth, int windowHeight, int topWidth, int screenHeight);

std::optional<HiddenScreens> hiddenScreens(int windowWidth, int windowHeight, int topWidth, int screenHeight);

chase::render::PixelRect hiddenTopArea(int windowWidth, int windowHeight);

}
