#pragma once
// Wheels and suspension of the vehicle physics body (docs/re/vehicles-physics.md): the wheel position 0x3396cc, the ground ray of 0x33c718 and
// the spring and damper force of 0x339030, applied to the rigid body at the wheel, and the vehicle step of 0x33aea8 that adds the engine and
// the tyres of vehicle_drivetrain.hpp. No SDL dependency.
#include "runtime/collision.hpp"
#include "runtime/vehicle.hpp"
#include "runtime/vehicle_body.hpp"
#include "runtime/vehicle_box_contact.hpp"
#include "runtime/vehicle_collision.hpp"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

// The ray starts this far above the wheel centre along the body's up axis (w+0x150, set by the wheel constructor 0x339bec).
constexpr float kWheelRayLift = 0.99f;
// 0x3396cc: the ray reaches max(travel - radius, kWheelRayMinimumReach) + radius below the wheel centre (w+0x118).
constexpr float kWheelRayMinimumReach = 0.001f;
// 0x339030 multiplies the stiffness by kSuspensionStiffnessScale * kSuspensionForceFactor * (setting +4 of 0x376e58) and the damping by
// kSuspensionDampingScale * kSuspensionDampingFactor * (setting +8). The settings are 0.75 and 0.7 in the oracle (written at run time; the
// writer is not read).
constexpr float kSuspensionStiffnessScale = 4000.0f;
constexpr float kSuspensionForceFactor = 3.0f;
constexpr float kSuspensionDampingScale = 2000.0f;
constexpr float kSuspensionDampingFactor = 0.9f;
constexpr float kSuspensionStiffnessSetting = 0.75f;
constexpr float kSuspensionDampingSetting = 0.7f;

// The two 11-entry tables at 0x376eac (stiffness by compression) and 0x376ed8 (damping by compression), filled at run time; values read in
// the oracle. Linear between the entries (0x1e12e8).
float suspensionStiffnessCurve(float compression);
float suspensionDampingCurve(float compression);

float wheelRayLength(const WheelSuspension& wheel);

// 0x339030 with contact: distance from the wheel centre to the hit, the hit point's velocity along the ground normal, WheelSuspensionScale.
// The force pushes along the normal and is never negative.
float suspensionForce(const WheelSuspension& wheel, float distance, float normalSpeed, float suspensionScale);

struct WheelHit {
    Vector3 point = {0, 0, 0};
    Vector3 normal = {0, 1, 0};
};
// Closest hit of the segment from `from` to `to`, nothing without one.
using WheelRay = std::function<std::optional<WheelHit>(const Vector3& from, const Vector3& to)>;

// The ray of 0x33c718. 0x155888 accepts a triangle only when the start is on the front of its plane (record normal and d) and the end is
// behind it. STAND-IN VEH-S31: 0x23b5e8 picks the triangle and 0x155888's inside test are not read; here it is the closest triangle of the
// player's query layer that the segment crosses this way, and other bodies (0x1ea6a0) are not tested.
std::optional<WheelHit> castWheelRay(const CollisionWorld& world, const Vector3& from, const Vector3& to, const std::function<bool(const CollisionWorld::Entry&)>& skip = {});

struct WheelState {
    Vector3 centre = {0, 0, 0};  // w+0x100: the wheel centre in the world
    Vector3 point = {0, 0, 0};   // w+0xbc: the ground hit, or travel below the centre along the body's down axis without contact
    Vector3 normal = {0, 1, 0};  // w+0xc8: the hit record normal, the body's up axis without contact
    float distance = 0;          // w+0xe0: centre to the ground hit, the ray length without contact
    float force = 0;             // w+0xb4
    bool contact = false;        // w+0x13c
};

// The wheel part of the vehicle update 0x33aea8 for a dynamic body: per wheel the centre (mount + side offset in body axes), the ray down the
// body's up axis, and the spring force at the centre. Adds the forces to the body; returns the wheel states in wheel order.
std::vector<WheelState> applyWheelForces(VehicleRigidBody& body, const std::vector<VehicleWheel>& wheels, float suspensionScale,
                                         const WheelRay& ray);

// STAND-IN VEH-S32: the inertia comes from the collision shape (0x243f90 calls virtual +0x20 of the shape with the mass, scales by InertiaScale
// and passes it to 0x1f3124), which is not read. The inverse inertia +0x190 measured in the trooper and in the donut van; the trooper's for every
// other class.
Vector3 vehicleInverseInertiaStandIn(const std::string& className);

