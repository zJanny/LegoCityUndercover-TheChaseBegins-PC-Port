#pragma once
// The map buttons layout (runtime/map_buttons.hpp) on the bottom screen: its setup and hit boxes from sprites/blend/mapbuttons/mapbuttons.bxaml,
// and the layout scene that draws the button instances with their storyboards. No SDL dependency.
#include "frontend/layout.hpp"
#include "runtime/map_buttons.hpp"

namespace chase::frontend {

runtime::MapButtonsSetup mapButtonsSetup(Resources& resources);
// The boxes of the hit test (node +0xf0 and the layout size) under the storyboards started so far.
std::array<runtime::MapButtonBox, runtime::kMapButtonCount> mapButtonBoxes(Resources& resources, const runtime::MapButtons& buttons);
// STAND-IN UI-S9: the portraits of the characters panel (portrait.bxaml, 0x33a01c) and the area name box `textWindow` of the fast travel map
// (0x20a828) are not drawn; the panel shows only its dark background.
LayoutScene mapButtonsScene(const runtime::MapButtons& buttons);

}
