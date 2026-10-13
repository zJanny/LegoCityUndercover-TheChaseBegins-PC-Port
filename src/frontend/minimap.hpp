#pragma once
// The bottom screen during gameplay (docs/re/hud.md): the level map turned so that the camera looks up the screen,
// the player's arrow in the middle and the characters button of sprites/blend/mapbuttons at the top left.
// STAND-IN: the map's placement is fitted to oracle screenshots (the .txt next to the map texture is not decoded), the
// original's map drawing code was not read, and no mission or collectible icons are drawn. No SDL dependency.
#include "frontend/canvas.hpp"
#include "frontend/resources.hpp"
#include <string>

namespace chase::frontend {

// Fitted to four oracle screenshots of lvl_01_cherrytreehills (artifacts/review/hud/oracle_map_*): the player stands at
// the screen point (160, 118), one world unit is one screen pixel, and the texture holds kMapTexelsPerUnit texels per unit.
constexpr float kMapPlayerScreenX = 160.0f, kMapPlayerScreenY = 118.0f;
constexpr float kMapTexelsPerUnit = 0.645f;
constexpr float kMapOriginU = 22.03f, kMapOriginV = 977.7f;   // texel of world (0, 0): u = origin + k x, v = origin - k z

struct MinimapView {
    float x = 0, z = 0;          // the player's world position
    float cameraYaw = 0;         // radians, atan2(forward.x, forward.z) of the camera
    float playerYaw = 0;         // radians, the player's heading
};

void drawMinimap(Canvas& bottom, Resources& resources, const std::string& levelName, const MinimapView& view);

}
