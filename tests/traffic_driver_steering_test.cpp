// Tests of src/runtime/traffic_driver_steering.*: the steering controller 0x27872c, the contact step 0x29cc38 and the top gear speed
// 0x20362c. Every expected value follows from the disassembly (llvm-objdump --triple=armv6k-none-eabi --mattr=+vfp2 of
// artifacts/game/game.arm.elf, read by 20261107-traffic-C on 2026-11-07, docs/re/traffic-ai-driver-steering.md): the literals
// 0x278a98..0x278acc, 0x29cd90..0x29cd98 and 0x203670..0x203678, evaluated by hand in single precision in the order of the code (the
// controller update of 0x278960..0x278984, run up to three times by the clamp of 0x278a1c..0x278a74). No emulator was used.
#include "runtime/traffic_driver_steering.hpp"

#include "runtime/vehicle_drivetrain.hpp"

#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b) { return std::fabs(a - b) <= 1e-5f * std::fmax(1.0f, std::fabs(b)); }

std::uint32_t bits(float value) {
    std::uint32_t out = 0;
    std::memcpy(&out, &value, sizeof out);
    return out;
}

AiMatrix identityAt(const AiVec3& position) {
    AiMatrix m{};
    m[0] = {1.0f, 0.0f, 0.0f};
    m[1] = {0.0f, 1.0f, 0.0f};
    m[2] = {0.0f, 0.0f, 1.0f};
    m[3] = position;
    return m;
}

AiDriverRecord driverWith(float speed, float cruise, float target) {
    AiDriverRecord driver;
    driver.speed = speed;
    driver.cruiseSpeed = cruise;
    driver.targetSpeed = target;
    driver.heightOffset = 1.0f;
    return driver;
}

TrafficSteerFrame frameWith(float dt, float gearSpeed) {
    TrafficSteerFrame frame;
    frame.node = identityAt({0.0f, 0.0f, 0.0f});
    frame.dt = dt;
    frame.bodyBase = 0.25f;
    frame.gearSpeed = gearSpeed;
    return frame;
}

void testConstants() {
    require(bits(kTrafficStraightAngle) == 0x3db2b8c3u, "0x278aa8 holds 0x3db2b8c3");
    require(bits(kTrafficSpeedProportional) == 0x3f666666u, "0x278ac4 holds 0x3f666666");
    require(bits(kTrafficSpeedDerivative) == 0x3b03126fu, "0x278ac8 holds 0x3b03126f");
    require(bits(kAiSteerFactor) == 0x3f4ccccdu, "0x278ab8 holds 0x3f4ccccd");
}

void testGearSpeed() {
    require(near(vehicleTopGearSpeed(6000.0f, 7, 2.0f, 0.5f), 185.23541f), "0x20362c: 6000 / |0.848 x 2 x 1| x 0.10472 x 0.5");
    require(near(vehicleTopGearSpeed(6000.0f, 3, 2.0f, 0.5f), 48.631462f), "0x20362c with +0x298 = 3 takes the ratio 3.23 of index 2");
    const float finalDrive = engineFinalDrive(6000.0f, 28.0f, 0.45f);
    require(near(vehicleTopGearSpeed(6000.0f, kGearCount, finalDrive, 0.45f), 28.0f),
            "with the final drive of 0x2435d0 the gear speed is TopSpeed");
    bool refused = false;
    try {
        vehicleTopGearSpeed(6000.0f, 8, 2.0f, 0.5f);
    } catch (const std::out_of_range&) {
        refused = true;
    }
    require(refused, "a gear count outside the 7 ratios is refused");
}

void testStraight() {
    AiDriverRecord driver = driverWith(10.0f, 10.0f, 10.0f);
    driver.stateUpdates = 40;
    TrafficSteeringState state;
    const TrafficSteerResult result = steerTrafficCar(driver, state, identityAt({0.0f, 0.0f, 10.0f}), frameWith(0.5f, 40.0f));
    require(result.placements.empty(), "no placement below 10 up and while moving");
    require(driver.stateUpdates == 0, "+0xa4 = 0 above speed 5 (0x2787d0, 0x278830)");
    require(driver.targetSpeed == 10.0f, "within 5 degrees +0x70 = +0x6c (0x278928)");
    require(result.steer == 0.0f, "straight ahead the steer is 0");
    require(state.steer.proportional == 5.0f && state.steer.integralGain == 0.0f && state.steer.derivative == 1.0f,
            "+0x84 = 5, +0x88 = 0, +0x8c = 1 for a target speed of 0 or more");
    require(state.speed.proportional == 0.9f && state.speed.integralGain == 0.0f && state.speed.derivative == 0.002f,
            "+0x98 = 0.9, +0x9c = 0, +0xa0 = 0.002");
    require(!result.accelerate && result.brake && bits(*result.brake) == 0x80000000u, "an output of 0 writes the brake -0.0 (0x278a88)");
}

