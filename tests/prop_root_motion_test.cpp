// Root motion of placed props (src/runtime/prop_root_motion.cpp, docs/re/objects.md): the delta B * inverse(A) of 0x2327b4 and the node
// move of 0x16b06c, on made-up samples. No game data needed.
#include "assets/assets.hpp"
#include "runtime/prop_root_motion.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b) { return std::fabs(a - b) < 1e-5f; }

runtime::RootSample sample(float x, float y, float z, float yaw) {
    runtime::RootSample s;
    s.translation[0] = x;
    s.translation[1] = y;
    s.translation[2] = z;
    s.yaw = yaw;
    return s;
}

void deltas() {
    const runtime::RootSample still = runtime::rootDelta(sample(1, 2, 3, 0.5f), sample(1, 2, 3, 0.5f));
    require(near(still.translation[0], 0) && near(still.translation[1], 0) && near(still.translation[2], 0) && near(still.yaw, 0), "no motion");

    const runtime::RootSample forward = runtime::rootDelta(sample(0, 0, 0, 0), sample(0, 0.25f, 1, 0.2f));
    require(near(forward.translation[0], 0) && near(forward.translation[1], 0.25f) && near(forward.translation[2], 1) && near(forward.yaw, 0.2f),
            "without a previous turn the delta is the difference");

    const float quarter = 1.5707964f;
    const runtime::RootSample turned = runtime::rootDelta(sample(0, 0, 0, quarter), sample(1, 0, 0, quarter));
    require(near(turned.translation[0], 0) && near(turned.translation[2], 1) && near(turned.yaw, 0),
            "a step along x after a quarter turn is a step along the previous frame's z (Ry rows (c,0,-s), (s,0,c))");
}

void nodeMoves() {
    const assets::Matrix identity = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 10, 20, 30, 1};
    const assets::Matrix stepped = runtime::applyRootDelta(identity, sample(0, -0.5f, 2, 0));
    require(near(stepped[12], 10) && near(stepped[13], 19.5f) && near(stepped[14], 32) && near(stepped[0], 1) && near(stepped[10], 1),
            "an unturned node moves by the delta");

    assets::Placement placement;
    placement.orientation[1] = 90.0f;
    placement.position[0] = 1;
    const assets::Matrix facing = assets::placementMatrix(placement);
    const assets::Matrix moved = runtime::applyRootDelta(facing, sample(0, 0, 1, 0));
    require(near(moved[12], 1 + facing[8]) && near(moved[14], facing[10]), "the delta goes through the node's rotation (third row for z)");

    const assets::Matrix turned = runtime::applyRootDelta(identity, sample(1, 0, 0, 0.5f));
    require(near(turned[12], 11) && near(turned[0], std::cos(0.5f)) && near(turned[2], -std::sin(0.5f)) && near(turned[8], std::sin(0.5f)),
            "the turn applies after the move: the translation uses the rotation before the turn");
    assets::Placement yawed;
    yawed.orientation[1] = 0.5f * 57.29578f;
    const assets::Matrix expected = assets::placementMatrix(yawed);
    for (int i : {0, 2, 8, 10}) require(near(turned[std::size_t(i)], expected[std::size_t(i)]), "the turn matches placementMatrix's yaw");
}

void noExtraCurve() {
    assets::Animation animation;
    require(!runtime::rootSample(animation, 3.0).has_value(), "a clip without the extra curve has no root motion");
}
}

int main() {
    try {
        deltas();
        nodeMoves();
        noExtraCurve();
    } catch (const std::exception& error) {
        std::cerr << "prop-root-motion: " << error.what() << '\n';
        return 1;
    }
    std::cout << "prop-root-motion: passed\n";
    return 0;
}
