// Spec: docs/design/qol-screen-layout.md §2026-10-30T14:07:44 and docs/design/qol-settings.md §2026-10-29T18:06:35 (the always shown
// PC Settings bar of the slot page keeps working in Improved: its click point is mapped through the centred panel).
// Pinned values computed by hand by 20261030-qolpanels2-A on 2026-10-30: Original 1920x1080 stacks two screens (scale 2.25 for the
// 400x480 stack, bottom screen at x=600,y=540); Improved 1920x1080 centres the 320x240 panel at x=240,y=0 with scale 4.5.
#include "qol/active_settings.hpp"
#include "qol/menu_window_sdl.hpp"

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

void setMaster(qol::Master master) {
    qol::Settings settings;
    settings.setMaster(master);
    qol::setActiveSettings(settings);
}

}

int main() {
    setMaster(qol::Master::Original);
    const qol::PageRect original = qol::frontBottomScreen(1920, 1080);
    require(near(original.x, 600) && near(original.y, 540) && near(original.width, 720) && near(original.height, 540),
        "Original: the bottom screen sits under the top screen");
    const auto stacked = qol::frontBottomPoint(1920, 1080, 960, 810);
    require(stacked && near((*stacked)[0], 160) && near((*stacked)[1], 120), "Original: stacked point mapping unchanged");
    require(!qol::frontBottomPoint(1920, 1080, 960, 300), "Original: the top screen is no bottom point");

    setMaster(qol::Master::Improved);
    const qol::PageRect panel = qol::frontBottomScreen(1920, 1080);
    require(near(panel.x, 240) && near(panel.y, 0) && near(panel.width, 1440) && near(panel.height, 1080), "Improved: the panel is centred");
    const auto centre = qol::frontBottomPoint(1920, 1080, 960, 540);
    require(centre && near((*centre)[0], 160) && near((*centre)[1], 120), "Improved: window centre maps to the canvas centre");
    require(!qol::frontBottomPoint(1920, 1080, 100, 540), "Improved: a point beside the panel maps nowhere");
    const auto bar = qol::frontBottomPoint(1920, 1080, 240 + 4.5f * 130, 4.5f * 205);
    require(bar && near((*bar)[0], 130) && near((*bar)[1], 205), "Improved: a point on the slot page bar maps to canvas pixels");
    if (failures) return 1;
    std::cout << "qol-front-panel-point OK\n";
    return 0;
}
