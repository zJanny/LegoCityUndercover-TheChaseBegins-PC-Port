// Tests of the free-body part of the vehicle physics body (src/runtime/vehicle_body.*). Pinned values: oracle run of 2026-10-15 (Azahar GDB memory
// trace at fixed dt 0.033, state drv-police-car-seated, A held 60 updates, rigid body at entity body+0x7c; docs/re/vehicles-physics.md): the damping
// coefficient (+0x1c0) of an update is DampingLinear * (0.04 * |velocity of the update before|)^2 with DampingLinear 0.02 (body+0xd58).
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

}  // namespace

int main() {
    near(vehicleLinearDamping(0.02f, 5.9083f), 0.001117, 2e-6, "damping at 5.91 m/s (oracle trace row 19)");
    near(vehicleLinearDamping(0.02f, 9.9325f), 0.003157, 2e-6, "damping at 9.93 m/s (oracle trace row 29)");
    near(vehicleLinearDamping(0.02f, 13.6366f), 0.005951, 2e-6, "damping at 13.64 m/s (oracle trace row 39)");
    near(vehicleLinearDamping(0.02f, 19.1199f), 0.011698, 2e-6, "damping at 19.12 m/s (oracle trace row 54)");
    near(vehicleLinearDamping(0.02f, 0.0f), 0.0, 0.0, "no damping at rest");
    near(vehicleLinearDamping(0.02f, 1000.0f), 0.02, 1e-9, "the speed is capped at 25.0 before the scale (0x33b0ec, constant 0x33b1f8; llvm-objdump)");
    near(vehicleLinearDamping(60.0f, 25.0f), 1.0, 0.0, "damping is clamped to 1 (0x1e124c)");

    VehicleFall fall;
    fall.height = 10.0f;
    stepVehicleFall(fall, 0.02f, 0.0f, 0.033f);
    near(fall.velocity, -39.0 * 0.033, 1e-5, "gravity -39 adds -1.287 m/s per 0.033 s step");
    near(fall.height, 10.0 - 39.0 * 0.033 * 0.033, 1e-5, "height integrates with the new velocity (semi-implicit)");
    for (int i = 0; i < 29; ++i) stepVehicleFall(fall, 0.02f, std::fabs(fall.velocity), 0.033f);
    expect(fall.velocity < -30.0f && fall.velocity > -39.0f * 30 * 0.033f, "a second of fall is slower than the undamped fall (quadratic drag)", fall.velocity);

    VehicleFall resting;
    resting.height = 5.0f;
    resting.velocity = -3.0f;
    settleOnGround(resting, 4.9f);
    near(resting.height, 4.9, 1e-6, "within the contact reach the body sits on the ground target");
    near(resting.velocity, 0.0, 0.0, "contact removes the vertical velocity");
    resting.height = 6.0f;
    resting.velocity = -3.0f;
    settleOnGround(resting, 4.9f);
    near(resting.height, 6.0, 0.0, "above the contact reach the body keeps falling");
    resting.height = 4.0f;
    settleOnGround(resting, 4.9f);
    near(resting.height, 4.9, 1e-6, "below the ground target the body is lifted to it");

    if (failures != 0) return 1;
    std::printf("vehicle body tests passed\n");
    return 0;
}
