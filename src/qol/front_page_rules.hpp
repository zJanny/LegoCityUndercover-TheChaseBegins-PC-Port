#pragma once

#include "frontend/flow.hpp"
#include "qol/front_panel.hpp"

#include <optional>

namespace qol {

enum class PageView { Panel, PictureOnly };

PageView frontPageView(chase::frontend::Screen screen, bool blackout);

std::optional<FrontPanels> frontPagePanelsFor(const Settings& settings, chase::frontend::Screen screen, bool blackout, int windowWidth,
    int windowHeight, int topWidth, int screenHeight);

struct SpinnerPlace {
    float x, y, size;
};

std::optional<SpinnerPlace> loadingSpinnerFor(const Settings& settings, chase::frontend::Screen screen, bool blackout);

}
