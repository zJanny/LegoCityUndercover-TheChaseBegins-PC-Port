#pragma once
// The bottom screen during gameplay (docs/re/hud.md): the level map turned so that the camera looks up the screen,
// the player's arrow in the middle and the characters button of sprites/blend/mapbuttons at the top left.
// STAND-IN (HUD-S60): the map matrix is the memory value of lvl_01 (the .txt next to the map texture is not read), and
// only waypoint and sub-objective icons are drawn. No SDL dependency.
#include "frontend/canvas.hpp"
#include "frontend/layout.hpp"
#include "frontend/map_marker.hpp"
#include "frontend/resources.hpp"
#include <array>
#include <string>
#include <vector>

namespace chase::frontend {

constexpr float kMapPlayerScreenX = kMapCentreX, kMapPlayerScreenY = kMapCentreY;

// Waypoint icons on the map (docs/re/objective-hud.md): placement, edge ellipse, arrow, halo, grow and ring come from the
// original's drawing code in frontend/map_marker.hpp. Every waypoint is icon type 0: WaypointType is 0 in the three class files
// (docs/re/objective-hud.md). STAND-IN (HUD-S60): the ring lines are white.
using MinimapMarker = MapMarkerSource;

// A sub-objective record on the map (0x2dbb04): the icon of every enabled target at alpha (1 - slot fade); and its slot (0x2db0dc,
// icon type 14 / 13 / 15 at the centroid, alpha = the slot timer), kept on the edge ellipse with an offscreen arrow.
struct MinimapGroupTargets {
    int iconType = 14;
    float alpha = 1;
    std::vector<std::array<float, 2>> targets;  // world x, z
};
struct MinimapGroupSlot {
    int iconType = 14;
    float x = 0, z = 0;
    float alpha = 0;
};

struct MinimapView {
    float x = 0, z = 0;          // the player's world position
    float cameraYaw = 0;         // radians, atan2(forward.x, forward.z) of the camera
    float playerYaw = 0;         // radians, the player's heading
    std::vector<MinimapMarker> markers;
    const MapMarkerField* markerField = nullptr;
    std::vector<MinimapGroupTargets> groupTargets;
    std::vector<MinimapGroupSlot> groupSlots;
    // The scanner view (docs/re/abilities-scanner.md): the characters button gives way to the audio (greyed) and exit buttons.
    // STAND-IN (SCAN-S8): the buttons stand at their final place without the slide-in storyboards, the audio button uses the
    // "disabled" look read from the oracle picture, and the player arrow and the map circle of the scanner screen are not drawn.
    bool scanner = false;
    // The map buttons (runtime/map_buttons.hpp) as frontend/map_buttons_view.hpp lays them out; without it the characters button alone.
    const LayoutScene* buttons = nullptr;
};

void drawMinimap(Canvas& bottom, Resources& resources, const std::string& levelName, const MinimapView& view);

}
