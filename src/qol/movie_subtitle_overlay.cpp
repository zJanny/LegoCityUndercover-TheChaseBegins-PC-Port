#include "qol/movie_subtitle_overlay.hpp"

#include "frontend/pages.hpp"
#include "qol/active_settings.hpp"
#include "qol/bottom_hook.hpp"

namespace qol {

namespace fe = chase::frontend;

std::optional<BubbleOffset> movieBubbleOffset(const Settings& settings, const fe::IntRect& box, int pictureWidth, int pictureHeight) {
    if (!bottomScreenHidden(settings) || box.width <= 0 || box.height <= 0) return std::nullopt;
    return BubbleOffset{(pictureWidth - fe::kBottomWidth) / 2, pictureHeight - 2 * box.y - box.height};
}

std::optional<FrontPanels> frontMoviePanelsFor(const Settings& settings, bool movieShowing, int windowWidth, int windowHeight, int topWidth,
    int screenHeight) {
    auto panels = frontPanelsFor(settings, windowWidth, windowHeight, topWidth, screenHeight);
    if (panels && movieShowing) panels->panel = {0, 0, 0, 0, 0};
    return panels;
}

bool drawMovieSubtitlesFor(const Settings& settings, fe::Canvas& picture, fe::Resources& resources, const std::vector<std::string>& lines) {
    if (!bottomScreenHidden(settings) || lines.empty()) return false;
    const auto offset = movieBubbleOffset(settings, fe::subtitleBoxRect(resources, lines), picture.width(), picture.height());
    if (!offset) return false;
    fe::drawSubtitleBox(picture, resources, lines, offset->x, offset->y);
    return true;
}

bool drawMovieSubtitles(fe::Canvas& picture, fe::Resources& resources, const std::vector<std::string>& lines) {
    return drawMovieSubtitlesFor(activeSettings(), picture, resources, lines);
}

}
