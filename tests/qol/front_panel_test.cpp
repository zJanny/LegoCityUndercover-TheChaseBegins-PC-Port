// Spec: docs/design/qol-screen-layout.md §2026-10-30T14:07:44 (QOLBOT-19: with Improved the top picture fills the window, darkened,
// the 320x240 bottom page is a panel centred over it, scaled up as far as it fits keeping 4:3; a click on a list row selects it and
// presses A, a click on a button picture A, B or Y presses that button, a click elsewhere does nothing).
// Pinned rectangles computed by hand by 20261030-qolpanels2-A on 2026-10-30 from the rule above: top picture 400x240 fitted and
// centred (the QOLBOT-1 rule), panel 320x240 fitted and centred in the window (the settings page rule). The page rule cases moved here
// unchanged from the old settings page hit test (20261029-qolmenu2-A) when 20261101-qolui3-F removed the old menu drawing on 2026-11-01.
#include "qol/front_panel.hpp"

#include <cmath>
#include <iostream>
#include <string>

namespace {

int failures = 0;

void require(bool ok, const std::string& what) {
    if (ok) return;
    std::cerr << "FAIL: " << what << '\n';
    ++failures;
}

bool near(float a, float b) { return std::fabs(a - b) < 0.01f; }

bool box(const qol::Box& b, float x, float y, float w, float h) { return near(b.x, x) && near(b.y, y) && near(b.width, w) && near(b.height, h); }

bool panel(const qol::PageRect& r, float x, float y, float w, float h) { return near(r.x, x) && near(r.y, y) && near(r.width, w) && near(r.height, h); }

qol::Settings withMaster(qol::Master master) {
    qol::Settings settings;
    settings.setMaster(master);
    return settings;
}

void originalAndCustomShowNothing() {
    for (qol::Master master : {qol::Master::Original, qol::Master::Custom})
        require(!qol::frontPanelsFor(withMaster(master), 1920, 1080, 400, 240), "no panels while the bottom screen is shown");
    require(!qol::frontPanelsFor(withMaster(qol::Master::Improved), 0, 1080, 400, 240), "an empty window has no panels");
}

void rectangles() {
    const qol::Settings improved = withMaster(qol::Master::Improved);
    auto p = qol::frontPanelsFor(improved, 1920, 1080, 400, 240);
    require(p && box(p->top, 60, 0, 1800, 1080) && panel(p->panel, 240, 0, 1440, 1080), "1920x1080: picture 1800x1080, panel 1440x1080");
    p = qol::frontPanelsFor(improved, 2560, 1080, 400, 240);
    require(p && box(p->top, 380, 0, 1800, 1080) && panel(p->panel, 560, 0, 1440, 1080), "wider than 4:3: both centred, bars at the sides");
    p = qol::frontPanelsFor(improved, 800, 1000, 400, 240);
    require(p && box(p->top, 0, 260, 800, 480) && panel(p->panel, 0, 200, 800, 600), "narrower than 4:3: both fitted to the width");
    p = qol::frontPanelsFor(improved, 400, 240, 400, 240);
    require(p && box(p->top, 0, 0, 400, 240) && panel(p->panel, 40, 0, 320, 240), "400x240: panel at native size");
}

void clickMapping() {
    const auto p = qol::frontPanelsFor(withMaster(qol::Master::Improved), 1920, 1080, 400, 240);
    const auto centre = qol::pagePoint(p->panel, 960, 540);
    require(centre && near((*centre)[0], 160) && near((*centre)[1], 120), "window centre is bottom-canvas (160,120)");
    const auto corner = qol::pagePoint(p->panel, 240, 0);
    require(corner && near((*corner)[0], 0) && near((*corner)[1], 0), "panel top-left is (0,0)");
    require(!qol::pagePoint(p->panel, 100, 540), "left of the panel is outside");
    require(!qol::pagePoint(p->panel, 1700, 540), "right of the panel is outside");
    require(!qol::pagePoint(p->panel, 960, 1080), "below the panel is outside");
    const auto narrow = qol::frontPanelsFor(withMaster(qol::Master::Improved), 800, 1000, 400, 240);
    require(!qol::pagePoint(narrow->panel, 400, 100), "above the panel in a tall window is outside");
    const auto mapped = qol::pagePoint(narrow->panel, 400, 200 + 2.5f * 100);
    require(mapped && near((*mapped)[0], 160) && near((*mapped)[1], 100), "tall window maps with its own scale");
}

void targets() {
    std::vector<qol::ClickTarget> targets;
    targets.push_back({{20, 40, 280, 30}, true, 0, qol::FrontButton::None});
    targets.push_back({{20, 80, 280, 30}, true, 1, qol::FrontButton::None});
    targets.push_back({{240, 200, 30, 30}, false, 0, qol::FrontButton::A});
    targets.push_back({{200, 200, 30, 30}, false, 0, qol::FrontButton::B});
    targets.push_back({{160, 200, 30, 30}, false, 0, qol::FrontButton::Y});
    const qol::ClickAction row = qol::classifyClick(targets, 100, 90);
    require(row.selectRow && *row.selectRow == 1 && row.press == qol::FrontButton::A, "a row click selects the row and presses A");
    const qol::ClickAction b = qol::classifyClick(targets, 210, 210);
    require(!b.selectRow && b.press == qol::FrontButton::B, "a B picture presses B");
    require(qol::classifyClick(targets, 170, 210).press == qol::FrontButton::Y, "a Y picture presses Y");
    require(qol::classifyClick(targets, 250, 210).press == qol::FrontButton::A, "an A picture presses A");
    require(!qol::classifyClick(targets, 5, 5).any(), "a click elsewhere does nothing");
    require(!qol::classifyClick({}, 100, 90).any(), "no targets, no action");
}

void dimming() {
    std::uint8_t pixels[8] = {200, 100, 50, 255, 10, 20, 30, 7};
    qol::darken(pixels, 2, 0.5f);
    require(pixels[0] == 100 && pixels[1] == 50 && pixels[2] == 25 && pixels[3] == 255, "colour halves, alpha kept");
    require(pixels[4] == 5 && pixels[5] == 10 && pixels[6] == 15 && pixels[7] == 7, "second pixel halves, alpha kept");
    qol::darken(pixels, 2, 2.0f);
    require(pixels[0] == 100, "a factor above 1 does not brighten");
}

}

