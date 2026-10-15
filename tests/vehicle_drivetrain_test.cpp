// Tests of the engine, gears, steering and tyres (src/runtime/vehicle_drivetrain.*) and of the vehicle step that uses them. Every pinned number
// is an Azahar oracle value of the police trooper (state drv-police-car-seated, GDB memtrace at fixed dt 0.033 of the body 0x08e16100 and the
// rigid body 0x08e16f40, 2026-10-15, artifacts/oracle/agentVEH32-out: tA.mem = A held, sAR.mem = A and D-pad right). A replay case starts from
// one traced update and compares the port's next world step (1/30 s) with the next traced update; the ground is the plane of each wheel's traced hit.
// Evidence: docs/re/vehicles-physics.md.
#include "runtime/vehicle_drivetrain.hpp"
#include "vehicle_trooper.hpp"

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

std::array<float, 4> orientationFromAxes(const Vector3& right, const Vector3& up, const Vector3& forward) {
    const float m[3][3] = {{right[0], up[0], forward[0]}, {right[1], up[1], forward[1]}, {right[2], up[2], forward[2]}};
    const float trace = m[0][0] + m[1][1] + m[2][2];
    const float s = std::sqrt(trace + 1.0f) * 2.0f;
    return {(m[2][1] - m[1][2]) / s, (m[0][2] - m[2][0]) / s, (m[1][0] - m[0][1]) / s, 0.25f * s};
}

struct Snapshot {
    Vector3 right, up, forward, position, velocity, angular;
    float rpm, clutch;
    int gear;
    float angle, flick;
    int flickUpdates;
    float previousSteer;
    std::array<float, 4> rates;
    BodyInputs inputs;
    std::array<Vector3, 4> hits, normals;
};

struct Expected {
    Vector3 velocity, angular;
    float rpm, angle;
    std::array<float, 4> rates, forces;
};

void replay(const Snapshot& s, const Expected& e, double velocityTolerance, double angularTolerance, double forceFraction, const char* name) {
    const VehicleRecord record = trooper::record();
    VehicleRigidBody body;
    body.position = s.position;
    body.orientation = orientationFromAxes(s.right, s.up, s.forward);
    body.velocity = s.velocity;
    body.angularVelocity = s.angular;
    body.inverseMass = 1.0f / 2600.0f;
    body.inverseInertia = vehicleInverseInertiaStandIn(record.className);
    DrivetrainState drive = vehicleDrivetrain(record);
    drive.rpm = s.rpm;
    drive.clutch = s.clutch;
    drive.gear = s.gear;
    drive.steerAngle = s.angle;
    drive.steerFlick = s.flick;
    drive.steerFlickUpdates = s.flickUpdates;
    drive.previousSteer = s.previousSteer;
    for (std::size_t w = 0; w < 4; ++w) drive.wheels[w].rate = s.rates[w];
    const WheelRay ray = [&](const Vector3& from, const Vector3& to) -> std::optional<WheelHit> {
        std::size_t best = 0;
        float bestDistance = 1e30f;
        for (std::size_t w = 0; w < 4; ++w) {
            const float dx = s.hits[w][0] - from[0], dz = s.hits[w][2] - from[2];
            if (dx * dx + dz * dz < bestDistance) bestDistance = dx * dx + dz * dz, best = w;
        }
        const Vector3& p = s.hits[best];
        const Vector3& n = s.normals[best];
        const float a = (from[0] - p[0]) * n[0] + (from[1] - p[1]) * n[1] + (from[2] - p[2]) * n[2];
        const float c = (to[0] - p[0]) * n[0] + (to[1] - p[1]) * n[1] + (to[2] - p[2]) * n[2];
        if (a < 0.0f || c >= 0.0f) return std::nullopt;
        const float t = a / (a - c);
        return WheelHit{{from[0] + (to[0] - from[0]) * t, from[1] + (to[1] - from[1]) * t, from[2] + (to[2] - from[2]) * t}, n};
    };
    const std::vector<WheelState> wheels = stepVehiclePhysics(body, drive, record, s.inputs, false, ray, kVehicleWorldStep);
    std::printf("%s: velocity %.4f %.4f %.4f angular %.4f %.4f %.4f rpm %.2f angle %.6f\n", name, body.velocity[0], body.velocity[1], body.velocity[2],
                body.angularVelocity[0], body.angularVelocity[1], body.angularVelocity[2], drive.rpm, drive.steerAngle);
    for (std::size_t a = 0; a < 3; ++a) {
        near(body.velocity[a], e.velocity[a], velocityTolerance, "velocity after one update");
        near(body.angularVelocity[a], e.angular[a], a == 1 ? 0.002 : angularTolerance, "angular velocity after one update");
    }
    near(drive.rpm, e.rpm, 0.1, "engine rpm");
    near(drive.steerAngle, e.angle, 1e-5, "steer angle");
    for (std::size_t w = 0; w < 4; ++w) {
        near(drive.wheels[w].rate, e.rates[w], 0.01, "wheel spin");
        near(wheels[w].force, e.forces[w], e.forces[w] * forceFraction, "suspension force");
    }
}

