// Test of the wheel node pose (src/runtime/vehicle_wheel_pose.*), the transcription of 0x33929c (docs/re/vehicles-physics.md). The mount, radius and
// distance numbers are the trooper's from the oracle records (wheel 0 mount (-0.85, 0.0685, 1.52), radius 0.48; rest distance 0.5288 front, the
// "Measured" paragraph of the suspension entry in docs/re/vehicles-physics.md). The expected matrices follow from the disassembly of 0x33929c,
// 0x237bc4 (rotation about Y), 0x2354b4 (rotation about X) and 0x258a10 (row vector product); no oracle trace of a wheel node exists.
#include "runtime/vehicle_wheel_pose.hpp"

#include <cmath>
#include <cstdio>

namespace {

using namespace chase::runtime;

int failures = 0;

void near(double got, double want, double tolerance, const char* what) {
    if (std::fabs(got - want) > tolerance) {
        std::printf("FAIL: %s (got %.5f want %.5f)\n", what, got, want);
        ++failures;
    }
}

const chase::assets::Matrix kIdentity = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

WheelSuspension trooperFrontLeft() {
    WheelSuspension w;
    w.mount = {-0.85f, 0.0685f, 1.52f};
    w.radius = 0.48f;
    return w;
}

WheelSuspension trooperFrontRight() {
    WheelSuspension w = trooperFrontLeft();
    w.mount[0] = 0.85f;
    return w;
}

void restPose() {
    WheelSpinState spin;
    const chase::assets::Matrix m = wheelPoseMatrix(trooperFrontLeft(), spin, 0, 0.5288f, kIdentity);
    near(m[12], -0.85, 1e-6, "mount x");
    near(m[13], 0.0685 - (0.5288 - 0.48), 1e-5, "mount y lowered by distance - radius");
    near(m[14], 1.52, 1e-6, "mount z");
    near(m[0], -1.0, 1e-6, "left wheel turned by pi: right row x");
    near(m[10], -1.0, 1e-6, "left wheel turned by pi: forward row z");
    near(m[5], 1.0, 1e-6, "up row unchanged");
}

void rightWheelUnturned() {
    WheelSpinState spin;
    const chase::assets::Matrix m = wheelPoseMatrix(trooperFrontRight(), spin, 0, 0.48f, kIdentity);
    near(m[0], 1.0, 1e-6, "right wheel not turned");
    near(m[13], 0.0685, 1e-6, "distance equal to the radius leaves the mount height");
}

void spinSign() {
    WheelSpinState spin;
    spin.angle = 0.5f;
    chase::assets::Matrix m = wheelPoseMatrix(trooperFrontRight(), spin, 2, 0.48f, kIdentity);
    near(m[6], std::sin(0.5), 1e-6, "right wheel: spin turns the up row towards forward");
    near(m[9], -std::sin(0.5), 1e-6, "right wheel: forward row towards up");
    m = wheelPoseMatrix(trooperFrontLeft(), spin, 2, 0.48f, kIdentity);
    near(m[6], std::sin(0.5), 1e-6, "left wheel: spin negated, then the turn by pi mirrors the forward row");
    near(m[9], std::sin(0.5), 1e-6, "left wheel: forward row");
}

void steerFrontOnly() {
    WheelSpinState spin;
    spin.steerAngle = 0.3f;
    const chase::assets::Matrix front = wheelPoseMatrix(trooperFrontRight(), spin, 1, 0.48f, kIdentity);
    near(front[0], std::cos(0.3), 1e-6, "front wheel steered");
    near(front[2], -std::sin(0.3), 1e-6, "steer turns the right row");
    near(front[8], std::sin(0.3), 1e-6, "steer turns the forward row");
    const chase::assets::Matrix rear = wheelPoseMatrix(trooperFrontRight(), spin, 2, 0.48f, kIdentity);
    near(rear[0], 1.0, 1e-6, "rear wheel not steered");
    const chase::assets::Matrix left = wheelPoseMatrix(trooperFrontLeft(), spin, 0, 0.48f, kIdentity);
    near(left[0], -std::cos(0.3), 1e-6, "left front wheel: turn by pi plus the steer angle");
}

void bodyMatrixApplies() {
    chase::assets::Matrix body = kIdentity;
    body[12] = 10.0f;
    body[13] = 20.0f;
    body[14] = 30.0f;
    WheelSpinState spin;
    const chase::assets::Matrix m = wheelPoseMatrix(trooperFrontLeft(), spin, 0, 0.48f, body);
    near(m[12], 10.0 - 0.85, 1e-5, "world x");
    near(m[13], 20.0 + 0.0685, 1e-5, "world y");
    near(m[14], 30.0 + 1.52, 1e-5, "world z");
}

void bodyOrigin() {
    VehicleRigidBody body;
    body.position = {1, 2, 3};
    const chase::assets::Matrix m = bodyOriginMatrix(body);
    near(m[12], 1, 1e-6, "origin x");
    near(m[13], 2, 1e-6, "origin y");
    near(m[14], 3, 1e-6, "origin z");
    near(m[0], 1, 1e-6, "identity orientation");
    near(m[15], 1, 1e-6, "w");
}

}

int main() {
    restPose();
    rightWheelUnturned();
    spinSign();
    steerFrontOnly();
    bodyMatrixApplies();
    bodyOrigin();
    if (failures) return 1;
    std::printf("vehicle wheel pose ok\n");
    return 0;
}
