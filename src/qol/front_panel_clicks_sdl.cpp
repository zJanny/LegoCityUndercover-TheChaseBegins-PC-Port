#include "qol/front_panel_clicks_sdl.hpp"

namespace qol {

namespace fe = chase::frontend;

namespace {

FrontButton buttonOf(std::uint32_t pad) {
    if (pad == fe::kButtonA) return FrontButton::A;
    if (pad == fe::kButtonB) return FrontButton::B;
    if (pad == fe::kButtonY) return FrontButton::Y;
    return FrontButton::None;
}

std::uint32_t padOf(FrontButton button) {
    switch (button) {
    case FrontButton::A: return fe::kButtonA;
    case FrontButton::B: return fe::kButtonB;
    case FrontButton::Y: return fe::kButtonY;
    case FrontButton::None: break;
    }
    return 0;
}

Box boxOf(const fe::TargetBox& box) {
    return {box.x, box.y, box.width, box.height};
}

}

std::vector<ClickTarget> clickTargets(const fe::PageTargets& targets) {
    std::vector<ClickTarget> out;
    for (const fe::ButtonTarget& button : targets.buttons) out.push_back({boxOf(button.box), false, 0, buttonOf(button.button)});
    for (std::size_t row = 0; row < targets.rows.size(); ++row) out.push_back({boxOf(targets.rows[row]), true, row, FrontButton::None});
    return out;
}

bool FrontClicks::consume(const SDL_Event& event, int windowWidth, int windowHeight, const fe::FrontEnd& front) {
    if (event.type != SDL_MOUSEBUTTONDOWN || event.button.button != SDL_BUTTON_LEFT) return false;
    const auto panels = frontPanels(windowWidth, windowHeight, fe::kTopWidth, fe::kScreenHeight);
    if (!panels) return false;
    const fe::PageTargets targets = front.targets();
    if (targets.empty()) return false;
    queue_.add(panelClick(panels->panel, clickTargets(targets), float(event.button.x), float(event.button.y)));
    return true;
}

void FrontClicks::apply(fe::FrontInput& pad) {
    const ClickAction action = queue_.take();
    if (action.selectRow) pad.selectRow = unsigned(*action.selectRow);
    pad.pressed |= padOf(action.press);
}

}
