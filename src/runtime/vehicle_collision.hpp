#pragma once
// The vehicle body's collision with the level floor (docs/re/vehicles-physics.md): the world step integrates a vehicle body (rigid flag 0x100)
// in 0x2d46d4, which hands it to 0x1c54f8 when its broadphase mask holds the static group. 0x1c54f8 tests eight body points against the floor
// graph (assets::FloorGraph, walk 0x23b5e8), integrates the body up to the earliest contact, answers each contact with an impulse (0x19c0a8)
// and repeats for the rest of the step, at most kVehicleCollisionPasses times; then it pushes points that lie below the floor out
// (0x1b79ac). The cell the body stands on is tracked by 0x20c0e4. No SDL dependency.
#include "runtime/floor_walk.hpp"
#include "runtime/vehicle.hpp"
#include "runtime/vehicle_body.hpp"

#include <array>
#include <vector>

namespace chase::runtime {

constexpr int kVehicleCollisionPasses = 6;
// 0x1c54f8: the contact time is shortened by this factor before the body moves up to it.
constexpr float kVehicleContactTimeScale = 0.9999f;
// 0x19c0a8 / 0x1b79ac: the approach speed counts from this much below zero.
constexpr float kVehicleContactSlop = 0.01f;
// 0x19c0a8: bounce = (powf(1 - min(-0.1125 * approach, 1), 6) * 0.94 + 1.05).
constexpr float kVehicleBounceSpeedScale = 0.1125f;
constexpr float kVehicleBouncePower = 6.0f;
constexpr float kVehicleBounceScale = 0.94f;
constexpr float kVehicleBounceBase = 1.05f;
// 0x19c0a8: friction limit = surface * normal impulse * 1.1, times 0.3 while the body's up axis has y above 0.1.
constexpr float kVehicleContactFriction = 1.1f;
constexpr float kVehicleUprightFriction = 0.3f;
constexpr float kVehicleUprightLimit = 0.1f;
// 0x1b79ac: friction limit of the push-out = surface * depth * 9000.1.
constexpr float kVehiclePushFriction = 9000.1f;
// 0x15c4a4: a point that the sweep follows through more cells than this gives no contact.
constexpr int kFloorSweepCells = 100;
// Surface bytes with their own rules: '$' (0x24) is never a contact of 0x15c4a4 and gives 0x19c0a8 the normal (0, 1, 0); '%' (0x25) is a wall
// for a body with flag +0xc4 bit 6.
constexpr std::uint8_t kFloorSurfaceSoft = 0x24;
constexpr std::uint8_t kFloorSurfaceWall = 0x25;

// The eight points at body+0xdc (0x243398) from the box of the collision shape in body axes: the four bottom corners, then the top corners with
// x and z times 0.9. With body flag +0xc4 bit 0 the bottom is at most the mount height of wheels 0 and 2.
std::array<Vector3, 8> vehicleProbePoints(const Vector3& low, const Vector3& high, const std::vector<VehicleWheel>& wheels);
// STAND-IN VEH-S50: the box comes from the Bullet collision shape (a compound with one box child, rigid +0xc0, getAabb), which is built by code
// that is not read; these are the local AABBs read in the oracle for the trooper and the donut van (the trooper's for every other class).
void vehicleCollisionBoxStandIn(const std::string& className, Vector3& low, Vector3& high);

struct FloorContact {
    float time = 0;               // fraction of the remaining step
    Vector3 point = {0, 0, 0};
    Vector3 normal = {0, 1, 0};
    int cell = -1;
    float depth = 0;              // how far the point lies below the floor
    std::uint16_t flags = 0;      // bit 2 (0x181e68): no vertical impulse
};
// The contact list of 0x1c54f8 (0x34-byte records): the first record's time is the earliest contact time, 1 while the list is empty.
struct FloorContacts {
    std::vector<FloorContact> items;
    float earliest = 1.0f;
};

// 0x15c4a4 for one body point at `point` moving with `velocity` over `dt`, starting in `cell` (updated as the sweep crosses cells): a point
// above the floor is swept along its velocity through the cells; it hits the floor plane inside the cell or a wall edge (an edge without a
// neighbour; with `percentWalls` also an edge into a '%' cell). A point below the floor that moves into it is a contact at time 0 with its
// depth. Keeps only the earliest contacts. Returns the contact time or -1. STAND-IN VEH-S51: the second cell search 0x164328 for a point more
// than 0.1 below a cell with flag bit 0 is not read; the point keeps its cell.
float sweepFloorPoint(const FloorMesh& mesh, float dt, int& cell, const Vector3& point, const Vector3& velocity, FloorContacts& contacts,
                      bool percentWalls);

// 0x19c0a8: the impulse of one contact (restitution from the approach speed, friction from the surface) at the contact point.
void floorContactImpulse(VehicleRigidBody& body, const FloorContact& contact, std::uint8_t surface, float surfaceFriction, float upY);
// 0x1b79ac: the push-out of a point below the floor: impulse along the normal of 2 Mass times the depth, with friction.
void floorPushImpulse(VehicleRigidBody& body, const FloorContact& contact, std::uint8_t surface, float surfaceFriction, float massTimesTwo);

// The cell tracker of the vehicle body (+0x8c, last position +0x1a4).
struct FloorTracker {
    int cell = -1;
    Vector3 last = {0, 0, 0};
};
// 0x15f82c: whether the point lies inside the cell's triangle in the xz plane.
bool insideFloorCell(const FloorMesh& mesh, int cell, const Vector3& point);
// 0x1edaac: the search for the cell of a point (bisection on the distance to each cell's first corner, then outwards from there).
int locateFloorCell(const FloorMesh& mesh, const Vector3& point);
// 0x20c0e4 (called by 0x33aea8 every step): without a cell the search, else the walk from the last position (failing at the graph's edge).
void trackFloorCell(const FloorMesh& mesh, FloorTracker& tracker, const Vector3& position);

struct VehicleCollisionBody {
    std::array<Vector3, 8> points{};
    float massTimesTwo = 0;     // body+0xc8
    bool percentWalls = false;  // body flag +0xc4 bit 6
    bool spheres = true;        // body flag +0xc4 bit 0 (set in the trooper and the donut van: +0xc4 = 0x800b)
    std::uint16_t surface = 0;  // body+0x9c, the body's own surface for 0x181e68 (0 in the trooper)
};
// 0x181e68 without its message: contact flag bit 2 for a body with flag bit 0 when its surface or the cell's is 0xf or 1 and the contact normal
// is steeper than 45 degrees (|normal y| below 0x3f350481).
std::uint16_t floorContactFlags(const VehicleCollisionBody& shape, std::uint8_t cellSurface, const FloorContact& contact);

// 0x1c54f8 for one world step after the forces are applied (0x2d46d4): without a tracked cell the body does not move. Returns whether a
// contact was answered. STAND-IN VEH-S52: the sphere sweep of the CollSpheres against flagged cell edges (0x1b6ef8, 0x16788c, body flag bit 0)
// and the contact messages of 0x181e68 to the entity (a reply 2000 drops the contact) are not ported.
bool collideVehicleWithFloor(VehicleRigidBody& body, const FloorMesh& mesh, const VehicleCollisionBody& shape, FloorTracker& tracker,
                             float surfaceFriction, float linearDamping, float angularDamping, float dt);

}
