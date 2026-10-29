#include "frontend/map_marker.hpp"
#include "frontend/map_hub.hpp"
#include <algorithm>
#include <cmath>

namespace chase::frontend {

namespace {
constexpr float kPi = 3.14159265358979f;

float lerp(float from, float to, float t) { return from + (to - from) * t; }
float ringFade(float clock) { return 1.0f - clock / kRingActiveSeconds; }
}

int roundedCoordinate(float value) { return int(value + 0.5f); }

std::array<float, 2> mapScreenOffset(const MapProjection& map, float playerX, float playerZ, float x, float z) {
    const std::array<float, 2> player = mapNormalised(map.file, playerX, 0.0f, playerZ), target = mapNormalised(map.file, x, 0.0f, z);
    const float scaleX = map.file.halfWidth * map.zoom, scaleY = map.file.halfHeight * map.zoom;
    return {scaleX * target[0] - scaleX * player[0], scaleY * target[1] - scaleY * player[1]};
}

MapMarkerPlacement placeMapMarker(const MapProjection& map, float playerX, float playerZ, float cameraYaw, float markerX, float markerZ) {
    MapMarkerPlacement placement;
    const float mapTurn = mapAngle(map.levelRecord, cameraYaw);
    const float c = std::cos(mapTurn), s = std::sin(mapTurn);
    const std::array<float, 2> offset = mapScreenOffset(map, playerX, playerZ, markerX, markerZ);
    const float dx = offset[0], dz = offset[1];
    placement.x = kMapCentreX + (c * dx - s * dz);
    placement.y = kMapCentreY + (-s * dx - c * dz);
    placement.onScreen = placement.x > 0.0f && placement.y > 0.0f && !(kMapScreenWidth <= placement.x) && !(kMapScreenHeight <= placement.y);
    const float ex = placement.x - kMapCentreX, ey = placement.y - kMapCentreY;
    placement.outsideEllipse = ex * ex / (kEdgeHalfWidth * kEdgeHalfWidth) + ey * ey / (kEdgeHalfHeight * kEdgeHalfHeight) > 1.0f;
    const float angle = std::atan2(ey * (kEdgeHalfWidth / kEdgeHalfHeight), ex);
    placement.directionX = std::cos(angle);
    placement.directionY = std::sin(angle);
    placement.edgeX = kMapCentreX + kEdgeHalfWidth * placement.directionX;
    placement.edgeY = kMapCentreY + kEdgeHalfHeight * placement.directionY;
    return placement;
}

bool mapPositionOnMap(const MapProjection& map, float playerX, float playerZ, float cameraYaw, float x, float z) {
    const MapMarkerPlacement placement = placeMapMarker(map, playerX, playerZ, cameraYaw, x, z);
    const float ex = placement.x - kMapCentreX, ey = placement.y - kMapCentreY;
    return ex * ex / (kOnMapHalfWidth * kOnMapHalfWidth) + ey * ey / (kOnMapHalfHeight * kOnMapHalfHeight) <= 1.0f;
}

const char* mapIconTexture(int iconType) {
    switch (iconType) {
    case 0: return "sprites/ui_bottomscreen/maps/waypoint.btga";
    case 13: return "sprites/ui_bottomscreen/maps/subobjectiveenemy.btga";
    case 14: return "sprites/ui_bottomscreen/maps/subobjectivepoint.btga";
    case 15: return "sprites/ui_bottomscreen/maps/subobjectivepointgreen.btga";
    default: return nullptr;
    }
}

const char* mapOffscreenArrowTexture(int iconType) {
    switch (iconType) {
    case 0: return "sprites/ui_bottomscreen/maps/waypointoffscreen.btga";
    case 13: return "sprites/ui_bottomscreen/maps/waypointoffscreenred.btga";
    case 14: return "sprites/ui_bottomscreen/maps/waypointoffscreenblue.btga";
    case 15: return "sprites/ui_bottomscreen/maps/waypointoffscreengreen.btga";
    default: return "sprites/ui_bottomscreen/maps/waypointoffscreenwhite.btga";
    }
}

float arrowRotation(const MapMarkerPlacement& placement) { return kPi - std::atan2(placement.directionX, placement.directionY); }

void MapMarkerField::advance(float dt, const MapProjection& map, float playerX, float playerZ, float cameraYaw, const std::vector<MapMarkerSource>& markers) {
    ringClock_ += dt;
    if (ringClock_ > kRingCycleSeconds) ringClock_ = 0.0f;
    std::map<int, MapMarkerAnimation> next;
    for (const MapMarkerSource& marker : markers) {
        MapMarkerAnimation animation;
        const auto known = animations_.find(marker.id);
        if (known != animations_.end()) animation = known->second;
        const MapMarkerPlacement placement = placeMapMarker(map, playerX, playerZ, cameraYaw, marker.x, marker.z);
        animation.scale += placement.onScreen ? -kScaleRate * dt : kScaleRate * dt;
        if (animation.scale >= 1.0f) animation.scale = 1.0f;
        else if (animation.scale <= 0.5f) animation.scale = 0.5f;
        const float shownX = float(roundedCoordinate(placement.outsideEllipse ? placement.edgeX : placement.x));
        const float shownY = float(roundedCoordinate(placement.outsideEllipse ? placement.edgeY : placement.y));
        const float distance = std::hypot(shownX - placement.x, shownY - placement.y);
        animation.grow += distance >= kOnScreenDistance ? kGrowRate * dt : -kGrowRate * dt;
        if (animation.grow >= 1.0f) animation.grow = 1.0f;
        else if (animation.grow <= 0.0f) animation.grow = 0.0f;
        next[marker.id] = animation;
    }
    animations_ = std::move(next);
}

const MapMarkerAnimation& MapMarkerField::animation(int id) const {
    static const MapMarkerAnimation fresh;
    const auto found = animations_.find(id);
    return found == animations_.end() ? fresh : found->second;
}

MapRing MapMarkerField::largeRing() const {
    const float t = ringFade(ringClock_);
    return {lerp(kRingLargeEnd, kRingStart, t), t};
}

MapRing MapMarkerField::smallRing() const {
    const float t = ringFade(ringClock_);
    return {lerp(kRingSmallEnd, kRingStart, t), t};
}

}
