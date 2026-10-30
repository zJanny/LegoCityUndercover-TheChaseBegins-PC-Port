#include "qol/front_panel_present.hpp"

#include "qol/front_panel.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace qol {

bool presentFrontPanels(chase::render::FrontPresenter& presenter, const chase::render::Pixels& top, const chase::render::Pixels& bottom,
    int windowWidth, int windowHeight) {
    const auto panels = frontPanels(windowWidth, windowHeight, top.width, top.height);
    if (!panels) return false;
    static std::vector<std::uint8_t> dimmed;
    const std::size_t count = std::size_t(top.width) * std::size_t(top.height);
    dimmed.assign(top.rgba, top.rgba + count * 4);
    darken(dimmed.data(), count, frontTopDim);
    const chase::render::FrontBox topBox{panels->top.x, panels->top.y, panels->top.width, panels->top.height};
    const chase::render::FrontBox panelBox{panels->panel.x, panels->panel.y, panels->panel.width, panels->panel.height};
    presenter.presentBoxes({top.width, top.height, dimmed.data()}, bottom, windowWidth, windowHeight, topBox, panelBox);
    return true;
}

}
