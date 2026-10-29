// Map screen icons (src/frontend/map_marker.*): placement, edge ellipse, arrow rotation, grow and ring clock against values
// of the original's code (0x2db0dc, 0x2ddd64) and oracle measurements.
#include "frontend/map_marker.hpp"
#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {
using namespace chase::frontend;

int failures = 0;

void require(bool condition, const std::string& what) {
    if (!condition) {
        std::cerr << "FAIL: " << what << '\n';
        ++failures;
    }
}

bool near(float a, float b, float tolerance) { return std::fabs(a - b) <= tolerance; }

// Provenance: round 20261029-hudmap-A, 2026-10-29, the 21 words of sprites/ui_bottomscreen/maps/lvl_01_cherrytreehills.txt
// (the same values as the map struct in memory at gameplay-start, objective-hud-map.md §2026-10-15T19:56); zoom 1.55 from the oracle.
MapProjection cherryTreeHills() {
    const std::uint32_t words[21] = {0x3fcce636, 0, 0, 0, 0, 0xb3964394, 0xbe08f891, 0, 0, 0x3fcce636, 0xb1c8e594, 0, 0xc4965a81,
                                     0xc48c33b6, 0x428af17d, 0x44a72f25, 0x3fcce636, 0x3fcce636, 0x44a72f25, 0x44800000, 0x44800000};
    std::vector<std::uint8_t> bytes;
    for (std::uint32_t word : words)
        for (int shift = 0; shift < 32; shift += 8) bytes.push_back(std::uint8_t(word >> shift));
    return {parseMapFile(bytes), kMapZoomStandIn};
}

const MapProjection kLvl01 = cherryTreeHills();
const float kMapPixelsPerUnit = mapScreenOffset(kLvl01, 0, 0, 1000.0f, 0)[0] / 1000.0f;

void placement() {
    // Oracle, state gameplay-start, slot record of 1-1_Waypoint_01 read from memory (0x08f6d530: x 0x43335480, y 0x4239ab96):
    // player (886.7644, 1111.1659), camera look (-0.7013, -0.6929) from the shotloop log, marker (817.35, 1071.20).
    const float yaw = std::atan2(-0.7013f, -0.6929f);
    const MapMarkerPlacement first = placeMapMarker(kLvl01, 886.7644f, 1111.1659f, yaw, 817.35f, 1071.20f);
    require(near(first.x, 179.33f, 0.3f) && near(first.y, 46.42f, 0.3f), "slot position of the first waypoint");
    require(first.onScreen && !first.outsideEllipse, "the first waypoint is on screen and inside the ellipse");

    const MapMarkerPlacement ahead = placeMapMarker(kLvl01, 0, 0, 0, 0, 1000.0f);
    require(ahead.outsideEllipse && ahead.onScreen == false, "far ahead is outside");
    require(near(ahead.edgeX, kMapCentreX, 0.01f) && near(ahead.edgeY, kMapCentreY - kEdgeHalfHeight, 0.01f), "the edge point is on the ellipse");
    require(near(kEdgeHalfWidth, 104.8f, 0.001f) && near(kEdgeHalfHeight, 100.8f, 0.001f), "half axes 320 * 0.44 - 36 and 240 * 0.57 - 36");
    require(near(kMapPixelsPerUnit, 0.94977f, 0.0001f), "map units are 0.94977 pixels");
    const MapMarkerPlacement edgeOnly = placeMapMarker(kLvl01, 0, 0, 0, 0, 100.0f);
    require(!edgeOnly.outsideEllipse, "100 units ahead is still inside the ellipse");
}

void arrow() {
    // 0x2672c0 turns the sprite by R(theta) in screen coordinates; the sprite points up; pi - atan2(x, y) is 0x2db0dc's angle.
    MapMarkerPlacement p;
    p.directionX = 0;
    p.directionY = -1;
    require(near(arrowRotation(p), 0.0f, 1e-4f), "outward up: no turn");
    p.directionY = 1;
    require(near(arrowRotation(p), 3.14159265f, 1e-4f), "outward down: half turn");
    p.directionX = 1;
    p.directionY = 0;
    require(near(arrowRotation(p), 1.5707963f, 1e-4f), "outward right: quarter turn clockwise");
}

