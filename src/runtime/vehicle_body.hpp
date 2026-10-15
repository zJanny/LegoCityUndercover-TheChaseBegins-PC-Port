#pragma once
// The free-body part of the vehicle physics body (docs/re/vehicles-physics.md): gravity, the speed-dependent linear damping and the semi-implicit
// vertical integration of the rigid body update. Wheels and suspension are not recovered; the ground contact below is the STAND-IN VEH-S20.
// No SDL dependency.
namespace chase::runtime {

// Rigid body +0x180 of the police trooper and of the donut van: (0, -39, 0), force 0x2d41c8 adds it times the mass every step.
constexpr float kVehicleBodyGravity = -39.0f;
// 0x33b0d4..0x33b120: linear damping = DampingLinear * (0.04 * min(|velocity|, 25))^2, clamped to 0..1 (0x1e124c).
constexpr float kVehicleDampingSpeedScale = 0.04f;
constexpr float kVehicleDampingSpeedCap = 25.0f;

float vehicleLinearDamping(float dampingLinear, float speed);

struct VehicleFall {
    float height = 0.0f;
    float velocity = 0.0f;
};

// 0x1f2eb4 (velocity += gravity * dt), 0x1f2c20 (velocity *= (1 - damping)^dt) and 0x1f2a50 (height += velocity * dt), in this order.
void stepVehicleFall(VehicleFall& fall, float dampingLinear, float speed, float dt);

// STAND-IN VEH-S20: the wheels' ground contact. A body at or below the ground target is put on it and loses its vertical velocity; one within
// kVehicleContactReach above it is put on it as well (suspension travel), one higher falls.
constexpr float kVehicleContactReach = 0.15f;
void settleOnGround(VehicleFall& fall, float groundTarget);

}
