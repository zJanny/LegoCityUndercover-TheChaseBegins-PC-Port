#pragma once
// The map centre and angle of the bottom screen (docs/re/objective-hud-map.md): the picture quad 0x2de450 and the angle 0x1ee848
// take the player's position and the camera angle, except for level record 1 (the hub), where they use the literals 0x2de82c /
// 0x2de830 (x 902.41, z 1117.0) and 0x1ee91c (-0.80285). The port's camera yaw has the opposite sign of the original's angle
// (measured at the hub, docs/re/objective-hud-map.md), so the literal enters negated. No SDL dependency.
#include "frontend/map_file.hpp"

namespace chase::frontend {

constexpr std::uint32_t kHubMapCentreXWord = 0x44619a3d, kHubMapCentreZWord = 0x448ba000, kHubMapAngleWord = 0xbf4d87ac;

struct MapFrame {
    float x = 0, z = 0;
    float yaw = 0;
};

float mapAngle(int levelRecord, float cameraYaw);
MapFrame mapFrame(int levelRecord, float playerX, float playerZ, float cameraYaw);

}
