#include "qol/front_panel_clicks.hpp"

namespace qol {

ClickAction panelClick(const PageRect& panel, const std::vector<ClickTarget>& targets, float windowX, float windowY) {
    const auto point = pagePoint(panel, windowX, windowY);
    if (!point) return {};
    return classifyClick(targets, (*point)[0], (*point)[1]);
}

void FrontClickQueue::add(const ClickAction& action) {
    if (action.any()) pending_ = action;
}

ClickAction FrontClickQueue::take() {
    const ClickAction action = pending_;
    pending_ = {};
    return action;
}

}
