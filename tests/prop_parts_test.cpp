// Per-node transforms of placed props (src/runtime/prop_nodes.cpp, src/runtime/mesh_bouncer.cpp). The geometry is the closed form of the
// code transcribed from game.arm.elf (0x19c9e0 flight, 0x21a600 slerp, 0x248618 / 0x248870 quaternion conversion, 0x260728 update of
// dummymeshbouncer); the parameters are the values of the class files of props/map_waypoint (levels/props/map_waypoint.blvl Badge child). No oracle
// number exists for these motions yet (docs/re/buildit.md, docs/re/objective-hud.md).
#include "runtime/mesh_bouncer.hpp"
#include "runtime/prop_nodes.hpp"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {
using namespace chase::runtime;

int failures = 0;

void require(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

bool near(float a, float b, float tolerance = 1e-4f) { return std::fabs(a - b) <= tolerance; }

bool sameMatrix(const NodeMatrix& a, const NodeMatrix& b, float tolerance = 1e-4f) {
    for (std::size_t i = 0; i < 16; ++i)
        if (!near(a[i], b[i], tolerance)) return false;
    return true;
}

void conversionRoundTrip() {
    // Bit1, Bit2 and Bit3 of models/prop_donut_truck_cabin_bi (model block +0x20, pxt dump of the extracted archive): one matrix per
    // branch of 0x248618 (positive trace, y largest, negative trace with x largest).
    const NodeMatrix matrices[] = {
        {2.22045e-16f, -1, -0.0f, -0.0f, 1, 2.22045e-16f, 0, 0, -0.0f, 0, 1, 0, -0.510532f, 1.04501f, -1.53979f, 1},
        {-0.61573f, -0.0f, -0.787957f, -0.0f, -0.0f, 1, 0, 0, 0.787957f, -0.0f, -0.61573f, 0, -0.792255f, -1.3775f, 1.59303f, 1},
        {-0.617993f, -0.0332854f, -0.785479f, -0.0f, -0.370152f, 0.893756f, 0.253352f, 0, 0.693594f, 0.447316f, -0.564655f, 0, -2.11755f, -1.97546f, 2.51409f, 1},
    };
    for (const NodeMatrix& m : matrices) require(sameMatrix(nodeMatrixOf(nodePoseOf(m)), m, 2e-5f), "matrix to quaternion and back");
}

void slerpHalfway() {
    const std::array<float, 4> identity{0, 0, 0, 1};
    const float half = std::sqrt(0.5f);
    const std::array<float, 4> quarterTurnAboutZ{0, 0, half, half};
    const auto middle = nodeSlerp(identity, quarterTurnAboutZ, 0.5f);
    const float angle = float(M_PI) / 8;
    require(near(middle[2], std::sin(angle)) && near(middle[3], std::cos(angle)) && near(middle[0], 0) && near(middle[1], 0), "slerp halfway is 45 degrees");
    const std::array<float, 4> flipped{0, 0, -half, -half};
    const auto shortArc = nodeSlerp(identity, flipped, 0.5f);
    require(near(shortArc[2], std::sin(angle)) && near(shortArc[3], std::cos(angle)), "slerp takes the short way to the negated quaternion");
}

void flightHop() {
    NodePose start, end;
    end.position = {4, 0, 3};
    require(sameMatrix(nodeFlight(start, end, 0.0f), nodeMatrixOf(start)), "t = 0 is the start pose");
    require(sameMatrix(nodeFlight(start, end, 1.0f), nodeMatrixOf(end)), "t = 1 is the end pose");
    const NodeMatrix middle = nodeFlight(start, end, 0.5f);
    require(near(middle[12], 2.0f) && near(middle[14], 1.5f), "position is the linear mix");
    require(near(middle[13], 0.2f * 1.0f * 7.0f), "the hop at t = 0.5 is 0.2 * (|dx| + |dz|)");
    require(near(nodeFlight(start, end, 0.25f)[13], 0.2f * 0.75f * 7.0f), "the hop at t = 0.25 is 0.2 * 4t(1 - t) * (|dx| + |dz|)");
}

void bouncerBadge() {
    NodeMatrix base{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0.3965f, 0, 1};
    MeshBouncerParams small;
    small.revsPerMin = 15.0f;
    small.bouncePerMin = 50.0f;
    small.bounceAmount = 0.08f;
    MeshBouncer bouncer(base, small);
    require(sameMatrix(bouncer.matrix(), base), "before the first update the badge stands as placed");
    for (int i = 0; i < 3; ++i) bouncer.update(0.1f);
    require(near(bouncer.matrix()[13], 0.3965f + 0.08f, 2e-4f), "after 0.3 s the bounce of 50 per minute is at its top");
    for (int i = 0; i < 7; ++i) bouncer.update(0.1f);
    const NodeMatrix quarterTurn = bouncer.matrix();
    require(near(quarterTurn[0], 0.0f, 2e-4f) && near(quarterTurn[2], -1.0f, 2e-4f) && near(quarterTurn[8], 1.0f, 2e-4f) && near(quarterTurn[10], 0.0f, 2e-4f),
            "after 1 s the badge has turned a quarter (15 revolutions per minute)");
    MeshBouncer wrapping(base, small);
    wrapping.update(59.9f);
    wrapping.update(0.2f);
    require(near(wrapping.clock(), 0.1f, 1e-3f), "the clock wraps at 60 s");
    MeshBouncer idle(base, MeshBouncerParams{});
    idle.update(5.0f);
    require(sameMatrix(idle.matrix(), base), "without parameters the child does not move");
}

}

int main() {
    conversionRoundTrip();
    slerpHalfway();
    flightHop();
    bouncerBadge();
    if (failures) return EXIT_FAILURE;
    std::cout << "prop-parts-test: ok\n";
    return EXIT_SUCCESS;
}
