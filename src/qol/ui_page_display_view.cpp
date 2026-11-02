#include "qol/ui_page_display.hpp"

#include "qol/view_distance.hpp"

#include <algorithm>
#include <cmath>

namespace qol::ui {

const char* const viewDistanceRowText = "View distance";

RowSpec DisplayPage::viewDistanceRow() const {
    const int step = effectiveViewDistanceStep(settings_);
    RowSpec row{viewDistanceRowText, RowKind::Slider, viewDistanceLabel(step),
        static_cast<double>(step) / viewDistanceSteps, settings_.master() != Master::Original};
    row.steps = viewDistanceSteps;
    for (int candidate = 0; candidate <= viewDistanceSteps; ++candidate)
        if (const std::string label = viewDistanceLabel(candidate); label.size() > row.widestValue.size()) row.widestValue = label;
    return row;
}

DisplayPageResult DisplayPage::changeViewDistance(const Action& action) {
    if (settings_.master() == Master::Original) return {};
    const int step = std::clamp(static_cast<int>(std::lround(action.fraction * viewDistanceSteps)), 0, viewDistanceSteps);
    if (step == settings_.viewDistanceStep()) return {};
    settings_.setViewDistanceStep(step);
    DisplayPageResult result;
    result.settingsChanged = true;
    return result;
}

bool DisplayPage::resetViewDistance() {
    if (settings_.viewDistanceStep() == 0) return false;
    settings_.setViewDistanceStep(0);
    return true;
}

}
