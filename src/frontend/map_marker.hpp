#pragma once
// Icons on the map screen (docs/re/objective-hud.md, drawing code 0x2db0dc, update 0x2ddd64, placement 0x1ee7a8). The map
// screen is 320x240; an icon sits at the player-relative position turned by the camera yaw, is kept on an ellipse around the
// screen centre when it lies outside it and then gets an arrow that points away from the centre. No SDL dependency.
#include <map>
#include <vector>

namespace chase::frontend {

constexpr float kMapScreenWidth = 320.0f, kMapScreenHeight = 240.0f;
constexpr float kMapCentreX = kMapScreenWidth * 0.5f, kMapCentreY = kMapScreenHeight * 0.5f;

// 0x1ee7a8 turns a world position into screen units with the map matrix at slot struct + 0x308: 512 (half of the 1024
// texels, struct + 0x354) * the zoom 1.55 (struct + 0x35c, read from the oracle at gameplay-start) * 1.6007755 (matrix
// scale) / 1337.4733 (matrix w). Both numbers are the values of maps/lvl_01_cherrytreehills.txt as memory shows them.
constexpr float kMapPixelsPerUnit = 512.0f * 1.55f * 1.6007755f / 1337.4733f;
// The same matrix gives the map texture's texel of a world position: u = 512 + 512 * (1.6007755 x - 1202.8282) / 1337.4733
// and v = 512 - 512 * (1.6007755 z - 1121.6160) / 1337.4733 (offsets from the same memory block, struct + 0x338).
constexpr float kMapTexelsPerUnit = 512.0f * 1.6007755f / 1337.4733f;
constexpr float kMapOriginU = 512.0f - 512.0f * 1202.8282f / 1337.4733f;
constexpr float kMapOriginV = 512.0f + 512.0f * 1121.6160f / 1337.4733f;
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

MapMarkerPlacement placeMapMarker(float playerX, float playerZ, float cameraYaw, float markerX, float markerZ);
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
    void advance(float dt, float playerX, float playerZ, float cameraYaw, const std::vector<MapMarkerSource>& markers);
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