void animation() {
    MapMarkerField field;
    const std::vector<MapMarkerSource> markers = {{7, 0.0f, -1000.0f}};
    field.advance(0.1f, kLvl01, 0, 0, 0, markers);
    require(near(field.animation(7).scale, 0.7f, 1e-4f), "the icon scale rises 2 per second away from the screen");
    require(near(field.animation(7).grow, 0.2f, 1e-4f), "the shown icon grows 2 per second when the edge point is 24 or more pixels away");
    for (int i = 0; i < 20; ++i) field.advance(0.1f, kLvl01, 0, 0, 0, markers);
    require(field.animation(7).scale == 1.0f && field.animation(7).grow == 1.0f, "both clamp at 1");
    const std::vector<MapMarkerSource> near_ = {{7, 0.0f, -10.0f}};
    for (int i = 0; i < 20; ++i) field.advance(0.1f, kLvl01, 0, 0, 0, near_);
    require(field.animation(7).scale == 0.5f && field.animation(7).grow == 0.0f, "on screen: scale 0.5, grow 0");
    require(field.animation(99).scale == 0.5f, "a fresh marker starts at scale 0.5");
}

void ring() {
    MapMarkerField field;
    require(field.ringsActive() && near(field.largeRing().radius, 2.0f, 1e-4f) && near(field.largeRing().alpha, 1.0f, 1e-4f), "the ring starts at radius 2");
    field.setRingClock(1.0f);
    require(near(field.largeRing().radius, 101.0f, 1e-3f) && near(field.largeRing().alpha, 0.5f, 1e-4f), "large ring at 1 s: 200 + (2 - 200) * 0.5");
    require(near(field.smallRing().radius, 11.0f, 1e-3f), "small ring at 1 s: 20 + (2 - 20) * 0.5");
    field.setRingClock(2.0f);
    require(!field.ringsActive(), "no ring from 2 s on");
    field.setRingClock(2.9f);
    field.advance(0.2f, kLvl01, 0, 0, 0, {});
    require(near(field.ringClock(), 0.0f, 1e-6f), "the clock restarts after 3 s");
}
}

void onMap() {
    // 0x33dccc: half axes 320 * 0.42 - 36 = 98.4 and 240 * 0.41 - 36 = 62.4 (literals 0x33de40, 0x33de44, 0x33de48), boundary inside.
    require(near(kOnMapHalfWidth, 98.4f, 0.001f) && near(kOnMapHalfHeight, 62.4f, 0.001f), "inner ellipse of the sub-objective update");
    const float ahead = 62.4f / kMapPixelsPerUnit;
    require(mapPositionOnMap(kLvl01, 0, 0, 0, 0, ahead * 0.999f) && !mapPositionOnMap(kLvl01, 0, 0, 0, 0, ahead * 1.01f), "ahead: 62.4 pixels up");
    const float side = 98.4f / kMapPixelsPerUnit;
    require(mapPositionOnMap(kLvl01, 0, 0, 0, side * 0.999f, 0) && !mapPositionOnMap(kLvl01, 0, 0, 0, side * 1.01f, 0), "aside: 98.4 pixels right");
    require(std::string(mapIconTexture(14)).find("subobjectivepoint.btga") != std::string::npos &&
                std::string(mapIconTexture(13)).find("subobjectiveenemy.btga") != std::string::npos &&
                std::string(mapIconTexture(15)).find("subobjectivepointgreen.btga") != std::string::npos,
            "icons 13, 14, 15 of the table 0x3588a0");
    require(std::string(mapOffscreenArrowTexture(14)).find("blue") != std::string::npos &&
                std::string(mapOffscreenArrowTexture(13)).find("red") != std::string::npos &&
                std::string(mapOffscreenArrowTexture(15)).find("green") != std::string::npos &&
                std::string(mapOffscreenArrowTexture(7)).find("white") != std::string::npos,
            "offscreen arrows 40, 42, 41 and 43 of 0x2db0dc");
}

int main() {
    placement();
    onMap();
    arrow();
    animation();
    ring();
    if (failures) return 1;
    std::cout << "map-marker-test: ok\n";
    return 0;
}
