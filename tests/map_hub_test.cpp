// The hub's fixed map centre and angle (src/frontend/map_hub.*): the literals of 0x2de82c, 0x2de830 and 0x1ee91c, the generic
// record rule and the placement against the measurement at the hub.
#include "frontend/map_file.hpp"
#include "frontend/map_hub.hpp"
#include "frontend/map_marker.hpp"
#include <cmath>
#include <cstring>
#include <iostream>
#include <string>

namespace {
using namespace chase::frontend;

int failures = 0;

void require(bool condition, const std::string& what) {
    if (!condition) {
        std::cerr << "FAIL: " << what << '\n';
        ++failures;
    }
}

float floatOf(std::uint32_t word) {
    float value;
    std::memcpy(&value, &word, sizeof value);
    return value;
}

// Provenance: round 20261029-hudmap-B, 2026-10-29, llvm-objdump of the executable: the words at 0x2de82c, 0x2de830 and 0x1ee91c.
void literals() {
    require(floatOf(kHubMapCentreXWord) == 902.40997f && floatOf(kHubMapCentreZWord) == 1117.0f, "centre literals 0x2de82c / 0x2de830");
    require(std::fabs(floatOf(kHubMapAngleWord) + 0.80285f) < 1e-5f, "angle literal 0x1ee91c");
}

void rule() {
    const MapFrame hub = mapFrame(kHubLevelRecord, 10.0f, 20.0f, 1.0f), moved = mapFrame(kHubLevelRecord, -500.0f, 300.0f, -2.0f);
    require(hub.x == moved.x && hub.z == moved.z && hub.yaw == moved.yaw, "record 1: the frame ignores player and camera");
    for (const int record : {0, 2, 3, 8, 9, 12}) {
        const MapFrame frame = mapFrame(record, 10.0f, 20.0f, 1.0f);
        require(frame.x == 10.0f && frame.z == 20.0f && frame.yaw == 1.0f, "record " + std::to_string(record) + ": player and camera");
    }
    require(mapAngle(-1, 0.5f) == 0.5f, "an unknown record follows the camera");
}

// Provenance: 20261029-hudmap-servant HUD-11, 2026-10-29, recorded from a save state in the police station lobby after mission 1-1:
// the lvl_01 picture's centre texel is (604.4, 257.0) of 1024 at screen (160, 120), rotation about 0.805 rad (fit 0.7995 to 0.8063,
// opposite sign to the literal), and the picture neither moves nor turns while the player walks or the camera turns.
// lvl_01 map file: 21 words of the shipped sprites/ui_bottomscreen/maps/lvl_01_cherrytreehills.txt (as in map_file_test).
void hubPlacement() {
    const std::uint32_t words[21] = {0x3fcce636, 0, 0, 0, 0, 0xb3964394, 0xbe08f891, 0, 0, 0x3fcce636, 0xb1c8e594, 0, 0xc4965a81, 0xc48c33b6,
                                     0x428af17d, 0x44a72f25, 0x3fcce636, 0x3fcce636, 0x44a72f25, 0x44800000, 0x44800000};
    std::vector<std::uint8_t> bytes;
    for (const std::uint32_t word : words)
        for (int shift = 0; shift < 32; shift += 8) bytes.push_back(std::uint8_t(word >> shift));
    const MapProjection projection{parseMapFile(bytes), kMapZoomStandIn, kHubLevelRecord};
    const MapFrame frame = mapFrame(kHubLevelRecord, 0.0f, 0.0f, 0.0f);
    const std::array<float, 2> n = mapNormalised(projection.file, frame.x, 0.0f, frame.z);
    const float u = 512.0f + 512.0f * n[0], v = 512.0f - 512.0f * n[1];
    require(std::fabs(u - 604.4f) < 0.5f && std::fabs(v - 257.0f) < 0.5f, "centre texel (604.4, 257.0)");
    require(frame.yaw > 0.7995f && frame.yaw < 0.8063f, "rotation 0.7995 to 0.8063");
    const MapMarkerPlacement a = placeMapMarker(projection, 12.0f, 34.0f, 2.5f, 1000.0f, 1000.0f);
    const MapMarkerPlacement b = placeMapMarker(projection, 12.0f, 34.0f, -1.0f, 1000.0f, 1000.0f);
    require(a.x == b.x && a.y == b.y, "hub markers turn by the fixed angle, not the camera");
    const MapProjection other{projection.file, kMapZoomStandIn, 2};
    const MapMarkerPlacement c = placeMapMarker(other, 12.0f, 34.0f, 2.5f, 1000.0f, 1000.0f);
    const MapMarkerPlacement d = placeMapMarker(other, 12.0f, 34.0f, -1.0f, 1000.0f, 1000.0f);
    require(c.x != d.x, "other records turn markers with the camera");
}

}

int main() {
    literals();
    rule();
    hubPlacement();
    if (failures) return 1;
    std::cout << "map-hub-test ok\n";
    return 0;
}
