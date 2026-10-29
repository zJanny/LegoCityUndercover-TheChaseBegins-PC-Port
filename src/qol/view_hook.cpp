#include "qol/view_hook.hpp"

#include "qol/active_settings.hpp"
#include "qol/view_layout.hpp"

namespace qol {

chase::render::ViewRegion widenedViewRegion(int windowWidth, const chase::render::PixelRect& topRect) {
    const ViewLayout layout = layoutInArea(windowWidth, topRect.height);
    const chase::render::PixelRect rect{layout.view.x, topRect.y + topRect.height - layout.view.y - layout.view.height, layout.view.width,
                                        layout.view.height};
    return chase::render::makeViewRegion(rect, layout.aspectWidth, layout.aspectHeight);
}

chase::render::ViewRegion topViewRegion(int windowWidth, const chase::render::PixelRect& topRect) {
    if (!enabled(Feature::WidenedView) || topRect.width <= 0 || topRect.height <= 0) return chase::render::originalViewRegion(topRect);
    return widenedViewRegion(windowWidth, topRect);
}

}