void testTurn() {
    AiDriverRecord driver = driverWith(25.0f, 30.0f, 30.0f);
    TrafficSteeringState state;
    const TrafficSteerResult result = steerTrafficCar(driver, state, identityAt({10.0f, 0.0f, 10.0f}), frameWith(0.1f, 40.0f));
    require(near(driver.targetSpeed, 25.606602f), "turning at 20 or more: +0x70 = min(+0x6c, 0x20362c x 0.8) x (0.5 + 0.5 z / |local|)");
    require(result.steer == 1.0f, "5 x 0.785398 + 1 x 0.785398 / 0.1 clamps to 1.0 (0x278990)");
    require(near(state.steer.previousError, 0.7853982f) && near(state.steer.integral, 0.07853982f), "+0x7c = the angle, +0x80 += angle x dt");
    require(result.accelerate && near(*result.accelerate, 0.054594155f) && !result.brake,
            "the speed output below 1 is the third update, 0.9 x (+0x70 - +0x68) x dt without the derivative");
    require(near(state.speed.previousError, 0.060660172f) && near(state.speed.integral, 0.018198052f),
            "+0x90 = the error, +0x94 advanced by three updates");

    AiDriverRecord full = driverWith(25.0f, 30.0f, 30.0f);
    full.flags = kAiDriverFlagFullSpeed;
    TrafficSteeringState fullState;
    const TrafficSteerResult slow = steerTrafficCar(full, fullState, identityAt({10.0f, 0.0f, 10.0f}), frameWith(0.1f, 20.0f));
    require(near(full.targetSpeed, 17.071068f), "with +0xaf bit 2 the gear speed 20 x 1.0 is below +0x6c and is taken");
    require(slow.brake && near(*slow.brake, 0.7136039f) && !slow.accelerate, "a negative output below 1 is the brake (0x278a88)");

    AiDriverRecord slowCar = driverWith(15.0f, 30.0f, 30.0f);
    TrafficSteeringState slowState;
    steerTrafficCar(slowCar, slowState, identityAt({10.0f, 0.0f, 10.0f}), frameWith(0.1f, 40.0f));
    require(slowCar.targetSpeed == 30.0f, "below speed 20 +0x70 = +0x6c (0x278898)");
}

void testPlacementsAndLaunch() {
    AiDriverRecord driver = driverWith(0.0f, 8.0f, 14.0f);
    driver.stateUpdates = 90;
    TrafficSteeringState state;
    state.launch = 1;
    const AiMatrix target = identityAt({0.0f, 12.0f, 10.0f});
    const TrafficSteerResult result = steerTrafficCar(driver, state, target, frameWith(0.1f, 40.0f));
    require(result.placements.size() == 2, "10 up and stuck for 90 updates: 0x23091c twice");
    require(result.placements[0] == target, "first the target itself (0x2787a0)");
    require(result.placements[1][3][1] == 11.25f && result.placements[1][3][2] == 10.0f,
            "then the target with y - 0x20af24 (1.0 - 0.25) (0x278820)");
    require(driver.stateUpdates == 0, "+0xa4 = 0 after the placement (0x278824)");
    require(driver.targetSpeed == 8.0f, "straight ahead +0x70 = +0x6c");
    require(state.launch == 1 && state.speed.proportional == 100.0f && state.speed.derivative == 0.0f,
            "+0x7a bit 0 below the target speed: gains 100 and 0, +0x7a kept");
    require(result.accelerate && *result.accelerate == 1.0f, "100 x 0.8 clamps to 1.0 after one update");
    require(near(state.speed.integral, 0.08f), "one update only when the output reaches 1.0");

    AiDriverRecord waiting = driverWith(0.0f, 8.0f, 14.0f);
    waiting.stateUpdates = 89;
    TrafficSteeringState waitingState;
    TrafficSteerFrame frame = frameWith(0.1f, 40.0f);
    const TrafficSteerResult early = steerTrafficCar(waiting, waitingState, identityAt({0.0f, 0.0f, 10.0f}), frame);
    require(early.placements.empty() && waiting.stateUpdates == 89, "below 90 updates nothing is placed and +0xa4 is kept");

    AiDriverRecord stopped = driverWith(0.0f, 8.0f, 14.0f);
    stopped.stateUpdates = 120;
    TrafficSteeringState stoppedState;
    frame.forcedStop = true;
    const TrafficSteerResult forced = steerTrafficCar(stopped, stoppedState, identityAt({0.0f, 0.0f, 10.0f}), frame);
    require(forced.placements.empty() && stopped.stateUpdates == 120, "record +0x310 set skips the stuck test");

    AiDriverRecord reached = driverWith(9.0f, 8.0f, 8.0f);
    TrafficSteeringState reachedState;
    reachedState.launch = 1;
    steerTrafficCar(reached, reachedState, identityAt({0.0f, 0.0f, 10.0f}), frameWith(0.1f, 40.0f));
    require(reachedState.launch == 0 && reachedState.speed.proportional == 0.9f, "at the target speed +0x7a = 0 (0x2789d8)");
}

