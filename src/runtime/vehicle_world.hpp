#pragma once
// The world steps of 0x30c768 for all vehicle bodies of a level at once (docs/re/vehicles-physics-collision.md): per step every body's update and
// floor pass, then the collision detection of every body against the static boxes and of every pair of vehicle bodies (compound against
// compound), then one solve of all manifolds (0x331814, the two-body rows of 0x332368) and the sleep rule. A sleeping body that a contact
// reaches is woken by the solver (0x3318a0 sets activation state 1) and moves from the next step on. No SDL dependency.
#include "runtime/vehicle_damage.hpp"
#include "runtime/vehicle_wheels.hpp"

#include <cstddef>
#include <functional>
#include <map>
#include <optional>
#include <utility>
#include <vector>

namespace chase::runtime {

// The contact message 0x80000002 (0x271764 for Bullet manifold points, 0x181e68 for the floor pass) for one body: the closing speed of the contact
// (0x19b5c4) and what the other body is.
using VehicleImpactSink = std::function<void(float speed, const ContactOther& other)>;

struct VehicleWorldBody {
    std::size_t id = 0;  // the broadphase pair order: the body with the lower id is the pair's first
    VehicleRigidBody* body = nullptr;
    DrivetrainState* drive = nullptr;
    bool* awake = nullptr;
    const VehicleRecord* record = nullptr;  // at the body's mass scale (vehicleRecordAtMassScale)
    BodyInputs controls;
    VehicleFloor* floor = nullptr;  // its `boxes` is the body's Bullet shape and static box contacts
    std::vector<WheelState>* lastWheels = nullptr;
    VehicleImpactSink impact = nullptr;
    bool destroyed = false;  // entity word +0xc bit 0x10, what the other side of a pair contact sees
};

// The boxes of a vehicle's Bullet shape from its entity box (modelCollisionBox): `shape` (vehicleShapeBox, the inertia) and `collision`
// (the same box: the compound's child box and the collision points).
struct VehicleShapeBoxes {
    Vector3 shapeLow{}, shapeHigh{}, collisionLow{}, collisionHigh{};
};
VehicleShapeBoxes vehicleShapeBoxes(const VehicleRecord& record, const float entityCentre[3], const float entityHalf[3]);
// The mass scale body+0x84 of a vehicle: 2 from the begin of the player's Jump In (state 0x11a, 0x31fffc) until the begin of his Jump Out (state
// 0x11b, 0x3222f4 sets 1), else 1 (the parked worker in the oracle). The value 2 is the global 0x3745b4 as the oracle held it (the data image
// holds 0 there; its writer is not read).
float vehicleMassScale(bool playerVehicle);

// STAND-IN VEH-S82: whether a vehicle is a dynamic body of the Bullet world. The link 0x156f74 adds the body (0x243300 with 1), makes it
// kinematic for owner flag 0x4 or 0x400 (0x2432d8) and removes it again (0x243300 with 0). The vehicle handler adds it again on message
// 0x80000004 (0x17a618, sent when the entity joins the active list in 0x173534) unless the entity word +0x10 has bit 0, and removes it on
// message 0x80000003 (0x17a648, sent by the removal at 0x22d4a4); what activates a given vehicle is not read. Oracle
// (drv-police-car-seated): the dynamic bodies are the trooper and the parked FP_Vehicle_03, 04 and 05; the traffic vehicles are kinematic and
// out of the world (collision flags 0x182, no broadphase handle). The port takes an enabled vehicle that is parked (state 4, or 10 for
// SpawnUnshovable) or already has a body.
bool vehicleHasDynamicBody(const VehicleRecord& record, bool hasBody, bool disabled);

// 0x271764 for a point of a vehicle pair (body A the pair's second vehicle, body B its first): when both bodies have flag bit 0 and more than one
// wheel on the ground (+0xda0), the flag word gets bit 0 for a driven A and bit 1 for a driven B (the solver scales that body's angular part, 0x331ac0),
// and a driven body against a BumpCollision vehicle (body flag +0xc4 bit 0x10000) makes the point's friction 0.2 and its restitution 1.1.
// STAND-IN VEH-S93: the body flags +0xc4 bit 7 (which refuses the rule) and bit 2 (which replaces the wheel count) are taken as clear; a body that has
// no wheel states yet (asleep since the start) counts as having its wheels on the ground, as the oracle trace of the parked worker shows.
void applyPairContactRule(ManifoldPoint& point, const VehicleWorldBody& a, const VehicleWorldBody& b);

// The box-box manifolds of vehicle pairs, keyed by the ids (first, second).
using VehiclePairManifolds = std::map<std::pair<std::size_t, std::size_t>, std::optional<ContactManifold>>;

// `steps` world steps (vehicleWorldSteps) of all bodies. Pairs where both bodies sleep are not processed (STAND-IN VEH-S62). The solver gets
// the static box manifolds of each awake body in list order, then the pair manifolds in id order (STAND-IN VEH-S60: the dispatcher's order).
void stepVehicleBodies(std::vector<VehicleWorldBody>& bodies, VehiclePairManifolds& pairs, const WheelRay& ray, int steps);

}