void worldSteps() {
    // ctrace of 0x33aea8 (agentVEH32-out sub033, sub066, sub020): one vehicle update per game update at dt 0.033, two at 0.066, one or none
    // at 0.02; tA skips the step of update 42 (the dump repeats update 41).
    float leftover = 0.0f;
    int steps = 0;
    for (int i = 0; i < 150; ++i) steps += vehicleWorldSteps(leftover, 0.033f);
    expect(steps == 148, "dt 0.033: 148 steps in 150 updates (a skipped step about every 100 updates)", steps);
    leftover = 0.0f;
    expect(vehicleWorldSteps(leftover, 0.066f) == 1 && vehicleWorldSteps(leftover, 0.066f) == 2, "dt 0.066: 1, then 2 steps");
    leftover = 0.0f;
    expect(vehicleWorldSteps(leftover, 0.02f) == 0 && vehicleWorldSteps(leftover, 0.02f) == 1, "dt 0.02: 0, then 1 step");
    // 0x30c768: at most scheduler +0xc0 = 2 steps (oracle read at 0x5164a8); the steps beyond are dropped, not kept for later.
    leftover = 0.0f;
    expect(vehicleWorldSteps(leftover, 0.2f) == 2, "dt 0.2: 6 whole steps, 2 run");
    expect(leftover < kVehicleWorldStep && vehicleWorldSteps(leftover, 0.0f) == 0, "dt 0.2: the dropped steps are not carried over");
}

void engineSpec() {
    const VehicleRecord record = trooper::record();
    near(record.engine.finalDrive, 15.87732, 1e-4, "0x2435d0: final drive from MaxRPM, TopSpeed, the top gear and the wheel radius (oracle body+0x29c)");
    near(record.engine.curve[1], 97.5, 1e-4, "0x243e9c: curve 85 + Friction 0.025 * 500 (oracle body+0x1e8)");
    near(record.engine.curve[16], 200.0, 1e-4, "0x243e9c: an empty entry is Friction * rpm (oracle body+0x224)");
}

void wakeUpdate() {
    // tA frame 16 -> 17: the first update with A after the forced stop; the body wakes (0x2308e4), the wheels are skipped (0x33b518), the
    // drivetrain runs: clutch 0 -> 0.4, rpm 900 -> 701.617, wheel spin 0 -> 3.2808 front, 7.6333 rear.
    const VehicleRecord record = trooper::record();
    DrivetrainState drive = vehicleDrivetrain(record);
    drive.gear = kGearTop;
    expect(controlDrivetrain(drive, record.engine, {0.0f, 1.0f, 0.0f, 0.0f}, 0.0f, 0.0f), "below the speed limit");
    near(drive.throttle, 1.0, 1e-9, "A in a forward gear is the throttle");
    near(drive.brakeOut, 0.0, 1e-9, "no brake with A");
    stepDrivetrain(drive, record.engine, {record.wheels[0].grip, record.wheels[1].grip, record.wheels[2].grip, record.wheels[3].grip}, 0.0f);
    near(drive.clutch, 0.4, 1e-6, "clutch +0.4 with throttle");
    near(drive.rpm, 701.617126, 0.01, "rpm after the momentum exchange");
    near(drive.wheels[0].rate, 3.28076124, 1e-4, "front wheel spin");
    near(drive.wheels[2].rate, 7.63329935, 1e-4, "rear wheel spin");
    expect(vehicleWakes({0.0f, 1.0f, 0.0f, 0.0f}, 0.0f, 0.0f, 0.0f), "A wakes the body");
    expect(!vehicleWakes({}, 0.3f, 1.0f, 0.0f), "slow and no input: no wake");
    expect(vehicleWakes({0.2f, 0.0f, 0.0f, 0.0f}, 0.0f, 0.0f, 0.0f), "a changed steer input wakes the body");
}

