// Tests of src/runtime/traffic_driver_calls.*: VehicleSetRaceTrim 0x286080 with 0x26d8c4 and the release 0x22a1f4, the body placement
// 0x23091c, the contact slide 0x21a73c (mode 255) / 0x2c8f80, the chase markers 0x230dbc / 0x230e60, the whistle stop 0x2c87a8 / 0x26e254
// and the brake lights 0x278b78 / 0x2cb42c. Every expected value follows from the disassembly (llvm-objdump --triple=armv6k-none-eabi
// --mattr=+vfp2 of artifacts/game/game.arm.elf, read by 20261108-traffic-A on 2026-11-08, docs/re/traffic-ai-driver-calls.md): the
// literals 0x26e260 / 0x26e270 (60, 120), 0x2c9070 (0.5), 0x278d34 (2.0), 0x278d40 (255.0) and the constants of the zero vectors
// 0x56d3a0 / 0x56d400 (static initialiser 0x344140), evaluated by hand in single precision. No emulator was used.
#include "runtime/traffic_driver_calls.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

template <typename Function>
bool throwsLogicError(Function function) {
    try {
        function();
    } catch (const std::logic_error&) {
        return true;
    }
    return false;
}

void testRaceTrimNeedsDriver() {
    TrafficHeldCar held;
    TrafficRaceTrimCar car;
    car.entity = 7;
    const TrafficRaceTrimResult result = setTrafficRaceTrim(car, 1.0f, held);
    require(!result.hideTargetHearts && held.entity == 0, "VehicleSetRaceTrim does nothing without a driver record");
}

void testRaceTrimOnHoldsFirstCar() {
    TrafficHeldCar held;
    AiDriverRecord driver;
    driver.flags = kAiDriverFlagAi;
    TrafficSteeringState steering;
    TrafficRaceTrimCar car;
    car.entity = 7;
    car.driver = &driver;
    car.steering = &steering;
    car.meshNode = true;
    car.entityFade = 3;
    car.entityFadeFlags = 0xff;
    const TrafficRaceTrimResult result = setTrafficRaceTrim(car, -0.0f, held);
    require(result.hideTargetHearts, "the race trim switches the target hearts off (0x230f04(0))");
    require(driver.flags == (kAiDriverFlagAi | kAiDriverFlagFullSpeed), "a flag of -0.0 has bits, so +0xaf bit 2 is set");
    require(steering.launch == 1, "0x26d8c4 sets the launch byte +0x7a");
    require(held.entity == 7, "the first car is held in 0x3745d4");
    require(car.entityFade == 255 && car.entityFadeFlags == 0xcf, "a mesh node: entity +0xdd = 255, +0xde loses bits 4 and 5");

    AiDriverRecord second;
    TrafficSteeringState secondSteering;
    TrafficRaceTrimCar other;
    other.entity = 9;
    other.driver = &second;
    other.steering = &secondSteering;
    other.meshNode = true;
    other.entityFadeFlags = 0x30;
    setTrafficRaceTrim(other, 2.0f, held);
    require(secondSteering.launch == 1 && held.entity == 7, "a second car gets the launch byte but is not held");
    require(other.entityFade == 0 && other.entityFadeFlags == 0x30, "a car that is not held keeps its fade bytes");
}

void testRaceTrimOffReleases() {
    TrafficHeldCar held;
    AiDriverRecord driver;
    TrafficSteeringState steering;
    TrafficRaceTrimCar car;
    car.entity = 7;
    car.driver = &driver;
    car.steering = &steering;
    car.meshNode = false;
    setTrafficRaceTrim(car, 1.0f, held);
    driver.flags = static_cast<std::uint8_t>(kAiDriverFlagAi | kAiDriverFlagFullSpeed | kAiDriverFlagSignal);
    driver.requestedState = kAiStateSteer;
    car.nodeFadeFlags = 0x01;
    const TrafficRaceTrimResult result = setTrafficRaceTrim(car, 0.0f, held);
    require(!result.hideTargetHearts, "clearing the trim does not touch the hearts");
    require(driver.flags == kAiDriverFlagSignal, "the release clears +0xaf bits 2 and 0");
    require(driver.requestedState == kAiStateHold && car.vehicleRequest == 4 && held.entity == 0,
            "the release requests state 0, the vehicle request 4 and frees 0x3745d4");
    require(car.nodeFadeFlags == 0x01, "without a mesh node the node fade bits stay");
    require(steering.launch == 1, "the release leaves the launch byte");

    TrafficRaceTrimCar other = car;
    other.entity = 9;
    AiDriverRecord otherDriver;
    otherDriver.flags = kAiDriverFlagAi;
    other.driver = &otherDriver;
    held.entity = 7;
    releaseTrafficHeldCar(other, held);
    require(otherDriver.flags == kAiDriverFlagAi && held.entity == 7, "0x22a1f4 acts only on the held car");

    car.meshNode = true;
    car.nodeFadeFlags = 0x00;
    releaseTrafficHeldCar(car, held);
    require(car.nodeFadeFlags == 0x30 && held.entity == 0, "a held mesh node gets node +0xde bits 4 and 5");

    TrafficRaceTrimCar orphan;
    orphan.entity = 5;
    held.entity = 5;
    require(throwsLogicError([&] { releaseTrafficHeldCar(orphan, held); }), "a held car without a driver record is refused");
}

