// Spec: docs/design/qol-screen-layout.md §2026-10-28T09:21:33, §2026-10-28T09:31:02, §2026-10-28T09:37:31 (widening between 5:3
// and 16:9, same vertical field of view) and the user's answers of 2026-10-29 recorded there ("Above bottom screen", "Top screen's
// rows"): the widened view fills the full window width at the rows of today's top screen.
// Pinned regions computed by hand from game_hud::screensFor and those rules by 20261029-qolview-A on 2026-10-29; the Improved
// regions over the whole window (QOLBOT-20) by 20261029-qolhook-C on 2026-10-29 from "Top screen's rows" + hidden bottom screen
// (floor rounding, odd pixel in the right bar: user answer to QOLFS-9). GL rectangles count y from the window bottom.
#include "game_hud.hpp"
#include "qol/active_settings.hpp"
#include "qol/menu_model.hpp"
#include "qol/view_hook.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

bool same(const chase::render::ViewRegion& a, const chase::render::ViewRegion& b) {
    return a.rect.x == b.rect.x && a.rect.y == b.rect.y && a.rect.width == b.rect.width && a.rect.height == b.rect.height &&
           a.aspectWidth == b.aspectWidth && a.aspectHeight == b.aspectHeight;
}

void expectRegion(int width, int height, chase::render::PixelRect rect, int aspectWidth, int aspectHeight, const std::string& name,
                  qol::CameraKind camera = qol::CameraKind::Gameplay) {
    const chase::render::ViewRegion region = qol::topViewRegion(width, height, game_hud::screensFor(width, height).top, camera);
    require(same(region, {rect, aspectWidth, aspectHeight}), name);
}

void originalModeFirst() {
    require(!qol::enabled(qol::Feature::WidenedView), "default store has the widened view off");
    for (int width = 400; width <= 2600; width += 37)
        for (int height = 240; height <= 1500; height += 29) {
            const chase::render::PixelRect top = game_hud::screensFor(width, height).top;
            if (!same(qol::topViewRegion(width, height, top, qol::CameraKind::Gameplay), chase::render::originalViewRegion(top)))
                require(false, "off: region equals the original at " + std::to_string(width) + "x" + std::to_string(height));
        }
}

void widenedOn() {
    qol::Settings settings;
    qol::MenuModel menu(settings);
    menu.open();
    menu.handle(qol::tabClick(static_cast<std::size_t>(qol::Tab::QualityOfLife)));
    menu.handle({qol::EventKind::Down});
    require(menu.handle({qol::EventKind::Activate}) == qol::Result::SwitchedToCustom, "menu turns the widened view on");
    qol::setActiveSettings(settings);
    require(qol::enabled(qol::Feature::WidenedView), "store reads the menu's switch");

    expectRegion(1920, 1080, {480, 540, 960, 540}, 960, 540, "1920x1080: 16:9 in the top screen's rows, bars beyond");
    expectRegion(400, 480, {0, 240, 400, 240}, 400, 240, "400x480: area is 5:3, original rectangle");
    expectRegion(400, 240, {93, 120, 213, 120}, 213, 120, "400x240 minimum window: 16:9 in a 400x120 area");
    expectRegion(800, 1200, {0, 600, 800, 480}, 800, 480, "800x1200 tall: the top screen's rows, 5:3");
    expectRegion(860, 1000, {0, 500, 860, 500}, 860, 500, "860x1000: area between 5:3 and 16:9 is the whole view");

    bool rejected = false;
    try {
        qol::widenedViewRegion({0, 0, 800, 0});
    } catch (const std::invalid_argument& error) {
        rejected = std::string(error.what()).find("view layout") != std::string::npos;
    }
    require(rejected, "empty top rectangle is rejected");
    require(same(qol::topViewRegion(0, 0, {0, 0, 0, 0}, qol::CameraKind::Gameplay), chase::render::originalViewRegion({0, 0, 0, 0})),
            "empty drawable while on: the original region, no exception from the draw path");

    menu.handle({qol::EventKind::Activate});
    qol::setActiveSettings(settings);
    require(!qol::enabled(qol::Feature::WidenedView), "menu turns it off again");
    expectRegion(1920, 1080, {510, 540, 900, 540}, 400, 240, "off again: today's top screen");

    qol::Settings improved;
    improved.setSwitch(qol::Feature::WidenedView, true);
    improved.setMaster(qol::Master::Custom);
    qol::setActiveSettings(improved);
    expectRegion(1920, 1080, {480, 540, 960, 540}, 960, 540, "Custom with the view on: widened above the bottom screen");

    improved.setMaster(qol::Master::Improved);
    qol::setActiveSettings(improved);
    expectRegion(1920, 1080, {0, 0, 1920, 1080}, 1920, 1080, "Improved: bottom screen hidden, widened over the whole window");
    expectRegion(3000, 1000, {611, 0, 1777, 1000}, 1777, 1000, "Improved 3000x1000: 16:9 in the whole window, bars at the sides");
    expectRegion(800, 1200, {0, 360, 800, 480}, 800, 480, "Improved 800x1200 tall: 5:3 centred, bars above and below");
    expectRegion(801, 1201, {0, 361, 801, 480}, 801, 480, "Improved 801x1201: floor rounding of the whole-window area");
    expectRegion(400, 240, {0, 0, 400, 240}, 400, 240, "Improved 400x240 minimum: the whole window");
}

