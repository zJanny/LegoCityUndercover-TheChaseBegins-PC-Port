// Tests of the driving rules (src/runtime/vehicle_drive.*). The numbers of the kinematic stand-in body are pinned against oracle measurements
// of the police trooper made on 2026-10-15 with Azahar (state drv-police-car-seated, RPC samples of the vehicle node, 30 updates per second);
// each block names its source run. Evidence: docs/re/vehicles-driving.md.
#include "runtime/vehicle_drive.hpp"

#include <cmath>
#include <cstdio>

namespace {

using namespace chase::runtime;

int failures = 0;

void expect(bool ok, const char* what, double got = 0.0) {
    if (!ok) {
        std::printf("FAIL: %s (got %.4f)\n", what, got);
        ++failures;
    }
}

void near(double got, double want, double tolerance, const char* what) { expect(std::fabs(got - want) <= tolerance, what, got); }

constexpr float kDt = 1.0f / 30.0f;
constexpr float kPoliceTopSpeed = 28.0f;

DriveBody run(DriveBody body, const DriveCommand& command, int updates, float topSpeed = kPoliceTopSpeed) {
    for (int i = 0; i < updates; ++i) stepDriveBody(body, command, topSpeed, kDt);
    return body;
}

void inputRules() {
    DriveControls controls;
    controls.stick = 0.291f;
    DriveInput input = readDriveInput(controls);
    near(input.steer, 0.291, 1e-6, "steer is the stick value (0x29cd9c)");
    controls.dpadLeft = 1.0f;
    near(readDriveInput(controls).steer, -1.0, 1e-6, "left D-pad channel 14 sets steer -1");
    controls.dpadRight = 1.0f;
    near(readDriveInput(controls).steer, 1.0, 1e-6, "right D-pad channel 15 wins over left");
    controls.accelerate = 0.5f;
    controls.brake = 0.0f;
    expect(readDriveInput(controls).accelerate && !readDriveInput(controls).brake, "A above zero accelerates, B at zero does not brake");
    controls.accelerate = 0.0f;
    controls.brake = 1.0f;
    expect(!readDriveInput(controls).accelerate && readDriveInput(controls).brake, "B above zero brakes");
}

void stateRules() {
    DriveInput held;
    held.accelerate = true;
    held.steer = 0.5f;
    expect(!commandForState(5, held, false, 10.0f).handBrake, "state 5 has no hand brake");
    expect(commandForState(3, held, false, 10.0f).handBrake, "state 3 sets the hand brake (0x23019c)");
    expect(commandForState(4, held, false, 0.0f).handBrake, "state 4 sets the hand brake");
    expect(commandForState(11, held, false, 0.0f).handBrake, "state 11 sets the hand brake");
    const DriveCommand stopped = commandForState(5, held, true, 10.0f);
    expect(stopped.handBrake && stopped.brake && !stopped.accelerate, "forced stop: hand brake, brake, no accelerate");
    near(stopped.steer, 0.5, 1e-6, "forced stop keeps the steering");
    expect(!commandForState(5, held, true, 0.05f).brake, "forced stop releases the brake at 0.1 or less");
    expect(requestedStateOnPlayerEntered(4) == 5u && requestedStateOnPlayerEntered(10) == 5u && requestedStateOnPlayerEntered(3) == 5u, "message 84 requests 5");
    expect(!requestedStateOnPlayerEntered(6) && !requestedStateOnPlayerEntered(7), "message 84 does nothing in states 6 and 7 (0x17a4bc)");
    expect(requestedStateOnPlayerExited(5) == 3u, "message 85 requests 3 from state 5");
    expect(!requestedStateOnPlayerExited(10) && !requestedStateOnPlayerExited(4), "message 85 keeps states 10 and 4 (0x17a530)");
}

void accelerationAgainstOracle() {
    // Oracle: A held from standstill, speed after k updates counted from the first update that moved (the first update already reaches 2.1 m/s),
    // median of the runs pcA, pcB, coast, exitmv and brk (artifacts/oracle/agentDRV-out, copies of the sampler files).
    struct Sample {
        int updates;
        double speed;
    };
    const Sample samples[] = {{1, 2.1}, {5, 3.44}, {10, 4.86}, {20, 8.57}, {30, 12.73}, {40, 16.54}, {50, 19.84}, {60, 22.8}, {70, 25.1}, {80, 26.45}};
    DriveCommand accelerate;
    accelerate.accelerate = true;
    for (const Sample& sample : samples) near(run({}, accelerate, sample.updates).speed, sample.speed, 1.0, "speed after A held from standstill");
    near(run({}, accelerate, 30).position[2], 7.07, 1.0, "distance 30 updates after the first move (oracle pcA 7.09, pcB 7.04, coast 7.09)");
    near(run({}, accelerate, 60).position[2], 25.0, 2.0, "distance 60 updates after the first move (oracle pcA 25.5, coast 24.3)");
    near(run({}, accelerate, 300).speed, kPoliceTopSpeed, 1e-4, "the speed ends at the class value TopSpeed 28 (oracle: 27.3 to 28.6 at the top)");
    near(run({}, accelerate, 300, 30.0f).speed, 30.0, 1e-4, "the top speed is the vehicle's own class value");
}

void brakingAgainstOracle() {
    // Oracle run pcB: B pressed at 18.24 m/s: 6.47 after 3 updates, 3.41 after 6, 0.88 after 8, stopped after 10. Run brk: B at 25.66 m/s: 13.83
    // after 3 updates, 8.17 after 6, 4.0 after 10, 0.42 after 14 (both runs lose 11.8 m/s in the first three updates).
    DriveBody body;
    body.speed = 18.24f;
    DriveCommand brake;
    brake.brake = true;
    near(run(body, brake, 3).speed, 6.47, 0.5, "pcB: 3 updates after B at 18.2 m/s");
    near(run(body, brake, 6).speed, 3.41, 0.5, "pcB: 6 updates");
    near(run(body, brake, 8).speed, 0.88, 1.0, "pcB: 8 updates");
    body.speed = 25.66f;
    near(run(body, brake, 3).speed, 13.83, 0.5, "brk: 3 updates after B at 25.7 m/s");
    near(run(body, brake, 6).speed, 8.17, 3.0, "brk: 6 updates");
    near(run(body, brake, 14).speed, 0.42, 3.0, "brk: 14 updates");
    DriveCommand both;
    both.accelerate = true;
    both.brake = true;
    body.speed = 18.38f;
    near(run(body, both, 3).speed, 7.06, 1.0, "both: A and B at 18.4 m/s brake (oracle both: 7.06 after 3 updates)");
    near(run(body, both, 8).speed, 1.35, 1.5, "both: 8 updates");
    DriveBody stopped;
    near(run(stopped, brake, 30).speed, -4.2, 0.8, "rev: speed 30 updates after B at standstill");
    near(run(stopped, brake, 60).speed, -7.83, 0.8, "rev: speed 60 updates after B at standstill");
    expect(run(stopped, brake, 600).speed >= -kDriveReverseSpeedLimit - 1e-4, "reverse speed stays within the largest measured value");
}

void coastingAgainstOracle() {
    // Oracle run forced (record +0x310 poked to 1 while A is held at 18.1 m/s): 11.39 after 2 updates, 8.86 after 3, 7.32 after 4, 5.49 after 5, 3.89 after 6,
    // 0.65 after 8. Run both (A and B held at 18.4 m/s): 7.06 after 3 updates, 5.62 after 4, 3.85 after 6, 1.35 after 8, then reverse. Oracle run coast: A released at 20.0 m/s: 19.19 after 2 updates, 17.87 after 6, 16.19 after 10, 12.77 after 20, 9.33 after 30, 5.98 after 40;
    // the car is at rest after 50. Run exitmv: hand brake from about 16 m/s: 4.91 after 6 updates, 2.87 after 8, 0.12 after 14.
    DriveBody body;
    body.speed = 20.0f;
    DriveCommand coast;
    near(run(body, coast, 2).speed, 19.19, 0.5, "coast: 2 updates after A released at 20 m/s");
    near(run(body, coast, 6).speed, 17.87, 0.5, "coast: 6 updates");
    near(run(body, coast, 10).speed, 16.19, 0.7, "coast: 10 updates");
    near(run(body, coast, 20).speed, 12.77, 1.0, "coast: 20 updates");
    near(run(body, coast, 30).speed, 9.33, 1.0, "coast: 30 updates");
    near(run(body, coast, 40).speed, 5.98, 1.0, "coast: 40 updates");
    near(run(body, coast, 80).speed, 0.0, 1e-6, "coast: at rest");
    DriveCommand handBrake;
    handBrake.handBrake = true;
    handBrake.brake = true;
    body.speed = 18.07f;
    near(run(body, handBrake, 2).speed, 11.39, 0.8, "forced: 2 updates after SetForcedStop at 18.1 m/s");
    near(run(body, handBrake, 4).speed, 7.32, 0.8, "forced: 4 updates");
    near(run(body, handBrake, 6).speed, 3.89, 0.8, "forced: 6 updates");
    near(run(body, handBrake, 9).speed, 0.0, 1.0, "forced: stopped 8 to 10 updates after the forced stop");
    body.speed = 16.0f;
    expect(run(body, handBrake, 6).speed < 5.5f, "exitmv: under 5.5 m/s 6 updates after the hand brake from 16 m/s (oracle 4.91)");
    near(run(body, handBrake, 14).speed, 0.0, 0.2, "exitmv: stopped 14 updates after the hand brake");
}

void steeringAgainstOracle() {
    // Oracle runs pcSt* (A held from standstill, constant stick) at update 20 (speed 8.6 m/s): yaw rate per steer value (stick raw 78, 100, 120, 140, 156).
    struct Sample {
        float steer;
        double yawRate;
    };
    const Sample samples[] = {{0.291f, 0.093}, {0.5238f, 0.253}, {0.7354f, 0.635}, {0.9471f, 0.93}, {1.0f, 1.007}};
    DriveCommand command;
    command.accelerate = true;
    for (const Sample& sample : samples) {
        command.steer = sample.steer;
        const DriveBody before = run({}, command, 20);
        const DriveBody after = run(before, command, 1);
        near((after.heading - before.heading) / kDt, sample.yawRate, 0.12, "yaw rate at 8.6 m/s");
    }
    command.steer = -1.0f;
    const DriveBody left = run({}, command, 20);
    expect(left.heading < 0.0f, "a negative steer turns the heading down");
    command.steer = 0.0f;
    near(run({}, command, 60).heading, 0.0, 1e-9, "no steer, no turn");
    command.steer = 1.0f;
    command.accelerate = false;
    near(run({}, command, 60).heading, 0.0, 1e-9, "no turn at standstill");
    DriveBody fast;
    fast.speed = 24.0f;
    command.accelerate = true;
    const DriveBody turned = run(fast, command, 10);
    near((turned.heading) / (10 * kDt), 1.5, 0.35, "full lock at 24 m/s: yaw rate about 1.5 rad/s (oracle hiL: 1.2 to 1.5)");
}

void stuckRule() {
    float timer = 0.0f;
    expect(!updateStuckTimer(timer, 1.0f, false, 0.0f, 3.0f), "an upright vehicle is not stuck");
    near(timer, 0.0, 1e-9, "an upright vehicle resets the timer");
    expect(!updateStuckTimer(timer, 0.1f, false, 0.0f, 1.0f), "tipped over, slow: 1.0 s is not enough");
    near(timer, 1.0, 1e-6, "the timer adds dt");
    expect(!updateStuckTimer(timer, 0.1f, false, 5.0f, 1.0f), "tipped over but moving fast: the timer stays");
    near(timer, 1.0, 1e-6, "a fast tipped vehicle keeps the timer");
    expect(!updateStuckTimer(timer, 0.1f, false, 0.0f, 1.0f), "exactly 2.0 s is not above 2.0");
    expect(updateStuckTimer(timer, 0.1f, false, 0.0f, 0.5f), "above 2.0 s requests state 8 (0x16ce9c)");
    expect(!updateStuckTimer(timer, 0.1f, true, 0.0f, 0.5f) && timer == 0.0f, "a frozen entity resets the timer");
}

void movement() {
    DriveBody body;
    body.heading = 0.0f;
    DriveCommand accelerate;
    accelerate.accelerate = true;
    body = run(body, accelerate, 30);
    expect(body.position[2] > 0.0f && std::fabs(body.position[0]) < 1e-5, "heading 0 drives along +z");
    DriveBody east;
    east.heading = float(M_PI) / 2.0f;
    east = run(east, accelerate, 30);
    expect(east.position[0] > 0.0f && std::fabs(east.position[2]) < 1e-3, "heading pi/2 drives along +x");
}

}

int main() {
    inputRules();
    stateRules();
    accelerationAgainstOracle();
    brakingAgainstOracle();
    coastingAgainstOracle();
    steeringAgainstOracle();
    stuckRule();
    movement();
    if (failures) {
        std::printf("%d failure(s)\n", failures);
        return 1;
    }
    std::printf("vehicle-drive tests passed\n");
    return 0;
}
