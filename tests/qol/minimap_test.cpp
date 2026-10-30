// Spec: docs/design/qol-screen-layout.md section 2026-10-28T09:31:02 (QOLMAP-2: round, top-right, about 1/5 of the screen height) and
// section 2026-10-28T09:48:15 (QOLMAP-4: only in Improved, hidden in cutscenes and panels).
// Pinned values hand-computed by 20261030-qolpanels-B (2026-10-30) from those entries: diameter = round(window height / 5), the circle
// touches the window's right edge, the 400x240 canvas is scaled by diameter / 240 and the circle is the 240 px disc around
// the 320x240 map's centre. Placement under the counters (QOLMAP-5, user 2026-10-30 "Below the counters"): hand-computed by
// 20261030-qolpanels2-B (2026-10-30): the circle's top is the corner HUD view's top plus (62 + 4) original units times view height / 240,
// 62 being the lowest row of the studs and counters plus one (qol-minimap-real-data) and 4 the stand-in margin; e.g. 1920x1080: 66 * 4.5 = 297,
// 960x600 (view 960x576 at y 12): 12 + 66 * 2.4 = 170, 800x800 (view 800x480 at y 160): 160 + 66 * 2 = 292. Hidden panels: QOLMAP-6.
#include "qol/minimap.hpp"
#include "qol/settings.hpp"
#include "qol/view_layout.hpp"

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

struct Run {
    int sinks = 0;
    chase::render::PixelRect rect;
    chase::frontend::Canvas round{400, 240};
    bool drawn = false;
};

Run run(const qol::Settings& settings, const qol::MinimapPanels& panels, int width, int height) {
    chase::frontend::Canvas map(320, 240);
    map.clear(200, 10, 10);
    std::uint8_t* centre = map.pixel(160, 120);
    centre[0] = 0, centre[1] = 0, centre[2] = 250;
    Run result;
    result.round.clearTransparent();
    result.drawn = qol::drawRoundMinimapWith(settings, map, result.round, panels, width, height,
                                             [&](const chase::frontend::Canvas&, const chase::render::PixelRect& rect) {
                                                 ++result.sinks;
                                                 result.rect = rect;
                                             });
    return result;
}

void originalModeFirst() {
    for (const qol::Master master : {qol::Master::Original, qol::Master::Custom}) {
        qol::Settings settings;
        settings.setMaster(master);
        for (const auto& [width, height] : {std::pair<int, int>{960, 600}, std::pair<int, int>{1920, 1080}}) {
            const Run result = run(settings, {}, width, height);
            require(!result.drawn && result.sinks == 0, std::string(qol::masterName(master)) + ": nothing drawn");
            bool clear = true;
            for (std::size_t i = 3; i < result.round.rgba().size(); i += 4) clear = clear && result.round.rgba()[i] == 0;
            require(clear, std::string(qol::masterName(master)) + ": canvas untouched");
        }
    }
}

void improved() {
    qol::Settings settings;
    settings.setMaster(qol::Master::Improved);

    const auto big = qol::minimapPlacement(1920, 1080);
    require(big && big->diameter == 216 && same(big->rect, {1632, 567, 360, 216}), "1920x1080: diameter 216, rectangle 1632 567 360x216");
    const auto small = qol::minimapPlacement(960, 600);
    require(small && small->diameter == 120 && same(small->rect, {800, 310, 200, 120}), "960x600: diameter 120, rectangle 800 310 200x120");
    const auto square = qol::minimapPlacement(800, 800);
    require(square && square->diameter == 160 && same(square->rect, {587, 348, 267, 160}), "800x800: diameter 160, rectangle 587 348 267x160");
    const auto ultrawide = qol::minimapPlacement(2560, 1080);
    require(ultrawide && ultrawide->diameter == 216 && same(ultrawide->rect, {2272, 567, 360, 216}), "2560x1080: diameter 216, rectangle 2272 567 360x216");
    for (const auto& [width, height] : {std::pair<int, int>{400, 240}, {960, 600}, {1280, 720}, {1920, 1080}, {800, 800}, {2560, 1080}, {1000, 1500}}) {
        const auto placement = qol::minimapPlacement(width, height);
        const qol::ViewLayout layout = qol::layoutInArea(width, height);
        const double scale = double(layout.view.height) / 240;
        const int circleTop = height - placement->rect.y - placement->diameter;
        const std::string shape = std::to_string(width) + "x" + std::to_string(height);
        require(circleTop >= layout.view.y + 62 * scale, shape + ": the circle starts below the counters");
        require(circleTop - layout.view.y - 62 * scale < 8 * scale, shape + ": the margin is small");
        require(placement->rect.y >= 0, shape + ": the circle stays inside the window");
    }
    require(!qol::minimapPlacement(0, 600) && !qol::minimapPlacement(960, 0), "empty window: no placement");

    const Run result = run(settings, {}, 1920, 1080);
    require(result.drawn && result.sinks == 1 && big && same(result.rect, big->rect), "Improved draws once at the placement");
    const chase::frontend::Canvas& round = result.round;
    require(round.pixel(0, 0)[3] == 0 && round.pixel(399, 239)[3] == 0 && round.pixel(100, 5)[3] == 0, "outside the circle is transparent");
    require(round.pixel(200, 120)[3] == 255 && round.pixel(200, 120)[2] == 250, "centre keeps the map's centre pixel");
    require(round.pixel(81, 120)[3] == 255 && round.pixel(318, 120)[3] == 255, "inside the left and right edge is opaque");
    require(round.pixel(79, 120)[3] == 0 && round.pixel(320, 120)[3] == 0, "beyond the radius is transparent");
    require(round.pixel(200, 0)[3] == 255 && round.pixel(200, 239)[3] == 255, "top and bottom of the disc are opaque");
    require(round.pixel(100, 60)[0] == 200 && round.pixel(100, 60)[3] == 255, "inside the disc the map's colour is kept");
}

void hiding() {
    qol::Settings settings;
    settings.setMaster(qol::Master::Improved);
    const std::pair<const char*, qol::MinimapPanels> panels[] = {
        {"scanner overlay", {true, false, false, false, false}},
        {"dialog box", {false, true, false, false, false}},
        {"speech box", {false, false, true, false, false}},
        {"scene change page", {false, false, false, true, false}},
        {"fast-travel map", {false, false, false, false, true}},
    };
    for (const auto& [name, open] : panels) {
        const Run panel = run(settings, open, 1920, 1080);
        require(!panel.drawn && panel.sinks == 0, std::string(name) + " open: nothing drawn");
    }
    require(!qol::MinimapPanels{}.any(), "no panel open by default");
    const Run none = run(settings, {}, 0, 0);
    require(!none.drawn && none.sinks == 0, "empty window: nothing drawn");
}

}

int main() {
    originalModeFirst();
    improved();
    hiding();
    if (failures) return 1;
    std::cout << "qol-minimap ok\n";
    return 0;
}
