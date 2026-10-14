#include "runtime/ai_face_target.hpp"

#include <cmath>

namespace chase::runtime {

FaceTargetResult stepFaceTarget(const std::array<float, 3>& position, const std::array<float, 3>& home, const std::array<float, 3>* target,
                                std::uint16_t previousHeading) {
    FaceTargetResult result;
    result.position = {home[0], position[1], home[2]};
    result.heading = previousHeading;
    if (!target) return result;
    const float angle = std::atan2((*target)[0] - result.position[0], (*target)[2] - result.position[2]) * 10430.378f;
    result.heading = static_cast<std::uint16_t>(static_cast<std::int32_t>(angle));
    result.facing = true;
    return result;
}

}
