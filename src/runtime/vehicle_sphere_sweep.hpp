#pragma once
// The sphere sweep of the vehicle body's floor pass 0x1c54f8 (docs/re/vehicles-physics-collision.md): a body with flag +0xc4 bit 0 carries a
// grid of collision spheres (set-up 0x33a4fc from the class values CollSpheres, CollRadius, ExtraColSpheres1 and the shape's box). Each pass the
// spheres are swept along the body's velocity against the wall cells that 0x1b6ef8 finds near the swept centre; 0x16788c tests one sphere
// against one cell's triangle and adds contacts to the pass's contact list. No SDL dependency.
#include "runtime/vehicle_collision.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace chase::runtime {

// Set-up of 0x33a4fc: a grid of counts[0] x counts[1] x counts[2] spheres of `radius` inside the box (an axis with a count above 1 is spread over
// the box minus two radii; an axis with a smaller count has its spheres at the box minimum for x and y and at the maximum for z), then one sphere at (0, 0.18, 0) of radius 0.2,
// and with `extraSpheres` two at (+-0.75, 0.1, 1.3) of radius 0.7. The reach is rigid+0x208 of 0x2d4490.
VehicleCollisionSpheres buildVehicleSpheres(const Vector3& low, const Vector3& high, const std::array<float, 3>& counts, float radius,
                                            bool extraSpheres);

// Result of 0x1b6ef8: the cells in the order the original lists them (one entry per qualifying wall edge, so a cell can appear twice) and for
// each cell index the edges it flagged (bit 0x80 << edge of cell word +6).
struct WallEdgeCells {
    std::vector<int> cells;
    std::vector<std::uint8_t> edges;
};

// 0x1b6ef8(reach, centre, startCell, mask, out): a breadth-first walk from `startCell` over the neighbours whose edge passes within `reach` of
// `centre` in the xz plane. An edge counts when its cell flag word has the bit `edgeBase << edge` (16 or 2 for the walls) and one of its end
// points lies less than 5 above `centre`.
WallEdgeCells findWallEdgeCells(const FloorMesh& mesh, const Vector3& centre, float reach, int startCell, unsigned edgeBase);

// 0x16788c for one sphere (`centre`, `radius`, moving with `velocity` over `dt`) against the triangle of `cell`. A sphere that already touches
// the triangle gives a contact at time 0, a sphere in front of the plane the contact at the plane crossing inside the triangle, or at the
// triangle's nearest edge point (a ray against the sphere). Contacts later than the earliest one of the list are dropped.
// STAND-IN VEH-S86: a plane crossing inside the triangle that is later than the list's earliest contact goes on to the edge test with no
// closest point in the original (0x1be3f0 leaves its output unwritten); the port gives no contact.
bool sweepSphereAgainstCell(const FloorMesh& mesh, int cell, float radius, float dt, const Vector3& centre, const Vector3& velocity,
                            FloorContacts& contacts);

// The sphere part of one pass of 0x1c54f8 (before the point sweeps): `driven` is body flag bit 3 (the walls are then the 2 << edge ones).
void sweepVehicleSpheres(const FloorMesh& mesh, const FloorTracker& tracker, const VehicleRigidBody& body, const VehicleCollisionSpheres& spheres,
                         bool driven, float dt, FloorContacts& contacts);

}
