#include "runtime/dialog_bg_fade.hpp"

#include <algorithm>

namespace chase::runtime {

void DialogBackgroundFade::update(bool tipShown, bool mapBlackout) {
    const float next = tipShown || mapBlackout ? alpha_ + kPanelAlphaStep : alpha_ - kPanelAlphaStep;
    alpha_ = std::clamp(next, 0.0f, 1.0f);
}

float DialogBackgroundFade::drawnAlpha() const {
    return float(int(alpha_ * 255.0f)) / 255.0f;
}

}
