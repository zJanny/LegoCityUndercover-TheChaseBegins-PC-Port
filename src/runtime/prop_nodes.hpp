#pragma once
// Per-node transforms of a placed prop: the pose math the original uses to move single nodes of an object (build-it bricks, 0x19c9e0 with
// 0x248618, 0x21a600 and 0x248870) and the node matrices a scene draws. Rules and measurements: docs/re/buildit.md. No SDL dependency.
#include "assets/assets.hpp"
#include <array>
#include <cstddef>
#include <vector>

namespace chase::runtime {

using NodeMatrix = assets::Matrix;

struct NodePose {
    std::array<float, 4> rotation{0, 0, 0, 1};  // quaternion x, y, z, w
    std::array<float, 3> position{0, 0, 0};
};

// 0x248618 (matrix to quaternion) and the translation row.
NodePose nodePoseOf(const NodeMatrix& matrix);
// 0x248870 (quaternion to the 3x3 part) and the translation row.
NodeMatrix nodeMatrixOf(const NodePose& pose);
// 0x21a600 with flag 0: shortest-arc spherical interpolation, linear above a dot product of 0.99999; the result is not normalised.
std::array<float, 4> nodeSlerp(const std::array<float, 4>& from, const std::array<float, 4>& to, float t);
// 0x19c9e0 without a via point: slerp of the rotations, linear position, and a hop of 0.2 * 4t(1 - t) * (|dx| + |dz|) added to y.
NodeMatrix nodeFlight(const NodePose& start, const NodePose& end, float t);

// Nodes of a model as the scene draws them: one matrix per node (row-vector convention, relative to the object), starting from the model's own
// node matrices. A node without a mesh is skipped by the scene.
struct PropNodes {
    std::vector<NodeMatrix> local;
    std::vector<int> mesh;  // mesh index of each node, -1 for none

    static PropNodes of(const assets::Model& model);
    // World matrix of a node: local * placement.
    NodeMatrix world(std::size_t node, const NodeMatrix& placement) const;
};

}
