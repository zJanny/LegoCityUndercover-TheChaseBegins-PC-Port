#include "qol/front_panel_present.hpp"

#include "qol/active_settings.hpp"
#include "qol/front_page_rules.hpp"
#include "qol/front_panel.hpp"
#include "qol/movie_subtitle_overlay.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace qol {

namespace fe = chase::frontend;

bool presentFrontPanels(chase::render::FrontPresenter& presenter, const fe::Canvas& top, const chase::render::Pixels& bottom,
    fe::Resources& resources, const fe::FrontEnd& front, int windowWidth, int windowHeight) {
    const bool movie = front.flow().movieShowing();
    const bool blackout = front.flow().blackout();
    const auto screen = front.flow().screen();
    const auto panels = frontPagePanelsFor(activeSettings(), screen, blackout, windowWidth, windowHeight, top.width(), top.height());
    if (!panels) return false;
    const chase::render::FrontBox topBox{panels->top.x, panels->top.y, panels->top.width, panels->top.height};
    const chase::render::FrontBox panelBox{panels->panel.x, panels->panel.y, panels->panel.width, panels->panel.height};
    if (movie) {
        static fe::Canvas picture(top.width(), top.height());
        picture = top;
        drawMovieSubtitles(picture, resources, front.subtitleLines());
        presenter.presentBoxes({picture.width(), picture.height(), picture.rgba().data()}, bottom, windowWidth, windowHeight, topBox, panelBox);
        return true;
    }
    if (frontPageView(screen, blackout) == PageView::PictureOnly) {
        presenter.presentBoxes({top.width(), top.height(), top.rgba().data()}, bottom, windowWidth, windowHeight, topBox, panelBox);
        return true;
    }
    static std::vector<std::uint8_t> dimmed;
    const std::size_t count = std::size_t(top.width()) * std::size_t(top.height());
    dimmed.assign(top.rgba().begin(), top.rgba().end());
    darken(dimmed.data(), count, frontTopDim);
    presenter.presentBoxes({top.width(), top.height(), dimmed.data()}, bottom, windowWidth, windowHeight, topBox, panelBox);
    return true;
}

}
