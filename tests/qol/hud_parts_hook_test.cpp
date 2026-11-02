// Spec: docs/design/qol-screen-layout.md §2026-10-29T11:42:22 (HUD parts at the corners of the 16:9 area). The part mapping is the
// brief of 20261030-qolstores-A (2026-10-30): Hearts, Portrait, Studs, Counters to SuperBricks, Ability to CameraLock (HUD-94).
// The widened view hides the bottom screen in every preset (user answer 2026-11-02), so the rectangles are over the whole window, recomputed by 20261102-qolmenu-B on 2026-11-02. Rectangles are the ones pinned in hud_hook_test.cpp (hand-computed by 20261029-qolmenu-D, 2026-10-29).
#include "game_hud.hpp"
#include "qol/active_settings.hpp"
#include "qol/hud_parts_hook.hpp"
#include "qol/settings.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace {

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

struct FakeCanvas {
    bool dirty = false;
    void clearTransparent() { dirty = false; }
};

struct FakeHud {
    std::vector<chase::frontend::HudPartSet> calls;
    bool cleanOnDraw = true;
    void draw(FakeCanvas& canvas, double, chase::frontend::HudPartSet parts) {
        cleanOnDraw = cleanOnDraw && !canvas.dirty;
        canvas.dirty = true;
        calls.push_back(parts);
    }
};

bool same(const chase::render::PixelRect& a, const chase::render::PixelRect& b) {
    return a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height;
}

void originalModeFirst() {
    for (int width = 400; width <= 2600; width += 211)
        for (int height = 240; height <= 1500; height += 173)
            require(qol::hudPartDraws(width, game_hud::screensFor(width, height).top, qol::CameraKind::Gameplay).empty(),
                    "off: no part draw at " + std::to_string(width) + "x" + std::to_string(height));
}

void widenedOn() {
    qol::Settings settings;
    settings.setSwitch(qol::Feature::WidenedView, true);
    settings.setMaster(qol::Master::Custom);
    qol::setActiveSettings(settings);

    const auto draws = qol::hudPartDraws(1920, game_hud::screensFor(1920, 1080).top, qol::CameraKind::Gameplay);
    using chase::frontend::HudPart;
    require(draws.size() == 5, "five parts");
    if (draws.size() != 5) return;
    const HudPart order[] = {HudPart::Hearts, HudPart::Portrait, HudPart::Studs, HudPart::SuperBricks, HudPart::CameraLock};
    const chase::render::PixelRect rects[] = {{0, 0, 1800, 1080}, {0, 0, 1800, 1080}, {120, 0, 1800, 1080}, {120, 0, 1800, 1080},
                                              {0, 0, 1800, 1080}};
    for (std::size_t i = 0; i < 5; ++i) {
        require(draws[i].frontendPart == order[i], "part mapping " + std::to_string(i));
        require(same(draws[i].rect, rects[i]), "rectangle " + std::to_string(i));
    }

    FakeHud hud;
    FakeCanvas canvas;
    std::vector<chase::render::PixelRect> sunk;
    qol::drawHudPartsWith(hud, canvas, 0.0, draws,
                          [&](const FakeCanvas&, const chase::render::PixelRect& rect) { sunk.push_back(rect); });
    require(hud.calls.size() == 5 && sunk.size() == 5, "one draw and one placement per part");
    require(hud.cleanOnDraw, "canvas cleared before each part");
    require(!canvas.dirty, "canvas left clear");
    for (std::size_t i = 0; i < hud.calls.size() && i < 5; ++i)
        require(hud.calls[i] == chase::frontend::HudPartSet(order[i]) && same(sunk[i], rects[i]),
                "draw " + std::to_string(i) + " is its own part at its rectangle");

    for (const qol::CameraKind camera : {qol::CameraKind::Cutscene, qol::CameraKind::Director, qol::CameraKind::Scanner}) {
        const chase::render::PixelRect top = game_hud::screensFor(1920, 1080).top;
        const auto picture = qol::hudPartDraws(1920, top, camera);
        require(picture.size() == 5, "5:3 camera: five parts");
        for (const auto& draw : picture) require(same(draw.rect, top), "5:3 camera: each part at the picture's rectangle");
    }

    qol::setActiveSettings(qol::Settings{});
    require(qol::hudPartDraws(1920, game_hud::screensFor(1920, 1080).top, qol::CameraKind::Gameplay).empty(), "off again: no part draw");
}

}

int main() {
    originalModeFirst();
    widenedOn();
    if (failures) return 1;
    std::cout << "qol hud parts hook: ok\n";
    return 0;
}
