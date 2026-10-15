#include "runtime/sky.hpp"
#include <algorithm>

namespace chase::runtime {

float skyLift(float playerY, float seaY) {
    const float height = std::min(playerY - seaY, kSkyHeightRange);
    const float clamped = std::max(height, 0.0f);
    return kSkyMaxLift * clamped / kSkyHeightRange;
}

std::array<float, 3> skyTranslation(const float eye[3], float lift) { return {eye[0], eye[1] - lift, eye[2]}; }

}