void brakeAndGears() {
    const VehicleRecord record = trooper::record();
    DrivetrainState drive = vehicleDrivetrain(record);
    drive.gear = kGearTop;
    controlDrivetrain(drive, record.engine, {0.0f, 0.0f, 1.0f, 0.0f}, 13.2f, 174.0f);
    expect(drive.brakeOut == 1.0f && drive.gear == kGearTop, "B above 6 m/s brakes (tB frame 117: brake 1, gear 6)");
    controlDrivetrain(drive, record.engine, {0.0f, 0.0f, 1.0f, 0.0f}, 1.4f, 2.0f);
    expect(drive.gear == kGearReverse, "B below 1.5 m/s selects reverse (fS frame 60: gear 0 at 0.63 m/s, not at 1.67 m/s in frame 59)");
    controlDrivetrain(drive, record.engine, {0.0f, 0.0f, 1.0f, 0.0f}, -3.0f, 9.0f);
    expect(drive.throttle == 1.0f && drive.brakeOut == 0.0f, "B in reverse is the throttle");
    controlDrivetrain(drive, record.engine, {}, -3.0f, 9.0f);
    near(drive.brakeOut, 0.2, 1e-6, "coasting at 2 m/s or more brakes with Slowdown (tC: brake 0.2)");
    controlDrivetrain(drive, record.engine, {}, -1.0f, 1.0f);
    near(drive.brakeOut, 1.0, 1e-6, "coasting below 2 m/s brakes fully");
    controlDrivetrain(drive, record.engine, {0.0f, 1.0f, 0.0f, 0.0f}, -1.0f, 1.0f);
    expect(drive.gear == kGearFirst && drive.brakeOut == 0.5f, "A in reverse brakes with 0.5 and selects gear 2 above -3.5 m/s");
    drive.gear = kGearTop;
    drive.steerAngle = 0.3f;
    controlDrivetrain(drive, record.engine, {0.0f, 0.0f, 1.0f, 0.0f}, 10.0f, 100.0f);
    expect(drive.handBrakeOut == 1.0f && drive.brakeOut == 0.0f, "B above 6 m/s with the wheels turned more than 0.2 is the hand brake");
}

void steering() {
    // sAR frame 44 -> 45: steer 1 held (flick sum 4), angle 0.343858778 -> 0.409834385 at 13.7 m/s.
    const VehicleRecord record = trooper::record();
    DrivetrainState drive = vehicleDrivetrain(record);
    drive.steerAngle = 0.343858778f;
    drive.steerFlick = 4.0f;
    drive.steerFlickUpdates = -1;
    steerVehicle(drive, record.engine.steering, 1.0f, {-0.728581488f, 0.0146848224f, -0.684801579f}, {-10.8159151f, 0.00922959205f, -8.35695648f});
    near(drive.steerAngle, 0.409834385, 1e-6, "steer angle of 0x33cb1c");
    near(drive.steerFlick, 5.0, 1e-6, "the steer input adds to the flick sum while above 0.5");
    drive.steerAngle = 0.0f;
    drive.steerFlick = 4.0f;
    drive.steerFlickUpdates = -1;
    steerVehicle(drive, record.engine.steering, 0.0f, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f});
    near(drive.steerFlick, 0.72, 1e-6, "on release the flick is 0.25 * sum, at most 0.8, then 0.9 of it remains");
    expect(drive.steerAngle < 0.0f, "the release flick steers the other way", drive.steerAngle);
}

