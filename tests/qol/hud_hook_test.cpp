// Widened view hides the bottom screen in every preset (user answer 2026-11-02, qol-screen-layout.md); expectations for it recomputed by 20261102-qolmenu-B on 2026-11-02 from the same rules over the whole window (scale = window height / 240).
// 5:3 cameras: 20261030-qolmenu-C, 2026-10-30: the picture is the region of qol::topViewRegion for a non-gameplay camera (user answer "Move to picture corners", docs/design/qol-screen-layout.md §2026-10-29T11:42:22).
// Spec: docs/design/qol-screen-layout.md §2026-10-28T09:37:31 (standard HUD corners: hearts and character picture top-left, studs
// and counters top-right, ability bottom-left; scaled to the window height), §2026-10-28T22:21:19 (anchors at the view's corners,
// scale = view height / 240) and §2026-10-29T11:42:22 (corners of the 16:9 area; area is the top screen's rows).
// Pinned rectangles computed by hand by 20261029-qolmenu-D on 2026-10-29 from game_hud::screensFor and those rules; GL rectangles
// count y from the window bottom. Each rectangle is where a 400x240 canvas holding one HUD part is drawn.
#include "game_hud.hpp"
#include "qol/active_settings.hpp"
#include "qol/hud_hook.hpp"
#include "qol/settings.hpp"

#include <iostream>
#include <string>

namespace {

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

bool same(const chase::render::PixelRect& a, const chase::render::PixelRect& b) {
    return a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height;
}

constexpr qol::CameraKind allCameras[] = {qol::CameraKind::Gameplay, qol::CameraKind::Cutscene, qol::CameraKind::Director,
                                         qol::CameraKind::Scanner};

constexpr qol::HudPart allParts[] = {qol::HudPart::Hearts, qol::HudPart::Portrait, qol::HudPart::Studs, qol::HudPart::Counters,
                                     qol::HudPart::Ability};

void expectRect(qol::HudPart part, int width, int height, chase::render::PixelRect rect, const std::string& name) {
    const chase::render::PixelRect got = qol::hudPartRect(part, width, game_hud::screensFor(width, height).top, qol::CameraKind::Gameplay);
    if (!same(got, rect))
        require(false, name + ": got " + std::to_string(got.x) + "," + std::to_string(got.y) + "," + std::to_string(got.width) + "," +
                           std::to_string(got.height));
}

void expectRectFor(qol::HudPart part, int width, int height, qol::CameraKind camera, chase::render::PixelRect rect, const std::string& name) {
    const chase::render::PixelRect got = qol::hudPartRect(part, width, game_hud::screensFor(width, height).top, camera);
    if (!same(got, rect))
        require(false, name + ": got " + std::to_string(got.x) + "," + std::to_string(got.y) + "," + std::to_string(got.width) + "," +
                           std::to_string(got.height));
}

void originalModeFirst() {
    require(!qol::enabled(qol::Feature::WidenedView), "default store has the widened view off");
    for (int width = 400; width <= 2600; width += 37)
        for (int height = 240; height <= 1500; height += 29) {
            const chase::render::PixelRect top = game_hud::screensFor(width, height).top;
            for (qol::HudPart part : allParts)
                for (const qol::CameraKind camera : allCameras)
                    if (!same(qol::hudPartRect(part, width, top, camera), top))
                        require(false, "off: part rectangle equals today's top rectangle at " + std::to_string(width) + "x" + std::to_string(height));
        }
}

void pictureCorners() {
    qol::Settings settings;
    settings.setSwitch(qol::Feature::WidenedView, true);
    settings.setMaster(qol::Master::Custom);
    qol::setActiveSettings(settings);
    const qol::CameraKind pictureCameras[] = {qol::CameraKind::Cutscene, qol::CameraKind::Director, qol::CameraKind::Scanner};
    const int sizes[][2] = {{1920, 1080}, {3000, 1000}, {800, 1200}, {400, 240}};
    for (const auto& size : sizes) {
        const chase::render::PixelRect top = game_hud::screensFor(size[0], size[1]).top;
        for (const qol::CameraKind camera : pictureCameras) {
            const chase::render::PixelRect picture = qol::topViewRegion(size[0], size[1], top, camera).rect;
            for (qol::HudPart part : allParts)
                require(same(qol::hudPartRect(part, size[0], top, camera), picture),
                        "5:3 camera: part rectangle is the picture at " + std::to_string(size[0]) + "x" + std::to_string(size[1]));
        }
    }
    expectRectFor(qol::HudPart::Studs, 1920, 1080, qol::CameraKind::Director, {60, 0, 1800, 1080}, "1920x1080 director: picture corners, not window corners");
    expectRectFor(qol::HudPart::Hearts, 1920, 1080, qol::CameraKind::Scanner, {60, 0, 1800, 1080}, "1920x1080 scanner: picture corners");
    expectRect(qol::HudPart::Studs, 1920, 1080, {120, 0, 1800, 1080}, "gameplay camera: still the widened corners");

    qol::Settings improved;
    improved.setMaster(qol::Master::Improved);
    qol::setActiveSettings(improved);
    expectRectFor(qol::HudPart::Studs, 1920, 1080, qol::CameraKind::Cutscene, {60, 0, 1800, 1080}, "Improved 1920x1080 cutscene: the picture centred");
    qol::setActiveSettings(qol::Settings{});
}

void widenedOn() {
    qol::Settings settings;
    settings.setSwitch(qol::Feature::WidenedView, true);
    settings.setMaster(qol::Master::Custom);
    qol::setActiveSettings(settings);

    for (qol::HudPart part : {qol::HudPart::Hearts, qol::HudPart::Portrait, qol::HudPart::Ability})
        expectRect(part, 1920, 1080, {0, 0, 1800, 1080}, "1920x1080 left parts at the view's left edge");
    for (qol::HudPart part : {qol::HudPart::Studs, qol::HudPart::Counters})
        expectRect(part, 1920, 1080, {120, 0, 1800, 1080}, "1920x1080 right parts end at the view's right edge");

    expectRect(qol::HudPart::Hearts, 1280, 720, {0, 0, 1200, 720}, "1280x720 top-left");
    expectRect(qol::HudPart::Studs, 1280, 720, {80, 0, 1200, 720}, "1280x720 top-right");

    expectRect(qol::HudPart::Hearts, 2520, 1080, {300, 0, 1800, 1080}, "21:9 window: left anchor inside the bar");
    expectRect(qol::HudPart::Studs, 2520, 1080, {420, 0, 1800, 1080}, "21:9 window: right anchor inside the bar");

    expectRect(qol::HudPart::Hearts, 800, 1200, {0, 360, 800, 480}, "800x1200 tall: 5:3 area over the whole window");
    expectRect(qol::HudPart::Studs, 800, 1200, {0, 360, 800, 480}, "800x1200 tall: right edge equals the 5:3 area");

    const chase::render::PixelRect empty{0, 0, 0, 0};
    require(same(qol::hudPartRect(qol::HudPart::Hearts, 0, empty, qol::CameraKind::Gameplay), empty), "empty drawable while on: unchanged, no exception");

    qol::setActiveSettings(qol::Settings{});
    expectRect(qol::HudPart::Studs, 1920, 1080, {510, 540, 900, 540}, "off again: today's top rectangle");
}

}

int main() {
    originalModeFirst();
    widenedOn();
    pictureCorners();
    if (failures) return 1;
    std::cout << "qol hud hook: ok\n";
    return 0;
}
