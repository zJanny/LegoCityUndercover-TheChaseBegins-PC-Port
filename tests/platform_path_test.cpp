// The path of a platform (docs/re/props-rocking-platforms.md): the arc length parameter 0x22a4c0, the spline position, the squad 0x270c60 with the
// slerp 0x21a600, the quaternion rows 0x248870, the forward turn 0x237a0c, the platform update on a path and the carry of a turning node, with
// made-up paths. Expected values are hand evaluations of that code (20261027-mission13-A, 2026-10-27, llvm-objdump of 0x2807c8..0x281418,
// 0x22a4c0..0x22a670, 0x270c60..0x270db4, 0x21a600..0x21a724, 0x248870..0x24890c, 0x237a0c..0x237a78, 0x23b960, 0x257ea0, 0x1ff9cc).
#include "runtime/platform_node_move.hpp"
#include "runtime/platform_path.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b, float tolerance) { return std::fabs(a - b) <= tolerance; }

bool nearRows(const PathRotation& a, const PathRotation& b, float tolerance) {
    for (std::size_t k = 0; k < a.size(); ++k)
        if (!near(a[k], b[k], tolerance)) return false;
    return true;
}

constexpr float kHalf = 0.70710678f;

assets::LevelPath line() {
    assets::LevelPath path;
    path.points = {{0, 0, 0}, {2, 0, 0}, {4, 0, 0}};
    return path;
}

void slerpModes() {
    const PathQuaternion identity{0, 0, 0, 1}, turned{0, kHalf, 0, -kHalf};
    const PathQuaternion shortest = slerpPathQuaternion(identity, turned, 0.5f, 0);
    require(near(shortest[1], -0.382683f, 1e-5f) && near(shortest[3], 0.923880f, 1e-5f), "mode 0 negates the far quaternion (dot -0.707)");
    const PathQuaternion kept = slerpPathQuaternion(identity, turned, 0.5f, 2);
    require(near(kept[1], 0.923880f, 1e-5f) && near(kept[3], 0.382683f, 1e-5f), "mode 2 keeps the sign: the long way over 135 degrees");
    const PathQuaternion close = slerpPathQuaternion(identity, identity, 0.25f, 2);
    require(close[3] == 1.0f, "a dot above 0x3f7fff58 blends linearly");
}

void quaternionRows() {
    const PathRotation rows = pathQuaternionRotation({0, kHalf, 0, kHalf});
    require(nearRows(rows, {0, 0, 1, 0, 1, 0, -1, 0, 0}, 1e-6f), "0x248870: a quarter turn about y has right (0, 0, 1) and forward (-1, 0, 0)");
    require(nearRows(pathForwardRotation({1, 0, 0}), {0, 0, -1, 0, 1, 0, 1, 0, 0}, 1e-6f), "0x237a0c: right (z, 0, -x), up forward x right");
    require(nearRows(pathForwardRotation({0, 1, 0}), {0, 0, 1, 1, 0, 0, 0, 1, 0}, 1e-6f), "0x237a0c: a forward above 0.98 in y takes right (0, -z, y)");
    require(nearRows(normalisedPathRotation({2, 0, 0, 0, 0, 0, 0, 0, 3}), {1, 0, 0, 0, 0, 1, 0, 0, 1}, 0.0f), "0x257ea0: a zero row becomes (0, 0, 1)");
}

void squadOnClosedPath() {
    assets::LevelPath path;
    path.closed = true;
    path.points = {{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}};
    const PathQuaternion identity{0, 0, 0, 1};
    path.orientations = {identity, {0, kHalf, 0, kHalf}, identity, identity, {0, 1, 0, 0}, identity, identity, identity};
    path.orientationControls = path.orientations;
    const PathQuaternion half = pathSquadOrientation(path, 0.5f);
    require(near(half[1], 0.382683f, 1e-5f) && near(half[3], 0.923880f, 1e-5f), "between equal tables the squad is the slerp: 45 degrees at f 0.5");
    const PathQuaternion last = pathSquadOrientation(path, 3.5f);
    require(near(last[1], kHalf, 1e-5f) && near(last[3], kHalf, 1e-5f), "a closed path wraps i but not j: point 3 blends to the table's entry 4");
    const PathQuaternion end = pathSquadOrientation(path, 4.0f);
    require(near(end[3], 1.0f, 1e-6f), "p = count wraps i to 0");
    const PathQuaternion none = pathSquadOrientation(line(), 1.0f);
    require(none[3] == 1.0f && none[1] == 0.0f, "a path without quaternions gives the identity");
}

