// Tests of src/runtime/traffic_driver_states.*: the request of a driver state, VehicleSetThrowObjects 0x2ad940, the throw of state 5
// (0x254250, 0x1bc660, 0x28abd4), the input copy 0x23019c, the destroy message of state 7, the state 6 entry and the hooks of states 4 to 8
// connected through ai_driver_input. Every expected value follows from the disassembly (llvm-objdump --triple=armv6k-none-eabi --mattr=+vfp2
// of artifacts/game/game.arm.elf, read by 20261111-chase-B on 2026-11-12, docs/re/traffic-driver-states.md): the literals 0x1cb4e4 (1.0),
// 0x1cb6e0 (2.0), 0x1cb6e4 (-2.0), 0x1cb6e8 (0.5), 0x28ac98 (2.0), 0x28ac9c (1.0), 0x230568 (0.1), 0x1cb5b0 (1000) and the defaults of the
// create 0x1caec4 (speed 3.0, gravity 0.01), evaluated by hand in single precision. No emulator was used.
#include "runtime/traffic_driver_states.hpp"

#include "assets/assets.hpp"
#include "runtime/pedestrian_manager.hpp"
#include "runtime/traffic_driver_connect.hpp"
#include "runtime/vehicle_ai_natives.hpp"

#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using namespace chase::runtime;
namespace assets = chase::assets;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b) { return std::fabs(a - b) <= 1e-5f; }

AiMatrix identityAt(const AiVec3& position) {
    AiMatrix m{};
    m[0] = {1.0f, 0.0f, 0.0f};
    m[1] = {0.0f, 1.0f, 0.0f};
    m[2] = {0.0f, 0.0f, 1.0f};
    m[3] = position;
    return m;
}

