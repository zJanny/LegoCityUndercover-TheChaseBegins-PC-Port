#pragma once
// Wheels and suspension of the vehicle physics body (docs/re/vehicles-physics.md): the wheel position 0x3396cc, the ground ray of 0x33c718 and
// the spring and damper force of 0x339030, applied to the rigid body at the wheel. Tyre forces (0x339374) and wheel spin (0x339a30) are VEH-32.
// No SDL dependency.
#include "runtime/collision.hpp"
#include "runtime/vehicle.hpp"
#include "runtime/vehicle_body.hpp"

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
std::optional<WheelHit> castWheelRay(const CollisionWorld& world, const Vector3& from, const Vector3& to);

struct WheelState {
    Vector3 centre = {0, 0, 0};  // w+0x100: the wheel centre in the world
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
// The node matrix (rows right, up, forward, translation) of a rigid body.
std::array<float, 16> vehicleNodeMatrix(const VehicleRigidBody& body, const std::array<float, 3>& centreOfMass);

// One update of a dynamic vehicle body: wheel forces, then the world step with the damping of 0x33aea8 (DampingLinear by the speed of the
// update before, DampingAngular). STAND-IN VEH-S33: the engine, tyres and steering are still the kinematic body (VEH-S3), so before the step
// the horizontal velocity is set to `speed` along `heading`, and after it the body is turned about the vertical to `heading`; height, pitch
// and roll come from the springs and gravity. Ends with keepWheelsAboveGround.
// STAND-IN VEH-S34 for the collision pass and constraint solver 0x30cdf8 (the body's CollSpheres against the level): after the step no wheel
// centre may lie below the ground that a ray from kBodyGroundLookUp above it finds; the body is lifted by the largest shortfall and loses its
// downward velocity. Without it a body that falls faster than the wheel rays reach drops through the street.
constexpr float kBodyGroundLookUp = 2.0f;
constexpr float kBodyGroundLookDown = 8.0f;
void keepWheelsAboveGround(VehicleRigidBody& body, const std::vector<VehicleWheel>& wheels, const WheelRay& ray);

std::vector<WheelState> stepVehiclePhysics(VehicleRigidBody& body, const VehicleRecord& record, float heading, float speed, const WheelRay& ray,
                                           float dt);

}
