#include "runtime/traffic_leave_snap.hpp"

#include <cmath>
#include <cstring>
#include <stdexcept>

namespace chase::runtime {

namespace {

std::int32_t bitsOf(float value) {
    std::int32_t bits;
    std::memcpy(&bits, &value, sizeof bits);
    return bits;
}

std::uint32_t channelOf(float value) {
    if (!(value > -2147483648.0f)) return 0;
    if (value >= 2147483648.0f) return 255;
    const std::int32_t truncated = static_cast<std::int32_t>(value);
    if (truncated < 0) return 0;
    return truncated > 255 ? 255u : static_cast<std::uint32_t>(truncated);
}

std::uint32_t packColour(std::uint32_t first, std::uint32_t second, std::uint32_t third) {
    return third | (second << 8) | (first << 16) | 0xff000000u;
}

}

bool leaveTraffic(TrafficManager& manager, std::size_t car, std::vector<TrafficEvent>* events) {
    if (car >= manager.cars().size()) return false;
    manager.leaveTraffic(car, events);
    return true;
}

TrafficGroundSegment trafficGroundSegment(const AiVec3& target) {
    return {{target[0], target[1] + kTrafficGroundProbeHalfHeight, target[2]}, {target[0], target[1] - kTrafficGroundProbeHalfHeight, target[2]}};
}

std::optional<std::array<float, 3>> trafficTriangleWeights(const TrafficGroundTriangle& triangle, const AiVec3& point) {
    const float absX = std::fabs(triangle.normal[0]), absY = std::fabs(triangle.normal[1]), absZ = std::fabs(triangle.normal[2]);
    std::size_t a, b;
    if (!(absX > absY)) {
        a = 0;
        b = !(absZ < absY) ? 1 : 2;
    } else if (absX > absZ) {
        a = 1;
        b = 2;
    } else {
        a = 0;
        b = 2;
    }
    const AiVec3& v0 = triangle.corners[0];
    const AiVec3& v1 = triangle.corners[1];
    const AiVec3& v2 = triangle.corners[2];
    const float scale = kTrafficGroundWeightScale;
    const float dA = (v1[b] - v2[b]) * scale;
    const float dB = (v0[a] - v2[a]) * scale;
    const float dC = (v1[a] - v2[a]) * scale;
    const float dD = (v0[b] - v2[b]) * scale;
    const float pa0 = (point[a] - v0[a]) * scale;
    const float pa2 = (point[a] - v2[a]) * scale;
    const float pb2 = (point[b] - v2[b]) * scale;
    const float pb0 = (point[b] - v0[b]) * scale;
    float determinant = dB * dA;
    determinant = determinant - dC * dD;
    if (determinant == 0.0f) return std::nullopt;
    float first = pa2 * dA;
    first = first - dC * pb2;
    float second = dB * pb0;
    second = second - pa0 * dD;
    const float inverse = 1.0f / determinant;
    const float w0 = first * inverse;
    const float w1 = second * inverse;
    const float w2 = (1.0f - w0) - w1;
    return std::array<float, 3>{w0, w1, w2};
}

std::optional<std::uint32_t> trafficGroundColour(const TrafficGroundMesh& mesh, const TrafficGroundTriangle& triangle, const AiVec3& point) {
    const std::optional<std::array<float, 3>> weights = trafficTriangleWeights(triangle, point);
    if (!weights) return std::nullopt;
    if (mesh.hasColours) {
        float sums[3] = {0.0f, 0.0f, 0.0f};
        for (std::size_t corner = 0; corner < 3; ++corner) {
            const std::uint32_t word = mesh.colours.at(triangle.colourIndex[corner]);
            for (std::size_t channel = 0; channel < 3; ++channel) {
                float product = static_cast<float>((word >> (8 * channel)) & 0xffu) * (*weights)[corner];
                sums[channel] = sums[channel] + product;
            }
        }
        return packColour(channelOf(sums[0]), channelOf(sums[1]), channelOf(sums[2]));
    }
    float shade = static_cast<float>(triangle.colourIndex[0]) * (*weights)[0];
    shade = shade + static_cast<float>(triangle.colourIndex[1]) * (*weights)[1];
    shade = shade + static_cast<float>(triangle.colourIndex[2]) * (*weights)[2];
    const float value = shade;
    float clamped = value;
    if (value < 0.0f) clamped = 0.0f;
    else if (bitsOf(value) > kTrafficGroundMaxBits) clamped = 255.0f;
    const std::uint32_t grey = static_cast<std::uint32_t>(static_cast<std::int32_t>(clamped)) & 0xffu;
    return packColour(grey, grey, grey);
}

bool snapTrafficTarget(const AiDriverRecord& driver, const TrafficGroundWorld& world) {
    if (!world.cast) throw std::logic_error("snapTrafficTarget: no ray cast");
    if (!world.sendColour) throw std::logic_error("snapTrafficTarget: no colour receiver");
    const std::optional<TrafficGroundHit> hit = world.cast(trafficGroundSegment(driver.target));
    if (!hit) return false;
    const std::optional<std::uint32_t> colour = trafficGroundColour(hit->mesh, hit->triangle, hit->point);
    if (!colour) return false;
    world.sendColour(*colour);
    return true;
}

}
