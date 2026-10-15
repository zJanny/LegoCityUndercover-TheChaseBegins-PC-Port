#pragma once
// The rigid body of the vehicle physics body (docs/re/vehicles-physics.md): gravity, the speed-dependent linear damping, forces at points and the
// world step 0x30c768 of one body (0x1f2eb4 forces, 0x1f2c20 damping, 0x1f2a50 prediction). The collision pass and the constraint solver 0x30cdf8
// are not recovered. No SDL dependency.
#include <array>

namespace chase::runtime {

using Vector3 = std::array<float, 3>;

// Rigid body +0x180 of the police trooper and of the donut van: (0, -39, 0), force 0x2d41c8 adds it times the mass every step.
constexpr float kVehicleBodyGravity = -39.0f;
// 0x33b0d4..0x33b120: linear damping = DampingLinear * (0.04 * min(|velocity|, 25))^2, clamped to 0..1 (0x1e124c).
constexpr float kVehicleDampingSpeedScale = 0.04f;
constexpr float kVehicleDampingSpeedCap = 25.0f;
// 0x33b44c..0x33b474: above the speed limit body+0xdbc the linear damping is 0.2.
constexpr float kVehicleSpeedLimitDamping = 0.2f;
// The physics world 0x30c768 steps in fixed steps and keeps the leftover time (world +0xf0); the vehicle update 0x33aea8 runs once per step.
// STAND-IN VEH-S42: the step 1/30 s is measured (oracle: 2 steps at dt 0.066, 1 or 0 at dt 0.02, one skipped step per about 100 updates at
// dt 0.033, and the traced updates replay exactly with 1/30); where the step value comes from is not read.
constexpr float kVehicleWorldStep = 1.0f / 30.0f;
// Adds dt to the leftover time and returns the number of whole world steps it holds, keeping the rest.
int vehicleWorldSteps(float& leftover, float dt);
// 0x1f2eb4: the angular velocity is limited to a quarter turn (constant 0x3fc90fdb) per step.
constexpr float kVehicleAngularStepLimit = 1.57079637f;

float vehicleLinearDamping(float dampingLinear, float speed);

// The physics world keeps its matrices in the other convention than the scene nodes: 0x232788 transposes them, so the body axes (right, up,
// forward as the rows of the node matrix) are the columns of the rotation of the quaternion.
struct VehicleRigidBody {
    Vector3 position = {0, 0, 0};                   // +0x34: the centre of mass
    std::array<float, 4> orientation = {0, 0, 0, 1};  // x, y, z, w as 0x1eb950 converts the matrix +0x04
    Vector3 velocity = {0, 0, 0};                   // +0x13c
    Vector3 angularVelocity = {0, 0, 0};            // +0x14c, world axes
    float inverseMass = 0;                          // +0x15c
    Vector3 inverseInertia = {0, 0, 0};             // +0x190, body axes
    Vector3 force = {0, 0, 0};                      // +0x1a0
    Vector3 torque = {0, 0, 0};                     // +0x1b0
};

// Rows: right, up, forward in world coordinates.
std::array<Vector3, 3> bodyAxes(const VehicleRigidBody& body);
Vector3 bodyToWorld(const VehicleRigidBody& body, const Vector3& local);
// The heading of the quaternion for a body that only turns about the vertical: forward (sin h, 0, cos h).
std::array<float, 4> headingOrientation(float heading);

// 0x33a4b4 / 0x2d40f0: force += F, torque += (point - position) x F.
void addForceAtPoint(VehicleRigidBody& body, const Vector3& force, const Vector3& point);
// 0x1e131c / 0x341694: velocity + angular velocity x (point - position).
Vector3 pointVelocity(const VehicleRigidBody& body, const Vector3& point);
// 0x229948: the velocity along the body's forward axis (body+0xc0 holds it from the start of the update).
float forwardSpeed(const VehicleRigidBody& body);
// 0x17b3d4..0x17b454: the velocity scaled so that the forward speed is at most `limit` in size.
void limitForwardSpeed(VehicleRigidBody& body, float limit);

// One step of 0x30c768 for one dynamic body: gravity force (0x30c630), 0x1f2eb4 (velocity += force * inverse mass * dt, angular velocity +=
// inverse world inertia * torque * dt, limited to kVehicleAngularStepLimit per step), 0x1f2c20 (both velocities times (1 - damping)^dt),
// 0x1f2a50 (position += velocity * dt, quaternion += dt / 2 * (angular velocity, 0) * quaternion, normalised). Clears the force and torque.
void stepRigidBody(VehicleRigidBody& body, float linearDamping, float angularDamping, float dt);

}
