#include "runtime/ai_go_to_target.hpp"

#include <cmath>

namespace chase::runtime {

GoToTargetResult stepGoToTarget(const GoToTarget& order, const std::array<float, 3>& position, std::uint16_t& heading) {
    const float dx = order.target[0] - position[0];
    const float dz = order.target[2] - position[2];
    float squared = dx * dx;
    squared = squared + dz * dz;
    const float distance = std::sqrt(squared);
    if (order.minDistance > distance) return GoToTargetResult::Arrived;
    if (!(position[1] >= order.target[1]) && order.arriveBelowTarget) return GoToTargetResult::Arrived;
    const float angle = std::atan2(dx, dz) * 10430.378f;
    heading = static_cast<std::uint16_t>(static_cast<std::int32_t>(angle));
    return GoToTargetResult::Moving;
}

}
