#pragma once
// The bottom screen during gameplay (docs/re/hud.md): the level map turned so that the camera looks up the screen,
// the player's arrow in the middle and the characters button of sprites/blend/mapbuttons at the top left.
// STAND-IN: the map's placement is fitted to oracle screenshots (the .txt next to the map texture is not decoded), the
// original's map drawing code was not read, and no mission or collectible icons are drawn. No SDL dependency.
#include "frontend/canvas.hpp"
#include "frontend/resources.hpp"
#include <string>
#include <vector>

namespace chase::frontend {

// Fitted to four oracle screenshots of lvl_01_cherrytreehills (artifacts/review/hud/oracle_map_*): the player stands at
// the screen point (160, 118), one world unit is one screen pixel, and the texture holds kMapTexelsPerUnit texels per unit.
constexpr float kMapPlayerScreenX = 160.0f, kMapPlayerScreenY = 118.0f;
constexpr float kMapTexelsPerUnit = 0.645f;
constexpr float kMapOriginU = 22.03f, kMapOriginV = 977.7f;   // texel of world (0, 0): u = origin + k x, v = origin - k z

// Active waypoint on the map (docs/re/objective-hud.md): a blue disc with the mission badge and a pulse ring. STAND-INS
// (HUD-S10): the marker is placed with kMarkerScale of the map's unit (fitted to three oracle screenshots), its sizes
// are fitted, the ring curve is a guess from stepped screenshots, and a marker outside the screen is not clamped.
constexpr float kMarkerScale = 0.92f;
constexpr float kMarkerDiscSize = 20.0f, kMarkerBadgeSize = 11.0f;
constexpr float kPulseStartRadius = 12.0f, kPulseEndRadius = 90.0f;
struct MinimapMarker {
    float x = 0, z = 0;          // world position of the waypoint object
};

struct MinimapView {
    float x = 0, z = 0;          // the player's world position
    float cameraYaw = 0;         // radians, atan2(forward.x, forward.z) of the camera
    float playerYaw = 0;         // radians, the player's heading
    std::vector<MinimapMarker> markers;
    float pulsePhase = 0;        // 0..1 of the ring's cycle
};

void drawMinimap(Canvas& bottom, Resources& resources, const std::string& levelName, const MinimapView& view);

}
