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

constexpr qol::HudPart allParts[] = {qol::HudPart::Hearts, qol::HudPart::Portrait, qol::HudPart::Studs, qol::HudPart::Counters,
                                     qol::HudPart::Ability};

void expectRect(qol::HudPart part, int width, int height, chase::render::PixelRect rect, const std::string& name) {
    const chase::render::PixelRect got = qol::hudPartRect(part, width, game_hud::screensFor(width, height).top);
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
                if (!same(qol::hudPartRect(part, width, top), top))
                    require(false, "off: part rectangle equals today's top rectangle at " + std::to_string(width) + "x" + std::to_string(height));
        }
}

void widenedOn() {
    qol::Settings settings;
    settings.setMaster(qol::Master::Improved);
    qol::setActiveSettings(settings);

    for (qol::HudPart part : {qol::HudPart::Hearts, qol::HudPart::Portrait, qol::HudPart::Ability})
        expectRect(part, 1920, 1080, {480, 540, 900, 540}, "1920x1080 left parts at the view's left edge");
    for (qol::HudPart part : {qol::HudPart::Studs, qol::HudPart::Counters})
        expectRect(part, 1920, 1080, {540, 540, 900, 540}, "1920x1080 right parts end at the view's right edge");

    expectRect(qol::HudPart::Hearts, 1280, 720, {320, 360, 600, 360}, "1280x720 top-left");
    expectRect(qol::HudPart::Studs, 1280, 720, {360, 360, 600, 360}, "1280x720 top-right");

    expectRect(qol::HudPart::Hearts, 2520, 1080, {780, 540, 900, 540}, "21:9 window: left anchor inside the bar");
    expectRect(qol::HudPart::Studs, 2520, 1080, {840, 540, 900, 540}, "21:9 window: right anchor inside the bar");

    expectRect(qol::HudPart::Hearts, 800, 1200, {0, 600, 800, 480}, "800x1200 tall: 5:3 area, original rectangle");
    expectRect(qol::HudPart::Studs, 800, 1200, {0, 600, 800, 480}, "800x1200 tall: right edge equals the original");

    const chase::render::PixelRect empty{0, 0, 0, 0};
    require(same(qol::hudPartRect(qol::HudPart::Hearts, 0, empty), empty), "empty drawable while on: unchanged, no exception");

    qol::setActiveSettings(qol::Settings{});
    expectRect(qol::HudPart::Studs, 1920, 1080, {510, 540, 900, 540}, "off again: today's top rectangle");
}

}

int main() {
    originalModeFirst();
    widenedOn();
    if (failures) return 1;
    std::cout << "qol hud hook: ok\n";
    return 0;
}
