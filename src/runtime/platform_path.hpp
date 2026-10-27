#pragma once
// The path of a platform (`Platform_Path`, R+0x50; docs/re/props-rocking-platforms.md): the node pose that the update 0x2807c8 takes from it
// (0x280f4c..0x281024): the arc length parameter 0x22a4c0 of ConstantSpeed platforms, the position and tangent of the path's sampler, the
// squad 0x270c60 of the path's quaternions and, for a path without quaternions, the turn of `orient` along the tangent (0x237a0c).
// No SDL dependency.
#include "assets/assets.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace chase::runtime {

using PathQuaternion = std::array<float, 4>;  // x, y, z, w
using PathRotation = std::array<float, 9>;    // rows right, up, forward (matrix floats 0..2, 4..6, 8..10)

constexpr PathRotation kIdentityPathRotation{1, 0, 0, 0, 1, 0, 0, 0, 1};
constexpr unsigned kPlatformPathLengthSteps = 20;  // 0x22a4c0 sums slot 1 of the sampler with 20 steps per segment (mov r1, #20 at 0x22a584)

// 0x21a600(out, a, b, mode, t): the dot of a and b; mode 2 keeps the sign, another mode negates b's weight and the dot when (dot < 0) differs
// from the mode; weights 1 - t and t when the dot's bits exceed 0x3f7fff58, else sin((1 - t) theta) / sin theta and sin(t theta) / sin theta.
PathQuaternion slerpPathQuaternion(const PathQuaternion& a, const PathQuaternion& b, float t, int mode);
// 0x270c60(path, out, p): identity without quaternions; an open path clamps p to 0 .. count - 1; i = floor(p), j = i + 1, f = p - i; a closed path
// with at least 3 points wraps i (signed remainder plus count) and keeps j, any other path limits j to count - 1. The result is
// slerp(slerp(Q[i], Q[j], f), slerp(C[i], C[j], f), 2 f (1 - f)) with mode 2, normalised (0x1ff9cc); Q are the path's quaternions, C the controls.
PathQuaternion pathSquadOrientation(const assets::LevelPath& path, float position);
// 0x248870: the rotation rows of a quaternion.
PathRotation pathQuaternionRotation(const PathQuaternion& q);
// 0x237a0c on a matrix whose forward row is `forward`: right = (z, 0, -x), or (0, -z, y) when |y| is above 0.98 (bits 0x3f7ae148), normalised
// (0x257ea0), up = forward x right (0x23b960).
PathRotation pathForwardRotation(const std::array<float, 3>& forward);
// The rows of a then b (a point p goes to p a b); `transposed` takes a's transpose (its inverse for a rotation).
PathRotation multiplyPathRotation(const PathRotation& a, const PathRotation& b, bool transposed = false);
PathRotation transposedPathRotation(const PathRotation& rows);
// p R for a point p.
std::array<float, 3> rotatePathPoint(const std::array<float, 3>& p, const PathRotation& rows);
// 0x257ea0 on each row: (0, 0, 1) for a length whose bits are at most 0x34000000.
PathRotation normalisedPathRotation(const PathRotation& rows);

// The path of a platform with the arc length table 0x22a4c0 builds on every call (the table is built once here; the same values).
struct PlatformPath {
    assets::LevelPath path;
    std::vector<float> lengths;
};
// Throws for a ConstantSpeed platform on a path whose type has no ported length sampler (slot 1 of type 1, 0x293494; no shipped path platform has one).
std::shared_ptr<const PlatformPath> platformPathOf(const assets::LevelPath& path, bool constantSpeed);

struct PlatformPathPose {
    std::array<float, 3> position{};
    std::optional<PathRotation> rotation;  // the node's new rotation rows, before the update normalises them
};
// 0x280f4c..0x281024 for the shaped value: the sampler's parameter is 0x22a4c0(shaped) with ConstantSpeed (R+0xb8 bit 1), else count x shaped; the
// rotation is the squad at count x shaped when the path has quaternions, else the forward turn of the sampler's tangent when `orient` is set.
PlatformPathPose platformPathPose(const PlatformPath& track, float shaped, bool constantSpeed, std::uint8_t orient);

}
