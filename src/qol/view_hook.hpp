#pragma once

#include "render/view_region.hpp"
#include "render/window.hpp"

namespace qol {

enum class CameraKind { Gameplay, Cutscene, Director, Scanner };

CameraKind cameraKindFor(bool cutsceneCamera, bool directorCamera, bool scannerCamera);

chase::render::ViewRegion widenedViewRegion(const chase::render::PixelRect& area);

chase::render::ViewRegion topViewRegion(int windowWidth, int windowHeight, const chase::render::PixelRect& topRect, CameraKind camera);

}
