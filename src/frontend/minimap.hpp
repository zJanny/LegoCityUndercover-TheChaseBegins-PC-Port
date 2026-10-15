#pragma once
// The bottom screen during gameplay (docs/re/hud.md): the level map turned so that the camera looks up the screen,
// the player's arrow in the middle and the characters button of sprites/blend/mapbuttons at the top left.
// STAND-IN (HUD-S60): the map matrix is the memory value of lvl_01 (the .txt next to the map texture is not read), and
// only waypoint icons are drawn. No SDL dependency.
#include "frontend/canvas.hpp"
#include "frontend/map_marker.hpp"
#include "frontend/resources.hpp"
#include <string>
#include <vector>

namespace chase::frontend {

constexpr float kMapPlayerScreenX = kMapCentreX, kMapPlayerScreenY = kMapCentreY;

// Waypoint icons on the map (docs/re/objective-hud.md): placement, edge ellipse, arrow, halo, grow and ring come from the
// original's drawing code in frontend/map_marker.hpp. STAND-INS (HUD-S60): every waypoint is drawn as icon type 0 (the type
// is a class parameter that was read for one waypoint only) and the ring lines are white.
using MinimapMarker = MapMarkerSource;

struct MinimapView {
    float x = 0, z = 0;          // the player's world position
    float cameraYaw = 0;         // radians, atan2(forward.x, forward.z) of the camera
    float playerYaw = 0;         // radians, the player's heading
    std::vector<MinimapMarker> markers;
    const MapMarkerField* markerField = nullptr;
};

void drawMinimap(Canvas& bottom, Resources& resources, const std::string& levelName, const MinimapView& view);

}
