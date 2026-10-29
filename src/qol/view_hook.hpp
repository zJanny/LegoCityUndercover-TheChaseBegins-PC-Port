#pragma once

#include "render/view_region.hpp"
#include "render/window.hpp"

namespace qol {

chase::render::ViewRegion widenedViewRegion(const chase::render::PixelRect& area);

chase::render::ViewRegion topViewRegion(int windowWidth, int windowHeight, const chase::render::PixelRect& topRect);

}
