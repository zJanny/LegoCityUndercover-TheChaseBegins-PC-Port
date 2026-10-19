#include "runtime/prop_nodes.hpp"

#include <cmath>

namespace chase::runtime {
namespace {

constexpr float kSlerpLinearAbove = 0.9999899864196777f;
constexpr float kFlightHop = 0.2f;

NodeMatrix identityMatrix() { return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}; }

}

NodePose nodePoseOf(const NodeMatrix& m) {
    NodePose pose;
    float q[4];
    const float trace = m[0] + m[5] + m[10];
    if (trace > 0.0f) {
        const float root = std::sqrt(trace + 1.0f);
        const float scale = 0.5f / root;
        q[3] = root * 0.5f;
        q[0] = (m[9] - m[6]) * scale;
        q[1] = (m[2] - m[8]) * scale;
        q[2] = (m[4] - m[1]) * scale;
    } else {
        constexpr int next[3] = {1, 2, 0};
        int i = 0;
        if (m[5] > m[0]) i = 1;
        if (m[10] > m[i * 5]) i = 2;
        const int j = next[i], k = next[j];
        float root = std::sqrt(m[i * 5] - (m[j * 5] + m[k * 5]) + 1.0f);
        q[i] = root * 0.5f;
        if (root != 0.0f) root = 0.5f / root;
        q[3] = (m[k * 4 + j] - m[j * 4 + k]) * root;
        q[j] = (m[i * 4 + j] + m[j * 4 + i]) * root;
        q[k] = (m[i * 4 + k] + m[k * 4 + i]) * root;
    }
    pose.rotation = {q[0], q[1], q[2], q[3]};
    pose.position = {m[12], m[13], m[14]};
    return pose;
}

NodeMatrix nodeMatrixOf(const NodePose& pose) {
    NodeMatrix m = identityMatrix();
    const auto [x, y, z, w] = pose.rotation;
    m[0] = 1 - 2 * (y * y + z * z);
    m[1] = 2 * (x * y - w * z);
    m[2] = 2 * (x * z + w * y);
    m[4] = 2 * (x * y + w * z);
    m[5] = 1 - 2 * (x * x + z * z);
    m[6] = 2 * (y * z - w * x);
    m[8] = 2 * (x * z - w * y);
    m[9] = 2 * (y * z + w * x);
    m[10] = 1 - 2 * (x * x + y * y);
    m[12] = pose.position[0];
    m[13] = pose.position[1];
    m[14] = pose.position[2];
    return m;
}

std::array<float, 4> nodeSlerp(const std::array<float, 4>& from, const std::array<float, 4>& to, float t) {
    float dot = from[0] * to[0] + from[1] * to[1] + from[2] * to[2] + from[3] * to[3];
    float sign = 1.0f;
    if (dot < 0.0f) {
        dot = -dot;
        sign = -1.0f;
    }
    float weightFrom = 1.0f - t;
    float weightTo;
    if (dot > kSlerpLinearAbove) {
        weightTo = sign * t;
    } else {
        const float omega = std::acos(dot);
        const float sine = std::sin(omega);
        weightFrom = std::sin(weightFrom * omega) / sine;
        weightTo = std::sin(t * omega) / sine * sign;
    }
    return {from[0] * weightFrom + weightTo * to[0], from[1] * weightFrom + weightTo * to[1], from[2] * weightFrom + weightTo * to[2],
            from[3] * weightFrom + weightTo * to[3]};
}

NodeMatrix nodeFlight(const NodePose& start, const NodePose& end, float t) {
    NodePose pose;
    pose.rotation = nodeSlerp(start.rotation, end.rotation, t);
    for (int a = 0; a < 3; ++a) pose.position[std::size_t(a)] = start.position[std::size_t(a)] + (end.position[std::size_t(a)] - start.position[std::size_t(a)]) * t;
    NodeMatrix m = nodeMatrixOf(pose);
    const float centred = 1.0f - 2.0f * t;
    const float hop = (1.0f - centred * centred) * kFlightHop;
    const float distance = std::fabs(start.position[0] - end.position[0]) + std::fabs(start.position[2] - end.position[2]);
    m[13] += distance * hop;
    return m;
}

PropNodes PropNodes::of(const assets::Model& model) {
    PropNodes nodes;
    nodes.mesh = model.nodeMesh;
    for (std::size_t n = 0; n < model.nodeMesh.size(); ++n) nodes.local.push_back(assets::nodeDrawMatrix(model, n));
    return nodes;
}

NodeMatrix PropNodes::world(std::size_t node, const NodeMatrix& placement) const {
    return assets::multiply(local.at(node), placement);
}

NodeMatrix nodeRelativeTo(const NodeMatrix& target, const NodeMatrix& own) {
    NodeMatrix inverse{};
    for (std::size_t row = 0; row < 3; ++row)
        for (std::size_t column = 0; column < 3; ++column) inverse[row * 4 + column] = own[column * 4 + row];
    for (std::size_t column = 0; column < 3; ++column)
        inverse[12 + column] = -(own[12] * inverse[column] + own[13] * inverse[4 + column] + own[14] * inverse[8 + column]);
    inverse[15] = 1.0f;
    return assets::multiply(target, inverse);
}

}
