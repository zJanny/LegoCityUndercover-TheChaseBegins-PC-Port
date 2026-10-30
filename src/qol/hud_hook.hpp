#pragma once

#include "qol/view_hook.hpp"
#include "render/window.hpp"

namespace qol {

enum class HudPart { Hearts, Portrait, Studs, Counters, Ability };

chase::render::PixelRect hudPartRect(HudPart part, int windowWidth, const chase::render::PixelRect& topRect, CameraKind camera);

}