TrafficPlacedBody movingBody() {
    TrafficPlacedBody body;
    body.flags = 0x000880ff;
    body.rigid.velocity = {1.0f, 2.0f, 3.0f};
    body.rigid.angularVelocity = {0.5f, 0.25f, 0.125f};
    body.rigid.force = {4.0f, 5.0f, 6.0f};
    body.rigid.torque = {7.0f, 8.0f, 9.0f};
    body.rigid.deactivationTime = 0.4f;
    body.activationState = 2;
    body.interpolationVelocity = {1.0f, 1.0f, 1.0f};
    body.interpolationAngularVelocity = {2.0f, 2.0f, 2.0f};
    body.idleRpm = 800.0f;
    body.drivetrain.rpm = 3500.0f;
    body.field28c = 0.75f;
    body.drivetrain.wheels.resize(4);
    for (WheelSpinState& wheel : body.drivetrain.wheels) wheel.rate = 12.0f;
    body.groundedWheels = 4;
    body.nodeParent = 100;
    return body;
}

AiMatrix matrixAt(const AiVec3& position) {
    AiMatrix m{};
    m[0] = {1.0f, 0.0f, 0.0f};
    m[1] = {0.0f, 1.0f, 0.0f};
    m[2] = {0.0f, 0.0f, 1.0f};
    m[3] = position;
    return m;
}

void testPlacementWithoutRoom() {
    TrafficPlacedBody body = movingBody();
    TrafficPlacementHooks hooks;
    hooks.roomAt = [](const AiVec3&) { return 0u; };
    const TrafficPlacementResult result = placeTrafficBody(body, matrixAt({1.0f, 2.0f, 3.0f}), 0, true, hooks);
    require(!result.placed, "no room at the point: 0x23091c returns 0");
    require(body.flags == 0x000000ff, "the flags +0xc4 lose 0x88000 before the room search");
    require(body.rigid.velocity[0] == 1.0f && body.groundedWheels == 4, "nothing else changes without a room");
}

void testPlacementDynamic() {
    TrafficPlacedBody body = movingBody();
    AiVec3 asked{};
    TrafficPlacementHooks hooks;
    hooks.roomAt = [&](const AiVec3& point) {
        asked = point;
        return 3u;
    };
    hooks.roomNode = [](std::uint32_t room) { return room == 3u ? 200u : 0u; };
    const TrafficPlacementResult result = placeTrafficBody(body, matrixAt({10.0f, 20.0f, 30.0f}), 0, true, hooks);
    require(result.placed && result.room == 3 && asked[1] == 20.0f, "the room comes from the matrix position");
    require(result.movedToMatrix && !result.resetFromNode, "a dynamic body takes the matrix");
    require(body.rigid.velocity[0] == 0.0f && body.rigid.angularVelocity[2] == 0.0f, "0x229920 and 0x21315c store the zero vectors");
    require(body.rigid.force[1] == 0.0f && body.rigid.torque[2] == 0.0f, "rigid +0x1a0..+0x1bc are cleared");
    require(body.interpolationVelocity[0] == 0.0f && body.interpolationAngularVelocity[2] == 0.0f, "rigid +0x84..+0xa0 are cleared");
    require(body.drivetrain.rpm == 800.0f && body.field28c == 0.0f, "+0x27c = +0x26c and +0x28c = 0");
    for (const WheelSpinState& wheel : body.drivetrain.wheels) require(wheel.rate == 0.0f, "every wheel spin w+0x88 is 0");
    require(body.groundedWheels == 0, "+0xda0 = 0");
    require(body.activationState == 1 && body.rigid.deactivationTime == 0.0f, "0x2308e4(rigid, 0) wakes a dynamic body");
    require(result.queueCommand9, "0x23f0d4 queues command 9");
    require(result.relinked && body.nodeParent == 200, "the node moves to the room's node");
    require(result.trackCell, "the cell tracker runs while 0x376c35 is set");
}