void testReverse() {
    AiDriverRecord driver = driverWith(-3.0f, -5.0f, -5.0f);
    TrafficSteeringState state;
    const TrafficSteerResult result = steerTrafficCar(driver, state, identityAt({1.0f, 0.0f, -10.0f}), frameWith(1.0f, 40.0f));
    require(driver.targetSpeed == -5.0f, "below speed 20 +0x70 = +0x6c");
    require(state.steer.derivative == 5.0f, "a target speed below 0 gives +0x8c = 5");
    require(near(result.steer, 0.9966865f), "local x and z negated, atan2(-1, 10) x (5 + 5), negated (0x278988)");
    require(result.brake && *result.brake == 1.0f && !result.accelerate, "0.9 x -2 is -1 or less on the second update: brake 1.0");
    require(state.speed.integral == -4.0f, "two updates when the output reaches -1.0");

    AiDriverRecord fast = driverWith(12.0f, 10.0f, 10.0f);
    TrafficSteeringState fastState;
    const TrafficSteerResult slowing = steerTrafficCar(fast, fastState, identityAt({0.0f, 0.0f, 10.0f}), frameWith(0.5f, 40.0f));
    require(slowing.brake && near(*slowing.brake, 0.9f) && fastState.speed.integral == -1.5f, "-0.9 on the third update: brake 0.9");
}

void testContactStep() {
    AiDriverRecord driver;
    std::vector<std::string> log;
    TrafficContactHooks hooks;
    std::uint32_t listed = 2;
    hooks.gather = [&](const AiVec3& centre, const AiVec3& half, const TrafficContactDescriptor& descriptor, unsigned maxEntries) {
        require(descriptor.include == 4 && descriptor.exclude == 0 && descriptor.kindsInclude == 0xffffffffu && descriptor.kindsExclude == 0 &&
                    descriptor.ownerFlagsInclude == 0 && descriptor.ownerFlagsExclude == 16 && descriptor.ownerStateInclude == 512 &&
                    descriptor.ownerStateExclude == 3 && descriptor.mode == 1 && maxEntries == 128,
                "the descriptor words of 0x29ccb4..0x29cd04");
        log.push_back("gather " + std::to_string(centre[2]) + " " + std::to_string(half[0]) + " " + std::to_string(half[2]));
        return listed;
    };
    hooks.slide = [&](const AiVec3& moved, std::uint32_t count) {
        log.push_back("slide " + std::to_string(moved[0]) + " " + std::to_string(moved[2]) + " " + std::to_string(count));
    };

    runTrafficContactStep(driver, {3.0f, 0.0f, 4.0f}, 2.5f, hooks);
    require(driver.entryPosition == AiVec3{3.0f, 0.0f, 4.0f}, "driver +0x58 = the node position (0x29cc78)");
    require(log.size() == 2 && log[0] == "gather 4.000000 7.500000 7.500000" && log[1] == "slide 3.000000 4.000000 2",
            "moved 5: half extents 2.5 + 5, then 0x21a73c with the move");

    log.clear();
    listed = 0;
    runTrafficContactStep(driver, {3.0f, 0.0f, 24.0f}, 2.5f, hooks);
    require(log.size() == 1 && log[0] == "gather 24.000000 2.500000 2.500000", "a move of 20 or more adds nothing; nothing listed, no slide");

    const TrafficContactStep still = trafficContactStep(driver, {3.0f, 0.0f, 24.0f}, 2.5f);
    require(still.halfExtents == AiVec3{2.5f, 2.5f, 2.5f} && still.moved == AiVec3{0.0f, 0.0f, 0.0f}, "no move: entity +0x5c alone");

    bool refused = false;
    try {
        runTrafficContactStep(driver, {0.0f, 0.0f, 0.0f}, 2.5f, TrafficContactHooks{});
    } catch (const std::logic_error&) {
        refused = true;
    }
    require(refused, "missing hooks are refused");
}

}

int main() {
    try {
        testConstants();
        testGearSpeed();
        testStraight();
        testTurn();
        testPlacementsAndLaunch();
        testReverse();
        testContactStep();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << "\n";
        return 1;
    }
    std::cout << "traffic driver steering: all checks passed\n";
    return 0;
}
