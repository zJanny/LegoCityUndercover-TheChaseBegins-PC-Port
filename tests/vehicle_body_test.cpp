// Tests of the rigid body of the vehicle physics body (src/runtime/vehicle_body.*). Pinned values: oracle runs of 2026-10-15 (Azahar GDB memory
// traces at fixed dt 0.033, state drv-police-car-seated, rigid body at entity body+0x7c; docs/re/vehicles-physics.md): the damping coefficient
// (+0x1c0) of an update is DampingLinear * (0.04 * |velocity of the update before|)^2 with DampingLinear 0.02 (body+0xd58); the rest from the
// code of 0x1f2eb4, 0x1f2c20, 0x1f2a50, 0x2d40f0 and 0x341694 (REA pseudo code, llvm-objdump).
#include "runtime/vehicle_body.hpp"

#include <cmath>
#include <cstdio>

namespace {

using namespace chase::runtime;

int failures = 0;

void expect(bool ok, const char* what, double got = 0.0) {
    if (!ok) {
        std::printf("FAIL: %s (got %.6f)\n", what, got);
        ++failures;
    }
}

void near(double got, double want, double tolerance, const char* what) { expect(std::fabs(got - want) <= tolerance, what, got); }

VehicleRigidBody trooper() {
    VehicleRigidBody body;
    body.position = {10.0f, 5.0f, 20.0f};
    body.inverseMass = 1.0f / 2600.0f;
    body.inverseInertia = {0.000138644f, 0.0000537915f, 0.000355591f};
    return body;
}

}  // namespace

int main() {
    near(vehicleLinearDamping(0.02f, 5.9083f), 0.001117, 2e-6, "damping at 5.91 m/s (oracle trace row 19)");
    near(vehicleLinearDamping(0.02f, 9.9325f), 0.003157, 2e-6, "damping at 9.93 m/s (oracle trace row 29)");
    near(vehicleLinearDamping(0.02f, 13.6366f), 0.005951, 2e-6, "damping at 13.64 m/s (oracle trace row 39)");
    near(vehicleLinearDamping(0.02f, 19.1199f), 0.011698, 2e-6, "damping at 19.12 m/s (oracle trace row 54)");
    near(vehicleLinearDamping(0.02f, 0.0f), 0.0, 0.0, "no damping at rest");
    near(vehicleLinearDamping(0.02f, 1000.0f), 0.02, 1e-9, "the speed is capped at 25.0 before the scale (0x33b0ec, constant 0x33b1f8; llvm-objdump)");
    near(vehicleLinearDamping(60.0f, 25.0f), 1.0, 0.0, "damping is clamped to 1 (0x1e124c)");

    VehicleRigidBody body = trooper();
    stepRigidBody(body, 0.0f, 0.0f, 0.033f);
    near(body.velocity[1], -39.0 * 0.033, 1e-5, "gravity -39 adds -1.287 m/s per 0.033 s step");
    near(body.position[1], 5.0 - 39.0 * 0.033 * 0.033, 1e-5, "the position integrates with the new velocity (semi-implicit)");
    near(body.force[1], 0.0, 0.0, "the step clears the force");

    body = trooper();
    body.velocity = {0.0f, -10.0f, 0.0f};
    stepRigidBody(body, 0.5f, 0.0f, 0.033f);
    near(body.velocity[1], (-10.0 - 39.0 * 0.033) * std::pow(0.5, 0.033), 1e-4, "0x1f2c20: velocity times (1 - damping)^dt after the force");

    const std::array<Vector3, 3> axes = bodyAxes(VehicleRigidBody{{0, 0, 0}, headingOrientation(0.5f)});
    near(axes[2][0], std::sin(0.5), 1e-6, "heading 0.5: forward x = sin h");
    near(axes[2][2], std::cos(0.5), 1e-6, "heading 0.5: forward z = cos h");
    near(axes[0][0], std::cos(0.5), 1e-6, "heading 0.5: right x = cos h");
    near(axes[0][2], -std::sin(0.5), 1e-6, "heading 0.5: right z = -sin h");
    near(axes[1][1], 1.0, 1e-6, "heading only: up stays vertical");

    body = trooper();
    addForceAtPoint(body, {0.0f, 1000.0f, 0.0f}, {10.0f, 5.0f, 21.5f});
    near(body.force[1], 1000.0, 0.0, "0x2d40f0: the force adds to +0x1a0");
    near(body.torque[0], -1500.0, 1e-3, "0x2d40f0: torque (point - position) x force; an upward push in front turns about -x");
    body.angularVelocity = {0.0f, 2.0f, 0.0f};
    body.velocity = {1.0f, 0.0f, 0.0f};
    const Vector3 velocity = pointVelocity(body, {10.0f, 5.0f, 21.0f});
    near(velocity[0], 3.0, 1e-6, "0x341694: velocity + angular velocity x arm (x)");
    near(velocity[2], 0.0, 1e-6, "0x341694: velocity + angular velocity x arm (z)");

    body = trooper();
    body.torque = {-1500.0f, 0.0f, 0.0f};
    body.force = {0.0f, 39.0f * 2600.0f, 0.0f};
    stepRigidBody(body, 0.0f, 0.0f, 0.033f);
    near(body.angularVelocity[0], -1500.0 * 0.000138644 * 0.033, 1e-7, "0x1f2eb4: inverse inertia about the body x axis times torque times dt");
    expect(bodyAxes(body)[2][1] > 0.0f, "turning about -x lifts the nose (forward y grows)", bodyAxes(body)[2][1]);

    body = trooper();
    body.force = {0.0f, 39.0f * 2600.0f, 0.0f};
    body.torque = {1.0e9f, 0.0f, 0.0f};
    stepRigidBody(body, 0.0f, 0.0f, 0.033f);
    near(body.angularVelocity[0] * 0.033, kVehicleAngularStepLimit, 1e-4, "0x1f2eb4: at most a quarter turn per step");

    body = trooper();
    body.angularVelocity = {0.0f, 1.0f, 0.0f};
    for (int i = 0; i < 30; ++i) {
        body.force = {0.0f, 39.0f * 2600.0f, 0.0f};
        stepRigidBody(body, 0.0f, 0.0f, 0.033f);
    }
    near(std::atan2(bodyAxes(body)[2][0], bodyAxes(body)[2][2]), 0.99, 1e-3, "0x1f2a50: 30 steps of 1 rad/s about y turn the heading by 0.99 rad");

    if (failures != 0) return 1;
    std::printf("vehicle body tests passed\n");
    return 0;
}