void otherCamerasKeepFiveByThree() {
    const qol::CameraKind others[] = {qol::CameraKind::Cutscene, qol::CameraKind::Director, qol::CameraKind::Scanner};
    for (const qol::CameraKind camera : others) {
        qol::Settings settings;
        qol::setActiveSettings(settings);
        for (int width = 400; width <= 2600; width += 211)
            for (int height = 240; height <= 1500; height += 173) {
                const chase::render::PixelRect top = game_hud::screensFor(width, height).top;
                require(same(qol::topViewRegion(width, height, top, camera), chase::render::originalViewRegion(top)),
                        "off: non-gameplay camera region equals the original at " + std::to_string(width) + "x" + std::to_string(height));
            }

        settings.setSwitch(qol::Feature::WidenedView, true);
        settings.setMaster(qol::Master::Custom);
        qol::setActiveSettings(settings);
        expectRegion(1920, 1080, {510, 540, 900, 540}, 400, 240, "Custom 1920x1080: 5:3 top screen, bars at the sides", camera);
        expectRegion(3000, 1000, {1083, 500, 834, 500}, 400, 240, "Custom 3000x1000: today's top screen rectangle", camera);

        settings.setMaster(qol::Master::Improved);
        qol::setActiveSettings(settings);
        expectRegion(1920, 1080, {60, 0, 1800, 1080}, 400, 240, "Improved 1920x1080: the 5:3 picture centred, bars at the sides", camera);
    }
}

void cameraKindOrder() {
    require(qol::cameraKindFor(false, false, false) == qol::CameraKind::Gameplay, "no special camera is the gameplay camera");
    require(qol::cameraKindFor(true, false, false) == qol::CameraKind::Cutscene, "cutscene camera");
    require(qol::cameraKindFor(false, true, false) == qol::CameraKind::Director, "director camera");
    require(qol::cameraKindFor(false, false, true) == qol::CameraKind::Scanner, "scanner camera");
    require(qol::cameraKindFor(true, true, true) == qol::CameraKind::Cutscene, "cutscene wins over director and scanner, as in the draw");
    require(qol::cameraKindFor(false, true, true) == qol::CameraKind::Scanner, "scanner wins over director, as in the draw");
}

}

int main() {
    originalModeFirst();
    widenedOn();
    otherCamerasKeepFiveByThree();
    cameraKindOrder();
    if (failures) return 1;
    std::cout << "qol view hook: ok\n";
    return 0;
}
