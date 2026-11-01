#include "runtime/attached_drawn_log.hpp"

#include "runtime/node_attach.hpp"

#include <cmath>

namespace chase::runtime {

std::optional<std::array<float, 3>> AttachedDrawnLog::observe(const std::string& child, const assets::Matrix& childWorld, const assets::Matrix& parentWorld) {
    if (logged_.count(child)) return std::nullopt;
    const std::array<float, 3> parentAt = {parentWorld[12], parentWorld[13], parentWorld[14]};
    const auto first = firstSeen_.emplace(child, parentAt).first->second;
    const float dx = parentAt[0] - first[0], dy = parentAt[1] - first[1], dz = parentAt[2] - first[2];
    if (std::sqrt(dx * dx + dy * dy + dz * dz) <= kAttachedDrawnLogDistance) return std::nullopt;
    logged_.insert(child);
    const assets::Matrix local = relativeToParent(childWorld, parentWorld);
    return std::array<float, 3>{local[12], local[13], local[14]};
}

}
