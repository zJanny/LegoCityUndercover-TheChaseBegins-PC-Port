// Spec: docs/design/qol-screen-layout.md section 2026-10-28T09:31:02 (QOLMAP-5: the minimap starts under the studs and counters).
// Measured by 20261030-qolpanels2-B (2026-10-30) with this test's drawing loop on the real data: the lowest opaque row of the Studs and
// SuperBricks parts of Hud::draw in the 400x240 top canvas is 61 (stud bar shown with 99999 studs; super bricks at most 31), over a pickup
// age of 0, 2, 6, 15, 30 and 60 updates at 30 updates per second. Exit code 77 when the game data is missing.
#include "frontend/hud.hpp"
#include "qol/minimap.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>

namespace {
using namespace chase;
using namespace chase::frontend;

int bottomRow(const Canvas& canvas) {
    for (int y = canvas.height() - 1; y >= 0; --y)
        for (int x = 0; x < canvas.width(); ++x)
            if (canvas.pixel(x, y)[3] != 0) return y;
    return -1;
}

int measure(Resources& resources) {
    int worst = -1;
    for (int pickupAge : {0, 2, 6, 15, 30, 60}) {
        Hud hud(resources, {});
        for (int update = 0; update < 90; ++update) hud.update(1.0f / 30);
        hud.studsBanked(0, 99999);
        hud.superBrickPickedUp();
        for (int update = 0; update < pickupAge; ++update) hud.update(1.0f / 30);
        for (const HudPart part : {HudPart::Studs, HudPart::SuperBricks}) {
            Canvas canvas(kTopWidth, kScreenHeight);
            canvas.clearTransparent();
            hud.draw(canvas, 1.0, HudPartSet(part));
            const int row = bottomRow(canvas);
            worst = std::max(worst, row);
        }
    }
    return worst;
}
}

int main(int argc, char** argv) {
    if (argc < 3) return 2;
    if (!std::ifstream(argv[1]).good() || !std::ifstream(argv[2]).good()) return 77;
    assets::FuseArchive archive(argv[1]);
    archive.loadNames(argv[2]);
    Resources resources(archive);
    const int worst = measure(resources);
    if (worst + 1 != qol::countersBottomOriginal) {
        std::cerr << "FAIL: lowest opaque row " << worst << ", countersBottomOriginal " << qol::countersBottomOriginal << '\n';
        return 1;
    }
    std::cout << "qol-minimap-real-data ok\n";
    return 0;
}