void testPlacementKinematicInRoom() {
    TrafficPlacedBody body = movingBody();
    body.rigidFlags = 0x102;
    body.nodeParent = 200;
    TrafficPlacementHooks hooks;
    hooks.roomNode = [](std::uint32_t) { return 200u; };
    const TrafficPlacementResult result = placeTrafficBody(body, matrixAt({0.0f, 0.0f, 0.0f}), 3, false, hooks);
    require(result.placed && result.resetFromNode && !result.movedToMatrix, "a kinematic body is reset from its node");
    require(body.activationState == 2 && body.rigid.deactivationTime == 0.4f, "0x2308e4(rigid, 0) leaves a kinematic body");
    require(!result.relinked && !result.trackCell, "the same room keeps the link; no tracker without 0x376c35");

    TrafficPlacedBody asleep = movingBody();
    asleep.activationState = 4;
    placeTrafficBody(asleep, matrixAt({0.0f, 0.0f, 0.0f}), 3, false, hooks);
    require(asleep.activationState == 4 && asleep.rigid.deactivationTime == 0.0f, "states 4 and 5 stay, the time is reset");
}

TrafficContactEntry entryOf(std::uint32_t owner, std::uint32_t word = 0) {
    TrafficContactEntry entry;
    entry.word = word;
    entry.owner = owner;
    entry.ownerNode = owner + 1000;
    entry.ownerNodeParent = 1;
    return entry;
}

void testContactSlide() {
    TrafficContactCar car;
    car.node = 50;
    car.nodeParent = 51;
    car.radius = 2.0f;
    std::vector<TrafficContactEntry> entries = {entryOf(1), entryOf(2, kTrafficContactSkipBit), entryOf(3), entryOf(4), entryOf(5)};
    entries[3].ownerNodeParent = 50;
    entries[4].ownerNode = 0;
    std::vector<std::size_t> tried;
    std::vector<std::uint32_t> marksSeen;
    TrafficContactSlideHooks hooks;
    hooks.ignorePair = [](std::uint32_t owner) { return owner == 3u; };
    hooks.push = [&](AiVec3&, std::size_t entry, std::vector<TrafficContactEntry>& list, bool slide) {
        require(slide, "a car without hword +0x10 bit 0x100 slides");
        tried.push_back(entry);
        marksSeen.push_back(list[entry].word);
        return true;
    };
    AiVec3 move = {3.0f, 0.0f, 4.0f};
    require(slideTrafficCarContacts(car, move, entries, hooks), "a push makes 0x2c8f80 return 1");
    require(tried.size() == 2 && tried[0] == 0 && tried[1] == 2, "skip bit, attached nodes and owners without a node are not tried");
    require(marksSeen[1] == kTrafficContactMarkBit, "an ignored pair carries the mark while 0x2c8f80 runs");
    require(entries[2].word == 0, "the marks are cleared at the end");
    require(std::fabs(move[0] - 0.6f) < 1e-6f && std::fabs(move[2] - 0.8f) < 1e-6f, "the move of length 5 is cut to 0.5 x radius 2");

    car.pusher = true;
    tried.clear();
    hooks.push = [&](AiVec3&, std::size_t entry, std::vector<TrafficContactEntry>&, bool slide) {
        require(!slide, "a pusher passes slide 0");
        tried.push_back(entry);
        return true;
    };
    std::vector<TrafficContactEntry> two = {entryOf(1), entryOf(2)};
    AiVec3 small = {0.1f, 0.0f, 0.0f};
    require(slideTrafficCarContacts(car, small, two, hooks) && tried.size() == 1, "a pusher stops after the first push");
    require(small[0] == 0.1f, "a move within half the radius stays");

    hooks.push = [](AiVec3&, std::size_t, std::vector<TrafficContactEntry>&, bool) { return false; };
    AiVec3 none = {30.0f, 0.0f, 0.0f};
    require(!slideTrafficCarContacts(car, none, two, hooks) && none[0] == 30.0f, "without a push the move is not cut");
}

