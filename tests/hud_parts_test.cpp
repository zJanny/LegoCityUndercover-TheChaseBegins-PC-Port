// The part set of the HUD draw (frontend/hud_parts.hpp) without game data. The element names are the children of the HUD page layout
// sprites/ui_hud/blend/main.bxaml, dumped by 20261029-qolmenu-G on 2026-10-29 from the game's layout file
// (docs/re/hud.md: UI_HUD_3DS holds the portrait images, UI_Hearts the six hearts, Stud_Count and CombatStud_Count the counters).
#include "frontend/hud_parts.hpp"
#include <iostream>
#include <string>

namespace {
using namespace chase::frontend;

int failures = 0;

void require(bool condition, const std::string& what) {
    if (!condition) {
        std::cerr << "FAIL: " << what << '\n';
        ++failures;
    }
}

}

int main() {
    const HudPart parts[] = {HudPart::Portrait, HudPart::Hearts, HudPart::Studs, HudPart::SuperBricks, HudPart::CameraLock};
    require(HudPartSet().empty(), "a default set is empty");
    require(!HudPartSet::all().empty(), "the full set is not empty");
    HudPartSet joined;
    for (const HudPart part : parts) {
        require(HudPartSet::all().contains(part), "the full set holds every part");
        require(!HudPartSet().contains(part), "the empty set holds no part");
        const HudPartSet one(part);
        for (const HudPart other : parts) require(one.contains(other) == (other == part), "a single part set holds only that part");
        joined = joined | one;
    }
    require(joined == HudPartSet::all(), "the union of every single part is the full set");

    require(hudHiddenLayoutElements(HudPartSet::all()).empty(), "the full set hides nothing of the layout");
    const auto withoutHearts = hudHiddenLayoutElements(HudPartSet(HudPart::Portrait) | HudPartSet(HudPart::Studs));
    require(withoutHearts == std::set<std::string>{"UI_Hearts"}, "without hearts only UI_Hearts is hidden");
    const auto onlyHearts = hudHiddenLayoutElements(HudPartSet(HudPart::Hearts));
    require(onlyHearts.count("UI_HUD_3DS") == 1 && onlyHearts.count("Stud_Count") == 1 && onlyHearts.count("UI_Hearts") == 0,
            "hearts alone hide the portrait and the stud counters");
    require(hudHiddenLayoutElements(HudPartSet()).size() ==
                hudPartLayoutElements(HudPart::Portrait).size() + hudPartLayoutElements(HudPart::Hearts).size() +
                    hudPartLayoutElements(HudPart::Studs).size(),
            "the empty set hides every part's layout elements, no element belongs to two parts");
    require(hudPartLayoutElements(HudPart::SuperBricks).empty() && hudPartLayoutElements(HudPart::CameraLock).empty(),
            "the super bricks and the camera lock are not part of the layout");

    if (failures) return 1;
    std::cout << "hud-parts-test: ok\n";
    return 0;
}
