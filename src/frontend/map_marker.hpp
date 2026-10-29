#pragma once
// Icons on the map screen (docs/re/objective-hud.md, drawing code 0x2db0dc, update 0x2ddd64, placement 0x1ee7a8). The map
// screen is 320x240; an icon sits at the player-relative position turned by the camera yaw, is kept on an ellipse around the
// screen centre when it lies outside it and then gets an arrow that points away from the centre. No SDL dependency.
#include "frontend/map_file.hpp"
#include <array>
#include <map>
#include <vector>

namespace chase::frontend {

constexpr float kMapScreenWidth = 320.0f, kMapScreenHeight = 240.0f;
constexpr float kMapCentreX = kMapScreenWidth * 0.5f, kMapCentreY = kMapScreenHeight * 0.5f;

// 0x1ee7a8 turns a world position into screen units with the level's map file (frontend/map_file.hpp): the normalised map
// position times the half size (struct + 0x354) and the zoom (struct + 0x35c). STAND-IN (HUD-S60): the zoom is fixed at 1.55,
// the value read from the oracle at gameplay-start; the zoom levels are not ported. STAND-IN (HUD-S79): world heights are not
// passed, the placement uses y = 0.
constexpr float kMapZoomStandIn = 1.55f;
// 0x2db0dc: half axes of the edge ellipse, 320 * 0.44 - 36 and 240 * 0.57 - 36 (the factors are read from memory at
// 0x375b34 and 0x375b38; their initialiser was not found).
constexpr float kEdgeHalfWidth = kMapScreenWidth * 0.44f - 36.0f, kEdgeHalfHeight = kMapScreenHeight * 0.57f - 36.0f;
constexpr float kArrowOffset = 4.0f;
constexpr float kOnScreenDistance = 24.0f;
constexpr float kScaleRate = 2.0f, kGrowRate = 2.0f;
constexpr float kRingCycleSeconds = 3.0f, kRingActiveSeconds = 2.0f;
constexpr float kRingLargeEnd = 200.0f, kRingSmallEnd = 20.0f, kRingStart = 2.0f;
constexpr float kRingLineWidth = 2.0f;
constexpr int kRingSegments = 64;

struct MapMarkerPlacement {
    float x = 0, y = 0;
    bool onScreen = false;
    bool outsideEllipse = false;
    float edgeX = 0, edgeY = 0;
    float directionX = 0, directionY = 0;
};

std::array<float, 2> mapScreenOffset(const MapProjection& map, float playerX, float playerZ, float x, float z);
MapMarkerPlacement placeMapMarker(const MapProjection& map, float playerX, float playerZ, float cameraYaw, float markerX, float markerZ);

// 0x33dccc (called through 0x29de40 by the sub-objective update): a world position is "on the map" when its placement lies
// inside the ellipse with half axes 320 * 0.42 - 36 and 240 * 0.41 - 36 (literals 0x33de40, 0x33de44, 0x33de48) around the
// screen centre, the boundary included.
constexpr float kOnMapHalfWidth = kMapScreenWidth * 0.42f - 36.0f, kOnMapHalfHeight = kMapScreenHeight * 0.41f - 36.0f;
bool mapPositionOnMap(const MapProjection& map, float playerX, float playerZ, float cameraYaw, float x, float z);

// Map icon types of the sub-objective groups (table 0x3588a0) and the offscreen arrow of each type (0x2db0dc).
const char* mapIconTexture(int iconType);
const char* mapOffscreenArrowTexture(int iconType);
constexpr float kGroupTargetHalfSize = 8.0f;  // 0x2dbb04: a 16 x 16 quad around each target
float arrowRotation(const MapMarkerPlacement& placement);
int roundedCoordinate(float value);

struct MapMarkerAnimation {
    float scale = 0.5f;
    float grow = 0.0f;
};

struct MapRing {
    float radius = 0, alpha = 0;
};

struct MapMarkerSource {
    int id = 0;
    float x = 0, z = 0;
};

class MapMarkerField {
public:
    void advance(float dt, const MapProjection& map, float playerX, float playerZ, float cameraYaw, const std::vector<MapMarkerSource>& markers);
    const MapMarkerAnimation& animation(int id) const;
    float ringClock() const { return ringClock_; }
    void setRingClock(float seconds) { ringClock_ = seconds; }
    bool ringsActive() const { return ringClock_ < kRingActiveSeconds; }
    MapRing largeRing() const;
    MapRing smallRing() const;

private:
    std::map<int, MapMarkerAnimation> animations_;
    float ringClock_ = 0;
};

}
