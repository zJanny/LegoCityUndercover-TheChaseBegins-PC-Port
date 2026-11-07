#include "runtime/traffic_route_functions.hpp"

#include "runtime/dcam.hpp"

#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>

namespace chase::runtime {
namespace {

using Vec3 = std::array<float, 3>;

constexpr std::uint8_t kRouteTypeSpline = 0;
constexpr std::uint8_t kRouteTypeLinear = 1;
constexpr float kNearestStart = std::numeric_limits<float>::max();  // 0x21d6ac

bool isSpline(const assets::LevelPath& path) {
    if (path.type == kRouteTypeLinear) return false;
    if (path.type != kRouteTypeSpline)
        throw std::invalid_argument("traffic route type " + std::to_string(int(path.type)) + " has no route functions");
    if (path.points.empty()) throw std::invalid_argument("traffic route without points");
    return true;
}

Vec3 samplePosition(const assets::LevelPath& path, float parameter) {
    Vec3 position{};
    sampleDirectorPath(path, parameter, position.data());
    return position;
}

std::uint32_t segmentCount(const assets::LevelPath& path) {
    std::uint32_t count = static_cast<std::uint16_t>(path.points.size());
    if (!path.closed) count -= 1;
    return count;
}

float squaredDistance(const Vec3& a, const Vec3& b) {
    const float dx = a[0] - b[0];
    const float dy = a[1] - b[1];
    const float dz = a[2] - b[2];
    float sum = dx * dx;
    sum += dy * dy;
    sum += dz * dz;
    return sum;
}

float length(const Vec3& v) {
    float sum = v[0] * v[0];
    sum += v[1] * v[1];
    sum += v[2] * v[2];
    return std::sqrt(sum);
}

}

float trafficRouteAhead(const assets::LevelPath& path, float ahead, float step, float from) {
    if (!isSpline(path)) return 0.0f;
    const float limit = static_cast<float>(segmentCount(path));
    const bool squared = kTrafficRouteAheadSquaredBelow > ahead;
    const float target = squared ? ahead * ahead : ahead;
    float walked = 0.0f;
    float parameter = from;
    while (walked < target && parameter < limit) {
        const Vec3 before = samplePosition(path, parameter);
        parameter = parameter + step;
        const Vec3 after = samplePosition(path, parameter);
        const float squaredStep = squaredDistance(before, after);
        walked = (squared ? squaredStep : std::sqrt(squaredStep)) + walked;
    }
    return parameter;
}

float trafficRouteNearest(const assets::LevelPath& path, const std::array<float, 3>& position, unsigned samples, float scale) {
    if (!isSpline(path)) return 0.0f;
    const float inverse = 1.0f / scale;
    const std::uint32_t total = segmentCount(path) * std::uint32_t(samples);
    float best = kNearestStart;
    float found = 0.0f;
    for (std::uint32_t k = 1; k <= total; ++k) {
        const float parameter = static_cast<float>(k) / static_cast<float>(samples);
        const Vec3 sample = samplePosition(path, parameter);
        Vec3 delta{};
        for (int axis = 0; axis < 3; ++axis) delta[std::size_t(axis)] = (sample[std::size_t(axis)] - position[std::size_t(axis)]) * inverse;
        const float distance = length(delta);
        if (distance < best) {
            best = distance;
            found = parameter;
        }
    }
    return found;
}

}
