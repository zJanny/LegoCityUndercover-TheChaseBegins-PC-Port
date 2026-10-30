// Spec: docs/design/qol-screen-layout.md §2026-10-28T09:31:02 (QOLMAP-2: round, top-right, about 1/5 of the screen height) and
// §2026-10-28T09:48:15 (QOLMAP-4: only in Improved, hidden in cutscenes and panels).
// Pinned values hand-computed by 20261030-qolpanels-B (2026-10-30) from those entries: diameter = round(window height / 5), the circle
// touches the window's top and right edges, the 400x240 canvas is scaled by diameter / 240 and the circle is the 240 px disc around
// the 320x240 map's centre.
#include "qol/minimap.hpp"
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

struct Run {
    int sinks = 0;
    chase::render::PixelRect rect;
    chase::frontend::Canvas round{400, 240};
    bool drawn = false;
};

Run run(const qol::Settings& settings, bool panelOpen, int width, int height) {
    chase::frontend::Canvas map(320, 240);
    map.clear(200, 10, 10);
    std::uint8_t* centre = map.pixel(160, 120);
    centre[0] = 0, centre[1] = 0, centre[2] = 250;
    Run result;
    result.round.clearTransparent();
    result.drawn = qol::drawRoundMinimapWith(settings, map, result.round, panelOpen, width, height,
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
            const Run result = run(settings, false, width, height);
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
    require(big && big->diameter == 216 && same(big->rect, {1632, 864, 360, 216}), "1920x1080: diameter 216, rectangle 1632 864 360x216");
    const auto small = qol::minimapPlacement(960, 600);
    require(small && small->diameter == 120 && same(small->rect, {800, 480, 200, 120}), "960x600: diameter 120, rectangle 800 480 200x120");
    require(!qol::minimapPlacement(0, 600) && !qol::minimapPlacement(960, 0), "empty window: no placement");

    const Run result = run(settings, false, 1920, 1080);
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
    const Run panel = run(settings, true, 1920, 1080);
    require(!panel.drawn && panel.sinks == 0, "panel open: nothing drawn");
    const Run none = run(settings, false, 0, 0);
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
