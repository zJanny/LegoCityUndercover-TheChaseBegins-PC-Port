#include "runtime/fall_arrest_bounds.hpp"

#include <cctype>
#include <cmath>

namespace chase::runtime {

namespace {
constexpr char kFallArrestPrefix[] = "FallArrest";
constexpr std::uint8_t kFallArrestMark = 2;

bool startsWithFallArrest(const std::string& name) {
    for (std::size_t i = 0; i < kFallArrestPrefixLength; ++i) {
        if (i >= name.size()) return false;
        if (std::tolower(static_cast<unsigned char>(name[i])) != std::tolower(static_cast<unsigned char>(kFallArrestPrefix[i]))) return false;
    }
    return true;
}
}

std::vector<std::size_t> fallArrestBounds(const assets::Level& level) {
    std::vector<std::size_t> bounds;
    for (std::size_t i = 0; i < level.bounds.size(); ++i)
        if (startsWithFallArrest(level.bounds[i].name)) bounds.push_back(i);
    return bounds;
}

bool pointInFallArrestBound(const assets::LevelBound& bound, bool enabled, const std::array<float, 3>& point) {
    if (!enabled) return false;
    const float x = point[0] - bound.centre[0], y = point[1] - bound.centre[1], z = point[2] - bound.centre[2];
    switch (bound.shape) {
    case 2: return std::sqrt(x * x + y * y + z * z) < bound.half[0];
    case 3: return std::fabs(x) < bound.half[0] && std::fabs(y) < bound.half[1] && std::fabs(z) < bound.half[2];
    case 4:
    case 6: return std::sqrt(x * x + z * z) < bound.half[0] && std::fabs(y) < bound.half[1];
    default: return false;
    }
}

std::vector<FallArrestHit> fallArrestUpdate(const assets::Level& level, const std::vector<std::size_t>& bounds, const std::vector<std::uint8_t>& boundEnabled,
                                            std::vector<FallArrestCharacter>& engaged) {
    std::vector<FallArrestHit> hits;
    const std::size_t count = engaged.size() < kFallArrestEngagedSlots ? engaged.size() : kFallArrestEngagedSlots;
    for (std::size_t c = 0; c < count; ++c) {
        FallArrestCharacter& character = engaged[c];
        if (!character.enabled || character.onGround || character.arrestMark != 0) continue;
        for (const std::size_t bound : bounds) {
            if (bound >= level.bounds.size()) continue;
            const bool enabled = bound < boundEnabled.size() && boundEnabled[bound] != 0;
            if (!pointInFallArrestBound(level.bounds[bound], enabled, character.position)) continue;
            character.arrestMark = kFallArrestMark;
            hits.push_back({character.name, bound});
            break;
        }
    }
    return hits;
}

}