void pageRule() {
    const qol::PageRect small = qol::menuPageRect(400, 240);
    require(near(small.scale, 1) && near(small.x, 40) && near(small.y, 0) && near(small.width, 320) && near(small.height, 240),
        "400x240: s = 1 with 40-pixel bars left and right");
    const qol::PageRect full = qol::menuPageRect(1920, 1080);
    require(near(full.scale, 4.5f) && near(full.width, 1440) && near(full.height, 1080) && near(full.x, 240) && near(full.y, 0),
        "1920x1080: s = 4.5, a 1440x1080 page and 240-pixel bars");
    const qol::PageRect wide = qol::menuPageRect(3440, 1440);
    require(near(wide.scale, 6) && near(wide.width, 1920) && near(wide.height, 1440) && near(wide.x, 760), "3440x1440: s = 6, a 1920x1440 page");
    const qol::PageRect tall = qol::menuPageRect(640, 960);
    require(near(tall.scale, 2) && near(tall.x, 0) && near(tall.y, 240), "a tall window gets bars above and below");
    const auto inside = qol::pagePoint(full, 240 + 4.5f * 159, 4.5f * 119);
    require(inside && near((*inside)[0], 159) && near((*inside)[1], 119), "window pixel to page pixel");
    require(!qol::pagePoint(full, 100, 500), "a pixel in the black bar is outside the page");
    require(!qol::pagePoint(full, 240 + 1440, 10), "the right edge is outside the page");
    require(qol::contains(qol::Box{10, 10, 5, 5}, 10, 14.9f) && !qol::contains(qol::Box{10, 10, 5, 5}, 15, 12) &&
            !qol::contains(qol::Box{10, 10, 0, 5}, 10, 12),
        "a box holds its left and top edge, not its right edge, and an empty box holds nothing");
}

int main() {
    pageRule();
    originalAndCustomShowNothing();
    rectangles();
    clickMapping();
    targets();
    dimming();
    if (failures) return 1;
    std::cout << "qol-front-panel OK\n";
    return 0;
}
