#include "frontend/map_hub.hpp"
#include <cstring>

namespace chase::frontend {

namespace {

float floatOf(std::uint32_t word) {
    float value;
    std::memcpy(&value, &word, sizeof value);
    return value;
}

}

float mapAngle(int levelRecord, float cameraYaw) {
    return levelRecord == kHubLevelRecord ? -floatOf(kHubMapAngleWord) : cameraYaw;
}

MapFrame mapFrame(int levelRecord, float playerX, float playerZ, float cameraYaw) {
    if (levelRecord == kHubLevelRecord) return {floatOf(kHubMapCentreXWord), floatOf(kHubMapCentreZWord), mapAngle(levelRecord, cameraYaw)};
    return {playerX, playerZ, cameraYaw};
}

}
