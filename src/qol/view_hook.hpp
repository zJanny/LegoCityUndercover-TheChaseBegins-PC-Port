#pragma once

#include "render/view_region.hpp"

namespace qol {

chase::render::ViewRegion widenedViewRegion(int windowWidth, const chase::render::PixelRect& topRect);

chase::render::ViewRegion topViewRegion(int windowWidth, const chase::render::PixelRect& topRect);

}