void testChaseMarkers() {
    ObjectiveState objectives;
    setTrafficChaseMarkers(objectives, "bits.waypointforcarryit", "car_stolen", true, true);
    setTrafficChaseMarkers(objectives, "bits.waypointforcarryit", "car_stolen", true, true);
    require(objectives.arrows()[1].size() == 1 && objectives.arrows()[1][0].target == "car_stolen", "one enemy arrow per car");
    const SubObjectiveGroup* group = objectives.group("bits.waypointforcarryit", 1);
    require(group != nullptr && group->targets.size() == 1, "the car joins the enemy group of the carry-it waypoint once");
    setTrafficChaseMarkers(objectives, "bits.waypointforcarryit", "car_stolen", false, true);
    require(objectives.arrows()[1].empty(), "leaving state 5 removes the arrow");
    group = objectives.group("bits.waypointforcarryit", 1);
    require(group == nullptr || group->targets.empty(), "and the group target");
}

void testWhistleStop() {
    TrafficWhistleStop stop;
    require(!trafficWhistleStops(stop, 0) && !trafficWhistleStops(stop, 5000), "after the reset nothing stops");
    GameRandom random(1234);
    GameRandom copy(1234);
    startTrafficWhistleStop(stop, 1000, random);
    const std::uint32_t expected = copy.next() % 60 + 1000 + 120;
    require(stop.until == expected, "until = clock + random % 60 + 120");
    require(trafficWhistleStops(stop, expected - 1) && !trafficWhistleStops(stop, expected), "traffic stops while the clock is below");
}

void testBrakeLights() {
    TrafficBrakeLights none;
    TrafficBrakeLightFrame braking;
    braking.brake = 1.0f;
    braking.dt = 1.0f / 30.0f;
    require(runTrafficBrakeLights(none, braking).parts.empty() && none.level == 0.0f, "no first light part: nothing");

    TrafficBrakeLights lights;
    lights.parts = {5, -1};
    lights.glowParts = {6, -1, 7, -1};
    TrafficBrakeLightResult result = runTrafficBrakeLights(lights, braking);
    require(lights.level == 1.0f, "braking in a forward gear lights at once");
    require(result.glowStarts.size() == 2 && result.glowStarts[0] == 0 && result.glowStarts[1] == 2, "glows start in the used slots");
    require(result.parts.size() == 1 && result.parts[0].part == 5 && result.parts[0].shown && result.parts[0].alpha == 255,
            "the light part is shown at alpha 255");
    lights.glows = {11, 0, 12, 0};
    result = runTrafficBrakeLights(lights, braking);
    require(result.glowStarts.empty() && result.parts.empty(), "an unchanged level shows nothing again; running glows stay");

    TrafficBrakeLightFrame rolling;
    rolling.dt = 0.125f;
    result = runTrafficBrakeLights(lights, rolling);
    require(lights.level == 0.75f && result.parts[0].alpha == 191, "the level falls by 2 x dt: (int)(255 x 0.75) = 191");
    require(result.glowStops.size() == 2, "the glows are stopped while the level is below 1");

    TrafficBrakeLightFrame reverse = braking;
    reverse.gear = 0;
    reverse.dt = 0.5f;
    result = runTrafficBrakeLights(lights, reverse);
    require(lights.level == 0.0f && result.parts.size() == 1 && !result.parts[0].shown, "in reverse the level falls to 0 and hides");

    TrafficBrakeLightFrame culled = braking;
    culled.nodeCulled = true;
    culled.hasBody = false;
    culled.gear = 0;
    result = runTrafficBrakeLights(lights, culled);
    require(lights.level == 1.0f && result.glowStarts.empty() && result.glowStops.size() == 2,
            "without a body the brake lights; a culled node gets no glow and its glows stop");

    endTrafficBrakeGlow(lights, 11, 1);
    require(lights.glows[0] == 11, "only event 2 ends a glow");
    endTrafficBrakeGlow(lights, 11, 2);
    require(lights.glows[0] == 0 && lights.glows[2] == 12, "the end call clears the matching slot");
}

}

int main() {
    try {
        testRaceTrimNeedsDriver();
        testRaceTrimOnHoldsFirstCar();
        testRaceTrimOffReleases();
        testPlacementWithoutRoom();
        testPlacementDynamic();
        testPlacementKinematicInRoom();
        testContactSlide();
        testChaseMarkers();
        testWhistleStop();
        testBrakeLights();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << "\n";
        return 1;
    }
    std::cout << "traffic driver calls: ok\n";
    return 0;
}
