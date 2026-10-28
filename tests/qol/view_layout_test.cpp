// Spec: docs/design/qol-screen-layout.md entries 2026-10-28T09:21:33, 09:31:02, 09:37:31.
// Pinned values computed by hand from the spec rules by 20261028-qolmenu-B on 2026-10-28.
// Minimum window 400x240 and the three choices of this file: user answer to QOLFS-9, 2026-10-28 (docs/design/qol-screen-layout.md).
// Integer rounding is floor and the odd bar pixel goes to the right or bottom bar (accepted by the user, QOLFS-9).
#include "qol/view_layout.hpp"

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

void expectLayout(int w, int h, qol::Rect view, std::size_t bars, qol::Rect bar0, qol::Rect bar1, const std::string& name) {
    const qol::ViewLayout layout = qol::computeViewLayout(w, h);
    require(layout.view == view, name + " view");
    require(layout.barCount == bars, name + " bar count");
    if (bars == 2) {
        require(layout.bars[0] == bar0, name + " bar 0");
        require(layout.bars[1] == bar1, name + " bar 1");
    }
    require(layout.aspectWidth == view.width && layout.aspectHeight == view.height, name + " aspect");
}

bool rejects(int w, int h) {
    try {
        qol::computeViewLayout(w, h);
    } catch (const std::invalid_argument& error) {
        return std::string(error.what()).find("view layout") != std::string::npos;
    }
    return false;
}

bool near(double a, double b) { return a - b < 1e-9 && b - a < 1e-9; }

}

int main() {
    expectLayout(400, 240, {0, 0, 400, 240}, 0, {}, {}, "original size");
    expectLayout(1000, 600, {0, 0, 1000, 600}, 0, {}, {}, "5:3");
    expectLayout(1920, 1080, {0, 0, 1920, 1080}, 0, {}, {}, "16:9");
    expectLayout(1700, 1000, {0, 0, 1700, 1000}, 0, {}, {}, "between 5:3 and 16:9");
    expectLayout(1920, 1200, {0, 24, 1920, 1152}, 2, {0, 0, 1920, 24}, {0, 1176, 1920, 24}, "16:10 is narrower than 5:3");
    expectLayout(800, 600, {0, 60, 800, 480}, 2, {0, 0, 800, 60}, {0, 540, 800, 60}, "4:3");
    expectLayout(600, 800, {0, 220, 600, 360}, 2, {0, 0, 600, 220}, {0, 580, 600, 220}, "portrait");
    expectLayout(2560, 1080, {320, 0, 1920, 1080}, 2, {0, 0, 320, 1080}, {2240, 0, 320, 1080}, "21:9");
    expectLayout(1921, 1080, {0, 0, 1920, 1080}, 2, {0, 0, 0, 1080}, {1920, 0, 1, 1080}, "odd pixel beyond 16:9");
    expectLayout(400, 400, {0, 80, 400, 240}, 2, {0, 0, 400, 80}, {0, 320, 400, 80}, "square at the minimum width");
    expectLayout(1920, 240, {747, 0, 426, 240}, 2, {0, 0, 747, 240}, {1173, 0, 747, 240}, "wide at the minimum height");

    require(rejects(0, 600), "zero width");
    require(rejects(800, 0), "zero height");
    require(rejects(-1, 600), "negative width");
    require(rejects(800, -5), "negative height");
    require(rejects(1, 1), "1x1");
    require(rejects(3, 1), "3x1");
    require(rejects(399, 240), "one pixel narrower than the minimum");
    require(rejects(400, 239), "one pixel lower than the minimum");

    {
        const qol::ViewLayout layout = qol::computeViewLayout(400, 240);
        const qol::HudAnchor topLeft = qol::hudAnchor(layout, qol::Corner::TopLeft);
        require(near(topLeft.scale, 1) && near(topLeft.x, 0) && near(topLeft.y, 0), "original size top-left anchor");
        const qol::HudPoint point = qol::hudPosition(qol::hudAnchor(layout, qol::Corner::TopRight), 380, 10);
        require(near(point.x, 380) && near(point.y, 10), "original size keeps original positions");
    }
    {
        const qol::ViewLayout layout = qol::computeViewLayout(1920, 1080);
        const qol::HudAnchor topLeft = qol::hudAnchor(layout, qol::Corner::TopLeft);
        const qol::HudAnchor topRight = qol::hudAnchor(layout, qol::Corner::TopRight);
        const qol::HudAnchor bottomLeft = qol::hudAnchor(layout, qol::Corner::BottomLeft);
        require(near(topLeft.scale, 4.5) && near(topLeft.x, 0) && near(topLeft.y, 0), "16:9 top-left anchor");
        require(near(topRight.x, 1920) && near(topRight.y, 0), "16:9 top-right anchor");
        require(near(bottomLeft.x, 0) && near(bottomLeft.y, 1080), "16:9 bottom-left anchor");
        const qol::HudPoint tr = qol::hudPosition(topRight, 380, 10);
        require(near(tr.x, 1830) && near(tr.y, 45), "16:9 top-right element");
        const qol::HudPoint bl = qol::hudPosition(bottomLeft, 10, 220);
        require(near(bl.x, 45) && near(bl.y, 990), "16:9 bottom-left element");
        const qol::HudPoint tl = qol::hudPosition(topLeft, 8, 4);
        require(near(tl.x, 36) && near(tl.y, 18), "16:9 top-left element");
    }
    {
        const qol::ViewLayout layout = qol::computeViewLayout(2560, 1080);
        require(near(qol::hudAnchor(layout, qol::Corner::TopLeft).x, 320), "21:9 top-left anchor inside the bar");
        require(near(qol::hudAnchor(layout, qol::Corner::TopRight).x, 2240), "21:9 top-right anchor inside the bar");
        require(near(qol::hudAnchor(layout, qol::Corner::TopRight).scale, 4.5), "21:9 scale from the view height");
    }
    {
        const qol::ViewLayout layout = qol::computeViewLayout(600, 800);
        const qol::HudAnchor topLeft = qol::hudAnchor(layout, qol::Corner::TopLeft);
        const qol::HudAnchor bottomLeft = qol::hudAnchor(layout, qol::Corner::BottomLeft);
        require(near(topLeft.scale, 1.5) && near(topLeft.y, 220), "portrait top-left anchor below the bar");
        require(near(bottomLeft.y, 580), "portrait bottom-left anchor above the bar");
    }

    if (failures) return 1;
    std::cout << "qol view layout: ok\n";
    return 0;
}
