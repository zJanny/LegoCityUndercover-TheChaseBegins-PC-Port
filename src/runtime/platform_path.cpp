#include "runtime/platform_path.hpp"

#include "runtime/detective.hpp"
#include "runtime/pedestrian_walk.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string>

namespace chase::runtime {
namespace {

constexpr std::int32_t kLinearDotBits = 0x3f7fff58;   // literal at 0x21a724
constexpr std::int32_t kSteepForwardBits = 0x3f7ae148;  // literal at 0x237a74 (0.98)
constexpr std::uint32_t kTinyLengthBits = 0x34000000;  // cmp r0, #872415232 at 0x257ec8
constexpr int kKeepSign = 2;
constexpr std::uint8_t kSplinePath = 0;

std::int32_t bitsOf(float value) {
    std::int32_t bits = 0;
    std::memcpy(&bits, &value, sizeof bits);
    return bits;
}

std::array<float, 3> normalised(const std::array<float, 3>& v) {
    float squared = v[0] * v[0];
    squared = squared + v[1] * v[1];
    squared = squared + v[2] * v[2];
    const float length = std::sqrt(squared);
    if (bitsOf(length) <= std::int32_t(kTinyLengthBits)) return {0.0f, 0.0f, 1.0f};
    const float inverse = 1.0f / length;
    return {v[0] * inverse, v[1] * inverse, v[2] * inverse};
}

const PathQuaternion& quaternionAt(const std::vector<PathQuaternion>& table, int index) {
    const std::size_t last = table.size() - 1;
    return table[std::min<std::size_t>(std::size_t(std::max(index, 0)), last)];
}

}

PathQuaternion slerpPathQuaternion(const PathQuaternion& a, const PathQuaternion& b, float t, int mode) {
    float dot = a[0] * b[0];
    dot = dot + a[1] * b[1];
    dot = dot + a[2] * b[2];
    dot = dot + a[3] * b[3];
    float sign = 1.0f;
    if (mode != kKeepSign && int(dot < 0.0f) != mode) {
        dot = -dot;
        sign = -1.0f;
    }
    float weightA = 1.0f - t, weightB = 0.0f;
    if (bitsOf(dot) > kLinearDotBits) {
        weightB = sign * t;
    } else {
        const float theta = std::acos(dot);
        const float sine = std::sin(theta);
        weightA = std::sin(weightA * theta) / sine;
        weightB = std::sin(t * theta) / sine * sign;
    }
    PathQuaternion out{};
    for (std::size_t k = 0; k < 4; ++k) out[k] = a[k] * weightA + weightB * b[k];
    return out;
}

PathQuaternion pathSquadOrientation(const assets::LevelPath& path, float position) {
    const int count = int(path.points.size());
    if (path.orientations.empty() || path.orientationControls.empty() || count == 0) return {0.0f, 0.0f, 0.0f, 1.0f};
    if (!path.closed) {
        if (position < 0.0f) position = 0.0f;
        if (position > float(count - 1)) position = float(count - 1);
    }
    const int floored = int(std::floor(position));
    const float f = position - float(floored);
    int from = floored, to = floored + 1;
    if (path.closed && count >= 3) {
        from = floored % count;
        if (from < 0) from += count;
    } else if (to > count - 1) {
        to = count - 1;
    }
    const PathQuaternion outer = slerpPathQuaternion(quaternionAt(path.orientations, from), quaternionAt(path.orientations, to), f, kKeepSign);
    const PathQuaternion inner =
        slerpPathQuaternion(quaternionAt(path.orientationControls, from), quaternionAt(path.orientationControls, to), f, kKeepSign);
    PathQuaternion q = slerpPathQuaternion(outer, inner, 2.0f * f * (1.0f - f), kKeepSign);
    float squared = q[0] * q[0];
    squared = squared + q[1] * q[1];
    squared = squared + q[2] * q[2];
    squared = squared + q[3] * q[3];
    const float inverse = 1.0f / std::sqrt(squared);
    for (float& component : q) component = component * inverse;
    return q;
}

PathRotation pathQuaternionRotation(const PathQuaternion& q) {
    const float x = q[0], y = q[1], z = q[2], w = q[3];
    const float x2 = x + x, y2 = y + y, z2 = z + z;
    const float xx = x * x2, wx = w * x2, yy = y * y2, zz = z * z2, yz = y * z2, xy = x * y2, xz = x * z2, wz = w * z2, wy = w * y2;
    return {1.0f - (yy + zz), xy - wz, xz + wy, xy + wz, 1.0f - (xx + zz), yz - wx, xz - wy, yz + wx, 1.0f - (xx + yy)};
}

PathRotation pathForwardRotation(const std::array<float, 3>& forward) {
    std::array<float, 3> right{};
    if (bitsOf(std::fabs(forward[1])) <= kSteepForwardBits) right = {forward[2], 0.0f, -forward[0]};
    else right = {0.0f, -forward[2], forward[1]};
    right = normalised(right);
    const std::array<float, 3> up = {forward[1] * right[2] - forward[2] * right[1], forward[2] * right[0] - forward[0] * right[2],
                                     forward[0] * right[1] - forward[1] * right[0]};
    return {right[0], right[1], right[2], up[0], up[1], up[2], forward[0], forward[1], forward[2]};
}

PathRotation multiplyPathRotation(const PathRotation& a, const PathRotation& b, bool transposed) {
    PathRotation out{};
    for (std::size_t row = 0; row < 3; ++row)
        for (std::size_t column = 0; column < 3; ++column) {
            const auto at = [&](std::size_t k) { return transposed ? a[k * 3 + row] : a[row * 3 + k]; };
            out[row * 3 + column] = at(0) * b[column] + at(1) * b[3 + column] + at(2) * b[6 + column];
        }
    return out;
}

PathRotation transposedPathRotation(const PathRotation& rows) {
    return {rows[0], rows[3], rows[6], rows[1], rows[4], rows[7], rows[2], rows[5], rows[8]};
}

std::array<float, 3> rotatePathPoint(const std::array<float, 3>& p, const PathRotation& rows) {
    std::array<float, 3> out{};
    for (std::size_t column = 0; column < 3; ++column) out[column] = p[0] * rows[column] + p[1] * rows[3 + column] + p[2] * rows[6 + column];
    return out;
}

PathRotation normalisedPathRotation(const PathRotation& rows) {
    PathRotation out{};
    for (std::size_t row = 0; row < 3; ++row) {
        const std::array<float, 3> unit = normalised({rows[row * 3], rows[row * 3 + 1], rows[row * 3 + 2]});
        std::copy(unit.begin(), unit.end(), out.begin() + std::ptrdiff_t(row * 3));
    }
    return out;
}

std::shared_ptr<const PlatformPath> platformPathOf(const assets::LevelPath& path, bool constantSpeed) {
    auto track = std::make_shared<PlatformPath>();
    track->path = path;
    if (constantSpeed) {
        if (path.type != kSplinePath) throw std::invalid_argument("platform path type " + std::to_string(int(path.type)) + " has no ported length sampler");
        track->lengths = detectivePathLengths(track->path, kPlatformPathLengthSteps);
    }
    return track;
}

PlatformPathPose platformPathPose(const PlatformPath& track, float shaped, bool constantSpeed, std::uint8_t orient) {
    const assets::LevelPath& path = track.path;
    const float count = float(path.points.size());
    const float parameter = constantSpeed ? detectivePathParameter(path, track.lengths, shaped) : count * shaped;
    PlatformPathPose pose;
    PedestrianVec3 position{}, tangent{};
    pedestrianLevelPath(path).sample(parameter, position, tangent);
    pose.position = position;
    if (!path.orientations.empty()) pose.rotation = pathQuaternionRotation(pathSquadOrientation(path, count * shaped));
    else if (orient != 0) pose.rotation = pathForwardRotation(tangent);
    return pose;
}

}