void replays() {
    replay({{-0.709513068f, 0.00138619123f, 0.704690754f}, {0.00444572698f, 0.999986947f, 0.00250908406f},
            {-0.704678118f, 0.00491309119f, -0.709509969f}, {886.830444f, 17.0750675f, 1116.76587f}, {-2.84091783f, 0.0236335192f, -2.86262512f},
            {-0.0519470833f, 0.000401644327f, 0.0596417077f}, 1327.50232f, 1.0f, 6, 0.0f, 0.0f, 0, 0.0f,
            {7.32849026f, 7.32752895f, 11.7905788f, 11.7916632f}, {0.0f, 1.0f, 0.0f, 0.0f},
            {Vector3{886.415833f, 16.6054935f, 1115.03198f}, {885.098572f, 16.6056995f, 1116.34021f}, {887.221619f, 16.6172466f, 1118.51648f},
             {888.577454f, 16.6142006f, 1117.1698f}},
            {Vector3{-0.00335141248f, 0.999988079f, -0.00353018334f}, {-0.00335141248f, 0.999988079f, -0.00353018334f},
             {-0.000848137541f, 0.999994814f, -0.00311339088f}, {0.000876202947f, 0.999997437f, -0.00207548589f}}},
           {{-3.05170012f, -0.00183335494f, -3.0725286f}, {-0.0342929401f, 0.000576201419f, 0.0430736877f}, 1413.65149f, 0.0f,
            {7.96509504f, 7.96407223f, 12.4793873f, 12.4805517f}, {24076.3203f, 23874.0234f, 25759.9082f, 25692.5547f}},
           0.0005, 0.0005, 0.001, "tA frame 25 -> 26 (A held, 3.0 m/s)");
    // In the turn the roll and pitch rates differ by up to 0.05 rad/s, the velocity by 0.03 and the spring forces by up to 7 % (the inertia is a
    // stand-in, VEH-S32, and the replay has no floor collision); the yaw rate stays within 0.002.
    const Vector3 flat = {0.00240988517f, 0.999997139f, 3.39709841e-05f};
    replay({{-0.684325933f, 0.0273690913f, 0.728662312f}, {0.0294426773f, 0.9995175f, -0.00989140756f}, {-0.728581488f, 0.0146848224f, -0.684801579f},
            {881.581909f, 17.0517101f, 1111.67639f}, {-10.8159151f, 0.00922959205f, -8.35695648f}, {-0.169964358f, 0.465350419f, -0.1492185f},
            4058.41162f, 1.0f, 6, 0.343858778f, 4.0f, -1, 1.0f, {27.2033119f, 26.941309f, 33.5973167f, 33.8796959f}, {1.0f, 1.0f, 0.0f, 0.0f},
            {Vector3{881.096313f, 16.5940037f, 1109.96362f}, {879.824463f, 16.5970249f, 1111.31689f}, {882.021851f, 16.5916576f, 1113.41809f},
             {883.331055f, 16.5885506f, 1112.02515f}},
            {flat, flat, flat, flat}},
           {{-11.4307461f, 0.0367507115f, -8.08181286f}, {-0.181684524f, 0.638130307f, -0.109699093f}, 4162.14453f, 0.409834385f,
            {28.0591888f, 27.6075764f, 34.3201904f, 34.816349f}, {27162.2051f, 19314.8125f, 24501.5234f, 32453.0977f}},
           0.03, 0.06, 0.07, "sAR frame 44 -> 45 (A and right, 13.7 m/s)");
}

}

int main() {
    worldSteps();
    engineSpec();
    wakeUpdate();
    brakeAndGears();
    steering();
    replays();
    if (failures) {
        std::printf("%d failure(s)\n", failures);
        return 1;
    }
    std::printf("vehicle-drivetrain tests passed\n");
    return 0;
}
