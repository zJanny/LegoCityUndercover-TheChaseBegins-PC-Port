#include "qol/hud_hook.hpp"

#include "qol/active_settings.hpp"
#include "qol/view_layout.hpp"

#include <cmath>

namespace qol {

namespace {

Corner cornerOf(HudPart part) {
    switch (part) {
    case HudPart::Hearts:
    case HudPart::Portrait: return Corner::TopLeft;
    case HudPart::Studs:
    case HudPart::Counters: return Corner::TopRight;
    case HudPart::Ability: return Corner::BottomLeft;
    }
    return Corner::TopLeft;
}

}

chase::render::PixelRect hudPartRect(HudPart part, int windowWidth, const chase::render::PixelRect& topRect) {
    if (!enabled(Feature::WidenedView) || topRect.width <= 0 || topRect.height <= 0) return topRect;
    const ViewLayout layout = layoutInArea(windowWidth, topRect.height);
    const HudAnchor anchor = hudAnchor(layout, cornerOf(part));
    const HudPoint origin = hudPosition(anchor, 0, 0);
    const int width = static_cast<int>(std::lround(originalViewWidth * anchor.scale));
    const int height = static_cast<int>(std::lround(originalViewHeight * anchor.scale));
    const int left = static_cast<int>(std::lround(origin.x));
    const int top = static_cast<int>(std::lround(origin.y));
    return {left, topRect.y + topRect.height - top - height, width, height};
}

}
