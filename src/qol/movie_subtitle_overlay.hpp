#pragma once

#include "frontend/canvas.hpp"
#include "frontend/resources.hpp"
#include "qol/front_panel.hpp"
#include "qol/settings.hpp"

#include <optional>
#include <string>
#include <vector>

namespace qol {

struct BubbleOffset {
    int x, y;
};

std::optional<BubbleOffset> movieBubbleOffset(const Settings& settings, const chase::frontend::IntRect& box, int pictureWidth, int pictureHeight);

std::optional<FrontPanels> frontMoviePanelsFor(const Settings& settings, bool movieShowing, int windowWidth, int windowHeight, int topWidth,
    int screenHeight);

bool drawMovieSubtitlesFor(const Settings& settings, chase::frontend::Canvas& picture, chase::frontend::Resources& resources,
    const std::vector<std::string>& lines);
bool drawMovieSubtitles(chase::frontend::Canvas& picture, chase::frontend::Resources& resources, const std::vector<std::string>& lines);

}