void testRequest() {
    TrafficDrivers drivers;
    drivers.load({1.0f, 1.0f});
    requestTrafficDriverState(drivers, 1, kAiStateKnocked);
    require(drivers.car(1).driver.requestedState == kAiStateKnocked && drivers.car(0).driver.requestedState == 0,
            "the request writes +0x79 of that car only");
    bool threw = false;
    try {
        requestTrafficDriverState(drivers, 2, kAiStateKnocked);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    require(threw, "a car outside the table is refused");
}

void testThrowObjectsFlag() {
    setVehicleThrowObjects(nullptr, 1.0f);
    AiDriverRecord driver;
    driver.flags = kAiDriverFlagAi;
    setVehicleThrowObjects(&driver, -0.0f);
    require(driver.flags == (kAiDriverFlagAi | kAiDriverFlagEffect), "-0.0 has bits: +0xaf bit 1 set");
    setVehicleThrowObjects(&driver, 0.0f);
    require(driver.flags == kAiDriverFlagAi, "0.0 clears +0xaf bit 1");
}

void testThrowTarget() {
    AiMatrix node{};
    node[0] = {0.0f, 0.0f, -1.0f};
    node[1] = {0.0f, 1.0f, 0.0f};
    node[2] = {1.0f, 0.0f, 0.0f};
    node[3] = {10.0f, 2.0f, 5.0f};
    const AiVec3 target = vehicleThrowTarget({0.5f, 0.5f, -2.0f}, node);
    require(near(target[0], 8.0f) && near(target[1], 2.5f) && near(target[2], 4.5f), "0x254250: rows by the point plus the translation");
}

void testLaunch() {
    const AiVec3 flat = vehicleThrowLaunch({0, 0, 0}, {3, 0, 4}, 3.0f, 0.01f);
    // h = 5, tangent = 0 + 0.01 x 5 / (9 x 2) = 0.0027777778
    require(near(flat[0], 1.8f) && near(flat[2], 2.4f) && near(flat[1], 3.0f * (0.05f / 18.0f)), "a flat throw keeps the speed");
    const AiVec3 steep = vehicleThrowLaunch({0, 0, 0}, {0, 1, 2}, 1.0f, 2.0f);
    // h = 2, dy = 1: tangent 0.5 + 2 x 2 / 2 = 2.5 > 1, speed = sqrt(2 x 2 x 2 / (1 x 2)) = 2, tangent 1
    require(near(steep[0], 0.0f) && near(steep[1], 2.0f) && near(steep[2], 2.0f), "a steep throw goes at 45 degrees with a new speed");
    const AiVec3 high = vehicleThrowLaunch({0, 0, 0}, {0, 5, 2}, 1.0f, 2.0f);
    require(near(high[1], 1.0f) && near(high[2], 1.0f), "a target higher than far keeps the speed at 45 degrees");
    const AiVec3 above = vehicleThrowLaunch({1, 0, 1}, {1, -1, 1}, 1.0f, 2.0f);
    require(near(above[0], 0.0f) && near(above[2], 1.0f), "no horizontal distance gives the direction (0, 1)");
}

void testThrowObject() {
    std::vector<VehicleThrowObject> objects(3);
    objects[0].disabled = false;
    objects[1].maxTime = 0x45;
    objects[1].frozen = true;
    objects[1].centre = {0.0f, 1.0f, 0.0f};
    objects[1].parent = 4;
    const AiMatrix node = identityAt({0.0f, 0.0f, 0.0f});
    const VehicleThrowResult result = throwVehicleObject(objects, {3.0f, 1.0f, 4.0f}, 9, 7, node, 1.0f);
    require(result.thrown == std::size_t(1) && result.relinked, "the first disabled object is thrown and linked to the car's parent");
    const VehicleThrowObject& thrown = objects[1];
    require(!thrown.disabled && !thrown.frozen && thrown.timer == 0x45 && thrown.owner == 9 && thrown.parent == 7, "enabled, timer, owner");
    require(near(thrown.velocity[0], 1.8f) && near(thrown.velocity[2], 2.4f), "launched at the target from its centre");
    require(objects[2].disabled, "only one object per throw");
    std::vector<VehicleThrowObject> none(1);
    none[0].disabled = false;
    require(!throwVehicleObject(none, {}, 1, 1, node, 1.0f).thrown, "no free object: nothing thrown");
}

void testCopyInputs() {
    AiDriverVehicle vehicle;
    vehicle.steerInput = 0.25f;
    vehicle.accelerateInput = 0.75f;
    BodyInputs inputs = copyVehicleRecordInputs(vehicle, 2, 1, 10.0f);
    require(inputs.steer == 0.25f && inputs.accelerate == 0.75f && inputs.brake == 0.0f && inputs.handBrake == 0.0f, "state 2 copies as is");
    require(vehicle.steerInput == 0.0f && vehicle.accelerateInput == 0.0f, "the record inputs are cleared");
    inputs = copyVehicleRecordInputs(vehicle, 11, 1, 10.0f);
    require(inputs.handBrake == 1.0f, "state 11 sets the hand brake");
    vehicle.forcedStop = 1;
    vehicle.accelerateInput = 1.0f;
    vehicle.steerInput = -0.5f;
    inputs = copyVehicleRecordInputs(vehicle, 2, 1, 10.0f);
    require(inputs.accelerate == 0.0f && inputs.brake == 1.0f && inputs.handBrake == 1.0f && inputs.steer == -0.5f, "forced stop");
    inputs = copyVehicleRecordInputs(vehicle, 2, 1, 0.1f);
    require(inputs.brake == 0.0f, "forced stop at 0.1 or less releases the brake");
    inputs = copyVehicleRecordInputs(vehicle, 2, 0, 10.0f);
    require(inputs.brake == 0.0f, "forced stop in gear 0 releases the brake");
}

void testDestroyAndStopping() {
    const DamageMessage message = trafficDestroyMessage();
    require(message.hearts == 1000 && message.kind == 0 && message.type == 0 && message.fraction == 0.0f, "state 7: message 0 with 1000");
    std::uint8_t forcedStop = 0;
    const TrafficStoppingEntry entry = enterTrafficStopping(forcedStop, false);
    require(forcedStop == 1 && entry.event == "vehiclechaseended" && entry.smokeEffect == 0x11f && !entry.globalEvent, "state 6 entry");
}

struct Harness {
    AiDriverRecord driver;
    AiDriverVehicle vehicle;
    TrafficSteeringState steering;
    AiMatrix node = identityAt({0.0f, 0.0f, 0.0f});
    bool moved = false;
    std::vector<TrafficStateEvent> events;
    int copies = 0;
    int gravities = 0;
    int wakes = 0;
    int gathers = 0;
    std::vector<bool> markers;
    AiDriverHooks hooks;

    Harness() {
        TrafficStateContext context;
        context.driver = &driver;
        context.vehicle = &vehicle;
        context.steering = &steering;
        context.node = &node;
        context.moved = &moved;
        context.gearSpeed = 40.0f;
        context.radius = 2.0f;
        context.copyInputs = [this] { ++copies; };
        context.normalGravity = [this] { ++gravities; };
        context.wakeBody = [this] { ++wakes; };
        context.chaseMarkers = [this](bool on) { markers.push_back(on); };
        context.contacts.gather = [this](const AiVec3&, const AiVec3&, const TrafficContactDescriptor&, unsigned) -> std::uint32_t {
            ++gathers;
            return 0;
        };
        context.contacts.slide = [](const AiVec3&, std::uint32_t) {};
        context.events = &events;
        connectTrafficDriverStates(hooks, context);
        hooks.setKinematic = [](bool) {};
        hooks.snapTarget = [] {};
        hooks.stopAll = [] { return false; };
        hooks.lights = [](bool, float) {};
        hooks.nodeHidden = [] { return true; };
    }
};

AiRouteFunctions straightRoute() {
    AiRouteFunctions routes;
    routes.evaluate = [](std::uint32_t, float distance, AiVec3* position, AiVec3* direction) {
        if (position) *position = {0.0f, 0.0f, distance};
        if (direction) *direction = {0.0f, 0.0f, 1.0f};
    };
    routes.ahead = [](std::uint32_t, float ahead, float, float from) { return from + ahead; };
    routes.nearest = [](std::uint32_t, const AiVec3& position, unsigned, float) { return position[2]; };
    routes.length = [](std::uint32_t) { return std::uint16_t(100); };
    routes.signal = [](std::uint32_t) { return 0u; };
    return routes;
}

void testChaseStates() {
    Harness h;
    GameRandom random(1);
    const AiRouteFunctions routes = straightRoute();
    h.driver.flags = kAiDriverFlagAi | kAiDriverFlagEffect;
    h.driver.requestedState = kAiStateSteer;
    h.driver.cruiseSpeed = 29.0f;
    h.driver.targetSpeed = 29.0f;
    h.driver.route = 1;

    AiDriverStateFrame change;
    change.node = h.node;
    change.routes = &routes;
    runAiDriverStateChange(h.driver, h.vehicle, change, h.hooks, random);
    require(h.driver.state == kAiStateSteer && h.markers == std::vector<bool>{true}, "entering 5 lists the car (chase markers on)");
    require(h.events.size() == 1 && h.events[0].kind == TrafficStateEventKind::Throw, "update 0 of state 5 with +0xaf bit 1 throws");

    AiDriverInputFrame input;
    input.node = h.node;
    input.dt = 1.0f / 30.0f;
    input.gearSpeed = 40.0f;
    input.routes = &routes;
    runAiDriverInput(h.driver, h.vehicle, input, h.hooks);
    require(h.copies == 1 && h.vehicle.steerInput == 0.0f && h.vehicle.accelerateInput > 0.0f, "state 5 steers ahead and copies the inputs");

    AiDriverPostFrame post;
    post.node = h.node;
    post.dt = input.dt;
    post.bodySpeed = 3.0f;
    post.routes = &routes;
    runAiDriverPostStep(h.driver, h.vehicle, post, h.hooks, random);
    require(h.driver.speed == 3.0f && h.gravities == 1 && h.gathers == 1, "state 5 post step: speed, gravity, contact step");

    h.driver.requestedState = kAiStateStopping;
    runAiDriverStateChange(h.driver, h.vehicle, change, h.hooks, random);
    require(h.markers == std::vector<bool>({true, false}), "leaving 5 takes the markers off");
    require(h.events.back().kind == TrafficStateEventKind::Stopping && h.vehicle.forcedStop == 1, "entering 6 stops the car");

    h.driver.stateUpdates = 90;
    post.bodySpeed = 0.1f;
    runAiDriverPostStep(h.driver, h.vehicle, post, h.hooks, random);
    require(h.driver.requestedState == kAiStateBreak, "state 6 at rest after 90 updates requests 7");
    runAiDriverStateChange(h.driver, h.vehicle, change, h.hooks, random);
    require(h.events.back().kind == TrafficStateEventKind::Destroy && h.events.back().message.hearts == 1000, "entering 7 destroys the car");
}

void testKnockedWakes() {
    Harness h;
    GameRandom random(1);
    h.driver.state = kAiStateKnocked;
    h.driver.requestedState = kAiStateKnocked;
    AiDriverStateFrame change;
    change.activationState = 2;
    runAiDriverStateChange(h.driver, h.vehicle, change, h.hooks, random);
    require(h.wakes == 1, "state 4 wakes a sleeping body");
}

void testPlacement() {
    Harness h;
    AiMatrix target = identityAt({0.0f, 12.0f, 8.0f});
    h.hooks.steerToward(target, 1.0f / 30.0f);
    require(h.moved && h.node[3][1] == 12.0f && h.events.size() == 1 && h.events[0].kind == TrafficStateEventKind::Placed,
            "a target 10 or more above the car places it there");
}

int shipped(const std::string& fib, const std::string& csv) {
    if (!std::ifstream(fib) || !std::ifstream(csv)) {
        std::cout << "game data not extracted; skipping\n";
        return 77;
    }
    assets::FuseArchive archive(fib);
    archive.loadNames(csv);
    const assets::FuseEntry* entry = archive.find("levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl");
    require(entry != nullptr, "lvl_01 not found");
    const assets::Level level = assets::parseLevel(archive.read(*entry));
    const AiRouteFunctions routes = levelPathRouteFunctions(level);
    const std::uint32_t route = assets::nameHash("1-4_Vehicle_Path");
    const assets::LevelPath* path = nullptr;
    for (const assets::LevelPath& candidate : level.paths)
        if (candidate.nameHash == route) path = &candidate;
    require(path != nullptr, "lvl_01 has 1-4_Vehicle_Path");
    // Level data of lvl_01 (C++ loader, 20261111-chase-B, 2026-11-12): 1-4_Vehicle_Path is a closed type 0 path of 70 points.
    require(path->points.size() == 70 && path->closed && path->type == 0, "1-4_Vehicle_Path: closed type 0 path of 70 points");
    require(routes.length(route) == 70, "the route length is the point count");

    AiVec3 start{};
    AiVec3 tangent{};
    routes.evaluate(route, 5.0f, &start, &tangent);
    const AiMatrix node = pedestrianPlacement(start, tangent);
    VehicleAiNatives natives;
    natives.setRouteNearest(routes.nearest);
    natives.setChased("1-4_stolen_vehicle", node, route, 29.0f);
    VehicleAiRecord& record = *natives.mutableRecord("1-4_stolen_vehicle");
    require(record.driver.routeDistance == 5.0f, "VehicleSetChased: +0x64 is the nearest sample 0x21d5b0, path point 5");

    std::vector<bool> markers;
    ScriptCarFrame frame;
    frame.node = node;
    frame.dt = 1.0f / 30.0f;
    frame.gearSpeed = 40.0f;
    frame.vehicleState = 2;
    frame.routes = &routes;
    frame.snapTarget = [] {};
    frame.chaseMarkers = [&markers](bool on) { markers.push_back(on); };
    ScriptCarDriver car{&record.driver, &record.vehicle, &record.steering, &record.lights, &record.kinematic, &record.bodyInputs};
    GameRandom random(1);
    ScriptCarStep step = runScriptCarDriver(car, frame, random);
    require(record.driver.state == kAiStateSteer && markers == std::vector<bool>{true} && step.events.empty() && !step.node,
            "the first update enters state 5 and lists the car");
    step = runScriptCarDriver(car, frame, random);
    require(record.driver.routeDistance > 5.0f && record.driver.routeDistance < 70.0f, "state 5 steers to the route 8 ahead of the car");
    require(record.bodyInputs.accelerate > 0.0f && record.bodyInputs.brake == 0.0f && std::fabs(record.bodyInputs.steer) <= 1.0f &&
                record.vehicle.accelerateInput == 0.0f && record.vehicle.steerInput == 0.0f,
            "from rest the car accelerates toward its target speed 29, and the input copy clears the record inputs");
    frame.bodySpeed = 3.0f;
    step = runScriptCarPostStep(car, frame, random);
    require(record.driver.speed == 3.0f && step.events.empty(), "the post step of state 5 takes the body speed");
    std::cout << "traffic-driver-states: lvl_01 ok\n";
    return 0;
}

}

int main(int argc, char** argv) {
    try {
        testRequest();
        testThrowObjectsFlag();
        testThrowTarget();
        testLaunch();
        testThrowObject();
        testCopyInputs();
        testDestroyAndStopping();
        testChaseStates();
        testKnockedWakes();
        testPlacement();
        if (argc == 3) return shipped(argv[1], argv[2]);
    } catch (const std::exception& error) {
        std::cerr << "traffic-driver-states: " << error.what() << '\n';
        return 1;
    }
    std::cout << "traffic-driver-states: ok\n";
    return 0;
}
