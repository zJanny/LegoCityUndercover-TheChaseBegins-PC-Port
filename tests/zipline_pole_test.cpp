// The matrix of a zipline anchor's pole (src/runtime/zipline_pole.cpp): the model's node matrix applied first, then the anchor's node.
#include "runtime/zipline_pole.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using chase::assets::Matrix;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b) { return std::fabs(a - b) < 1e-4f; }

void point(const Matrix& m, const float in[3], float out[3]) {
    for (int c = 0; c < 3; ++c) out[c] = in[0] * m[c] + in[1] * m[4 + c] + in[2] * m[8 + c] + m[12 + c];
}
}

int main() {
    using namespace chase;
    try {
        assets::Model model;
        require(!runtime::modelRootNodeMatrix(model), "a model without nodes has no root matrix");
        model.nodeMesh = {0};
        model.nodeMatrices.resize(1);
        require(!runtime::modelRootNodeMatrix(model), "a zero matrix is not a root matrix");
        // Recorded from models/ability_parkour_zip_line/ability_parkour_zip_line.bfnmdl (parseModel dump of node 0).
        model.nodeMatrices[0] = {0, 0, -1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, -27.432f, 0, 1};
        const auto root = runtime::modelRootNodeMatrix(model);
        require(root.has_value(), "node 0 matrix");

        runtime::ZiplineAnchor anchor;
        anchor.position[0] = 10;
        anchor.position[1] = 50;
        anchor.position[2] = 20;
        const Matrix placement = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 10, 50, 20, 1};
        const float top[3] = {0.5f, 27.5f, 0.0f};
        float out[3];
        point(runtime::ziplinePoleMatrix(*root, anchor, placement), top, out);
        require(near(out[0], 10) && std::fabs(out[1] - 50.068f) < 1e-3f && near(out[2], 19.5f), "untargeted pole uses the placement");

        anchor.target = 0;
        anchor.right[0] = 0; anchor.right[2] = -1;
        anchor.forward[0] = 1; anchor.forward[2] = 0;
        point(runtime::ziplinePoleMatrix(*root, anchor, placement), top, out);
        require(near(out[0], 9.5f) && std::fabs(out[1] - 50.068f) < 1e-3f && near(out[2], 20.0f), "linked pole is turned by the node rows");
    } catch (const std::exception& error) {
        std::cerr << "zipline-pole-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "zipline-pole-test: ok\n";
    return 0;
}
