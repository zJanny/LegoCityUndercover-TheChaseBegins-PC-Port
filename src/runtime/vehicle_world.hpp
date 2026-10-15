#pragma once
// The world steps of 0x30c768 for all vehicle bodies of a level at once (docs/re/vehicles-physics-collision.md): per step every body's update and
// floor pass, then the collision detection of every body against the static boxes and of every pair of vehicle bodies (compound against
// compound), then one solve of all manifolds (0x331814, the two-body rows of 0x332368) and the sleep rule. A sleeping body that a contact
// reaches is woken by the solver (0x3318a0 sets activation state 1) and moves from the next step on. No SDL dependency.
#include "runtime/vehicle_wheels.hpp"

#include <cstddef>
#include <map>
#include <optional>
#include <utility>
#include <vector>

namespace chase::runtime {

struct VehicleWorldBody {
    std::size_t id = 0;  // the broadphase pair order: the body with the lower id is the pair's first
    VehicleRigidBody* body = nullptr;
    DrivetrainState* drive = nullptr;
    bool* awake = nullptr;
    const VehicleRecord* record = nullptr;  // at the body's mass scale (vehicleRecordAtMassScale)
    BodyInputs controls;
    VehicleFloor* floor = nullptr;  // its `boxes` is the body's Bullet shape and static box contacts
    std::vector<WheelState>* lastWheels = nullptr;
};

// The boxes of a vehicle's Bullet shape from its entity box (modelCollisionBox): `shape` (vehicleShapeBox, the inertia) and `collision`
// (vehicleCollisionBox, the compound's child box and the collision points).
struct VehicleShapeBoxes {
    Vector3 shapeLow{}, shapeHigh{}, collisionLow{}, collisionHigh{};
};
VehicleShapeBoxes vehicleShapeBoxes(const VehicleRecord& record, const float entityCentre[3], const float entityHalf[3]);
// The mass scale body+0x84 of a vehicle: 2 while the player sits in it (the enter path 0x31fffc), else 1 (the leave path 0x3222f4, the parked
// worker in the oracle). STAND-IN VEH-S81: the port switches with its own boarding phase; which boarding step runs the enter and leave paths is not
// read.
float vehicleMassScale(bool playerSeated);

// STAND-IN VEH-S82: whether a vehicle is a dynamic body of the Bullet world. The link 0x156f74 adds the body (0x243300 with 1), makes it
// kinematic for owner flag 0x4 or 0x400 (0x2432d8) and removes it again (0x243300 with 0); the code that adds it later is not read. Oracle
// (drv-police-car-seated): the dynamic bodies are the trooper and the parked FP_Vehicle_03, 04 and 05; the traffic vehicles are kinematic and
// out of the world (collision flags 0x182, no broadphase handle). The port takes an enabled vehicle that is parked (state 4, or 10 for
// SpawnUnshovable) or already has a body.
bool vehicleHasDynamicBody(const VehicleRecord& record, bool hasBody, bool disabled);

// The box-box manifolds of vehicle pairs, keyed by the ids (first, second).
using VehiclePairManifolds = std::map<std::pair<std::size_t, std::size_t>, std::optional<ContactManifold>>;

// `steps` world steps (vehicleWorldSteps) of all bodies. Pairs where both bodies sleep are not processed (STAND-IN VEH-S62). The solver gets
// the static box manifolds of each awake body in list order, then the pair manifolds in id order (STAND-IN VEH-S60: the dispatcher's order).
void stepVehicleBodies(std::vector<VehicleWorldBody>& bodies, VehiclePairManifolds& pairs, const WheelRay& ray, int steps);

}