void poseOnALine() {
    assets::LevelPath square;
    square.closed = true;
    square.points = {{0, 0, 0}, {2, 0, 0}, {2, 0, 2}, {0, 0, 2}};
    const std::shared_ptr<const PlatformPath> constant = platformPathOf(square, true);
    require(constant->lengths.size() == 5 && near(constant->lengths[4], constant->lengths[1] * 4.0f, 1e-4f),
            "20 steps per segment, a closed path has count + 1 entries, the four sides are equal");
    const PlatformPathPose middle = platformPathPose(*constant, 0.5f, true, 0);
    require(near(middle.position[0], 2.0f, 1e-4f) && near(middle.position[2], 2.0f, 1e-4f) && !middle.rotation,
            "ConstantSpeed: half the arc length of the square is point 2; no turn without orient");
    require(near(platformPathPose(*constant, 1.0f, true, 0).position[0], 0.0f, 1e-6f), "0x22a4c0 returns count for 1.0: the closed path's start again");
    const PlatformPathPose counted = platformPathPose(*platformPathOf(line(), false), 0.5f, false, 1);
    require(near(counted.position[0], 3.125f, 1e-5f), "without ConstantSpeed the parameter is count x shaped (1.5)");
    require(counted.rotation && nearRows(*counted.rotation, {0, 0, -1, 0, 1, 0, 1, 0, 0}, 1e-5f), "orient turns the node along the tangent");
}

void updateMovesAPathPlatform() {
    PlatformRecord record;
    record.motion.type = 1;
    record.motion.orient = 1;
    record.speed = 0.25f;
    record.motion.path = true;
    record.motion.track = platformPathOf(line(), false);
    platformMessage(record, 0xff);
    require(!platformUpdate(record, 1.0f / 30.0f) && record.state == kPlatformMovingState, "the trigger's update takes state 2");
    const std::optional<PlatformVector> node = platformUpdate(record, 1.0f / 30.0f);
    require(node && near((*node)[0], 1.453125f, 1e-5f) && near(record.value, 0.25f, 1e-6f), "the node goes to the spline at 3 x 0.25");
    require(record.rotation && nearRows(*record.rotation, {0, 0, -1, 0, 1, 0, 1, 0, 0}, 1e-5f), "and turns along the tangent");
}

void carryTurnsChildren() {
    NodeCarry carry;
    carry.setPlacement("platform", {10, 0, 0});
    carry.setPlacement("rider", {11, 0, 0});
    carry.setParent("rider", "platform");
    carry.place("platform", pathQuaternionRotation({0, kHalf, 0, kHalf}), {10, 0, -10});
    require(nearRows(carry.rotation("rider"), {0, 0, 1, 0, 1, 0, -1, 0, 0}, 1e-6f), "the child turns with the node");
    const PlatformVector offset = carry.offset("rider");
    require(near(offset[0], -1.0f, 1e-5f) && near(offset[2], 1.0f, 1e-5f) && near(carry.offset("platform")[0], 0.0f, 1e-5f),
            "a quarter turn about the platform takes the rider from (11, 0, 0) to (10, 0, 1)");
    carry.setParent("rider", "");
    carry.place("platform", kIdentityPathRotation, {0, 2, 0});
    require(carry.offset("platform")[1] == 2.0f && near(carry.offset("rider")[2], 1.0f, 1e-5f), "a detached node stays where it was carried");
    carry.setParent("rider", "platform");
    carry.place("platform", kIdentityPathRotation, {0, 3, 0});
    require(near(carry.offset("rider")[1], 1.0f, 1e-6f) && near(carry.offset("rider")[2], 1.0f, 1e-5f), "attached again, it keeps its new place under the node");

    NodeCarry crane;
    crane.setPlacement("platform", {0, 0, 0});
    crane.setPlacement("girder", {0, 0, 0});
    crane.setPlacement("crow", {0, 0, 0});
    crane.setParent("girder", "platform");
    crane.setParent("crow", "girder");
    crane.move("crow", {5, 0, 0});
    crane.move("platform", {0, -1, 0});
    require(crane.offset("crow")[0] == 5.0f && crane.offset("crow")[1] == -1.0f, "a child that moved itself keeps its move when its parent moves");
    crane.place("platform", pathQuaternionRotation({0, kHalf, 0, kHalf}), {0, -1, 0});
    const PlatformVector crow = crane.offset("crow");
    require(near(crow[0], 0.0f, 1e-5f) && near(crow[1], -1.0f, 1e-6f) && near(crow[2], 5.0f, 1e-5f), "and turns with its parent from that place: (5, 0, 0) to (0, 0, 5)");
}

}

int main() {
    try {
        slerpModes();
        quaternionRows();
        squadOnClosedPath();
        poseOnALine();
        updateMovesAPathPlatform();
        carryTurnsChildren();
    } catch (const std::exception& error) {
        std::cerr << "platform-path: " << error.what() << '\n';
        return 1;
    }
    std::cout << "platform-path: ok\n";
    return 0;
}