// The rigid body of a vehicle record whose node stands at `node` with `heading`: the centre of mass is the node plus CentreOfMass in body axes
// (0x243f90 keeps -CentreOfMass at body+0x44 as the node offset), the mass kVehicleBodyMassFactor * Mass.
VehicleRigidBody vehicleRigidBody(const VehicleRecord& record, const Vector3& node, float heading);
// btCompoundShape::calculateLocalInertia 0x33ee44 of the compound with one box child `low`..`high` (its AABB with margin, l = 2 half extents,
// mass * 0x3daaaaab * (ly^2 + lz^2, lx^2 + lz^2, lx^2 + ly^2)) for the class Mass, times InertiaScale when the class has it (0x243f90). Oracle
// (rigid +0x190 and body+0xb0): the trooper, the worker and the donut van, whose inertia follows its entity box (vehicleShapeBox).
Vector3 vehicleLocalInertia(float mass, const Vector3& low, const Vector3& high, const std::optional<std::array<float, 3>>& inertiaScale);
// The rigid body at mass scale body+0x84 (command 6 of 0x2e3390: mass = Mass * scale, inertia = vehicleLocalInertia * scale, setMassProps
// 0x1f3124 keeps the inverses); `shapeLow`..`shapeHigh` is vehicleShapeBox. Oracle: scale 1 for the parked worker, 2 for the vehicle the player sits
// in (the enter path 0x31fffc passes the float 0x3745b4, 2.0 in the oracle; the leave path 0x3222f4 passes the literal 1.0).
VehicleRigidBody vehicleRigidBody(const VehicleRecord& record, const Vector3& node, float heading, const Vector3& shapeLow, const Vector3& shapeHigh,
                                  float massScale);
void setVehicleMassScale(VehicleRigidBody& body, const VehicleRecord& record, const Vector3& shapeLow, const Vector3& shapeHigh, float massScale);
// The collision points of the box `low`..`high` (vehicleCollisionBox) and body+0xc8 = Mass * scale.
VehicleCollisionBody vehicleCollisionBody(const VehicleRecord& record, const Vector3& low, const Vector3& high, float massScale);
// The node matrix (rows right, up, forward, translation) of a rigid body.
std::array<float, 16> vehicleNodeMatrix(const VehicleRigidBody& body, const std::array<float, 3>& centreOfMass);

// The level floor of a vehicle body: the floor graph, the tracked cell and the body's collision points; optionally the body's Bullet contacts
// with the level's static boxes.
struct VehicleFloor {
    FloorMesh mesh;
    FloorTracker tracker;
    VehicleCollisionBody shape;
    VehicleBoxContacts* boxes = nullptr;
};
VehicleCollisionBody vehicleCollisionBody(const VehicleRecord& record);

// The drivetrain state of a new vehicle body: the engine at IdleSpeed (0x243e9c), gear 2 (0x33ce74), one spin state per wheel.
DrivetrainState vehicleDrivetrain(const VehicleRecord& record);

// One update of 0x33aea8 for a dynamic body, then the world step: the speed of the update before sets the linear damping (0.2 above the
// speed limit), the steering 0x33cb1c, the brake and gear rules, and unless the body was asleep at the start of the update (0x33b518 skips
// the wheels in the update it wakes) per wheel the setup 0x3396cc, the ray and suspension, the tyre force and the spin; then the drivetrain
// and the forces of the rigid body step; then the body moves through the floor collision pass 0x1c54f8, or freely without a floor (`floor`
// null, a test setup; in the game a body without a floor cell does not move).
std::vector<WheelState> stepVehiclePhysics(VehicleRigidBody& body, DrivetrainState& drive, const VehicleRecord& record, const BodyInputs& controls,
                                           bool wokeThisUpdate, const WheelRay& ray, float dt, VehicleFloor* floor = nullptr);

// The world steps of one update (vehicleWorldSteps) for one vehicle: per step the cell tracker 0x20c0e4, the wake test of 0x33b4a0 (0x2308e4
// resets the sleep time), then for an awake body stepVehiclePhysics with kVehicleWorldStep, the Bullet contacts with the static boxes
// (collision detection and solver of 0x30c768, after the floor pass) and the sleep rule updateVehicleActivation. An
// asleep body without a reason to wake stays asleep (mode 2). Returns whether the body is awake afterwards.
bool stepVehicleWorld(VehicleRigidBody& body, DrivetrainState& drive, bool awake, const VehicleRecord& record, const BodyInputs& controls,
                      const WheelRay& ray, int steps, VehicleFloor* floor = nullptr, std::vector<WheelState>* lastWheels = nullptr);

}
