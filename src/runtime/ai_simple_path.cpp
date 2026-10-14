#include "runtime/ai_simple_path.hpp"

#include <cmath>

namespace chase::runtime {
namespace {
constexpr float kHeadingUnitsPerRadian = 10430.378f;
}

bool nextSimplePathIndex(std::uint16_t index, std::uint16_t count, bool reverse, bool closed, std::uint16_t& next) {
    std::int32_t value = index;
    if (reverse) {
        value -= 1;
        if (value < 0) {
            if (!closed) return false;
            value = value - 1 + count;
        }
    } else {
        value += 1;
        if (!(count > value)) {
            if (!closed) return false;
            value -= count;
        }
    }
    next = static_cast<std::uint16_t>(value);
    return true;
}

SimplePathResult stepSimplePath(SimplePathFollow& follow, const std::array<float, 3>& position, std::uint16_t& heading) {
    if (follow.index >= follow.nodes.size()) return SimplePathResult::Ended;
    const std::array<float, 3>& node = follow.nodes[follow.index];
    const float dx = node[0] - position[0], dz = node[2] - position[2];
    heading = static_cast<std::uint16_t>(static_cast<std::int32_t>(std::atan2(dx, dz) * kHeadingUnitsPerRadian));
    const float distance = std::sqrt(dx * dx + dz * dz);
    if (!(distance >= kSimplePathArrivalDistance)) {
        std::uint16_t next = 0;
        if (!nextSimplePathIndex(follow.index, static_cast<std::uint16_t>(follow.nodes.size()), follow.reverse, follow.closed, next))
            return SimplePathResult::Ended;
        follow.index = next;
        return SimplePathResult::Advanced;
    }
    return SimplePathResult::Moving;
}

}
