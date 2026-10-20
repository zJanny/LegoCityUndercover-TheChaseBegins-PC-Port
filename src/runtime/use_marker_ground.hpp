#pragma once
// Where a use marker stands (placement 0x1db800, docs/re/use-spot.md): from the zone centre in the entity frame a segment 5 m down the node's up
// axis; the hit is the marker position, without a hit the zone centre. No SDL dependency.
#include "runtime/camera_reset_probe.hpp"
#include "runtime/collision.hpp"

#include <array>

namespace chase::runtime {

constexpr float kUseMarkerGroundSearch = 5.0f;

// STAND-IN SCRIPT-S249: the hit is the nearest one of the camera's query (CameraResetProbe, as DETECT-S9); 0x1db800 gathers the objects of the
// box (0x237f30, up to 50), drops some kinds and tests with 0x204764 / 0x22a7a4, stepping up 0.5 m past objects; none of that is ported.
std::array<float, 3> useMarkerGround(const CollisionWorld& world, const std::array<float, 16>& matrix, const float zoneCentre[3]);

}
