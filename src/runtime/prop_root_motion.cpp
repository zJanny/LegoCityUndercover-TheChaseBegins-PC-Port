#include "runtime/prop_root_motion.hpp"

#include <algorithm>
#include <cmath>

namespace chase::runtime {
namespace {

RootSample sampleAt(const assets::Animation& animation, std::size_t curve, unsigned frame, unsigned samples) {
    const assets::JointChannels channels = assets::decodeChannels(animation.jointCurves[curve], std::min(frame, samples - 1), samples);
    const assets::Matrix matrix = assets::localMatrix(channels);
    RootSample sample;
    std::copy_n(channels.values + 9, 3, sample.translation);
    sample.yaw = std::atan2(matrix[8], matrix[10]);
    return sample;
}

}

std::optional<RootSample> rootSample(const assets::Animation& animation, double frame) {
    const std::uint32_t word = animation.jointTableWord;
    if (!animation.hasJointTable || !(word & 0x10000000u)) return std::nullopt;
    const std::size_t curve = word & 0x1fffu;
    const unsigned samples = ((word >> 15) & 0x1fffu) + ((word >> 29) & 1u);
    if (curve >= animation.jointCurves.size() || samples == 0) return std::nullopt;
    const double clamped = std::clamp(frame, 0.0, double(samples - 1));
    const unsigned lower = unsigned(std::floor(clamped));
    const float fraction = float(clamped - double(lower));
    const RootSample a = sampleAt(animation, curve, lower, samples);
    if (fraction == 0.0f) return a;
    const RootSample b = sampleAt(animation, curve, lower + 1, samples);
    RootSample out;
    for (int k = 0; k < 3; ++k) out.translation[k] = a.translation[k] + (b.translation[k] - a.translation[k]) * fraction;
    out.yaw = a.yaw + (b.yaw - a.yaw) * fraction;
    return out;
}

RootSample rootDelta(const RootSample& previous, const RootSample& current) {
    const float c = std::cos(previous.yaw), s = std::sin(previous.yaw);
    const float dx = current.translation[0] - previous.translation[0], dy = current.translation[1] - previous.translation[1],
                dz = current.translation[2] - previous.translation[2];
    RootSample delta;
    delta.translation[0] = dx * c - dz * s;
    delta.translation[1] = dy;
    delta.translation[2] = dx * s + dz * c;
    delta.yaw = current.yaw - previous.yaw;
    return delta;
}

assets::Matrix applyRootDelta(const assets::Matrix& node, const RootSample& delta) {
    const float c = std::cos(delta.yaw), s = std::sin(delta.yaw);
    const float turn[3][3] = {{c, 0, -s}, {0, 1, 0}, {s, 0, c}};
    assets::Matrix out = node;
    for (int row = 0; row < 3; ++row)
        for (int column = 0; column < 3; ++column) {
            float sum = 0;
            for (int k = 0; k < 3; ++k) sum += turn[row][k] * node[std::size_t(k * 4 + column)];
            out[std::size_t(row * 4 + column)] = sum;
        }
    for (int column = 0; column < 3; ++column) {
        float sum = 0;
        for (int k = 0; k < 3; ++k) sum += delta.translation[k] * node[std::size_t(k * 4 + column)];
        out[std::size_t(12 + column)] = node[std::size_t(12 + column)] + sum;
    }
    return out;
}

}
