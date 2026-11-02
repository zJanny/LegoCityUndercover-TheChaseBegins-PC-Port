#include "qol/view_hook.hpp"

#include "qol/active_settings.hpp"
#include "qol/bottom_hook.hpp"
#include "qol/view_layout.hpp"

namespace qol {

chase::render::ViewRegion widenedViewRegion(const chase::render::PixelRect& area) {
    const ViewLayout layout = layoutInArea(area.width, area.height);
    const chase::render::PixelRect rect{area.x + layout.view.x, area.y + area.height - layout.view.y - layout.view.height, layout.view.width,
                                        layout.view.height};
    return chase::render::makeViewRegion(rect, layout.aspectWidth, layout.aspectHeight);
}

CameraKind cameraKindFor(bool cutsceneCamera, bool directorCamera, bool scannerCamera, bool moviePlaying) {
    if (moviePlaying) return CameraKind::Movie;
    if (cutsceneCamera) return CameraKind::Cutscene;
    if (scannerCamera) return CameraKind::Scanner;
    return directorCamera ? CameraKind::Director : CameraKind::Gameplay;
}

bool cameraWidens(CameraKind camera) { return camera == CameraKind::Gameplay; }

chase::render::ViewRegion topViewRegion(int windowWidth, int windowHeight, const chase::render::PixelRect& topRect, CameraKind camera) {
    if (!enabled(Feature::WidenedView) || !cameraWidens(camera) || topRect.width <= 0 || topRect.height <= 0) return chase::render::originalViewRegion(topRect);
    if (bottomScreenHidden(activeSettings())) return widenedViewRegion(hiddenTopArea(windowWidth, windowHeight));
    return widenedViewRegion({0, topRect.y, windowWidth, topRect.height});
}

}
