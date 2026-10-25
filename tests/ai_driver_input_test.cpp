// Tests of src/runtime/ai_driver_input.*: the AI driver of the traffic cars. Every expected value follows from the disassembly
// (llvm-objdump --triple=armv6k-none-eabi --mattr=+vfp2 of artifacts/game/game.arm.elf, read by 20261025-traffic-B, docs/re/traffic.md): the
// record 0x1796e8, the input provider 0x26d934, the state change 0x1cb148, the post step 0x1bc79c. The spawn order 0 -> 1 -> 2 one update each
// was also measured by 20261025-traffic-servant (idle job TRAFFIC-2, recorded from a save state on a street with traffic after mission 1-1, 2026-10-25, RPC reads of driver +0x78 / +0x79).
#include "runtime/ai_driver_input.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b) { return std::fabs(a - b) <= 1e-5f; }

AiMatrix matrixAt(const AiVec3& position, const AiVec3& forward) {
    AiMatrix m{};
    m[0] = {forward[2], 0.0f, -forward[0]};
    m[1] = {0.0f, 1.0f, 0.0f};
    m[2] = forward;
    m[3] = position;
    return m;
}

struct Calls {
    std::vector<std::string> log;
    AiMatrix steerTarget{};
    float steerDt = 0.0f;
    AiMatrix node{};
    AiVec3 effect{};
    bool signalPass = true;
    bool nextResult = true;
    std::uint32_t nextRoute = 0;
    bool stopAtEnd = false;
    bool hidden = false;
    bool followResult = true;
    float moveTo = -1.0f;
};

AiRouteFunctions straightRoute(Calls& calls) {
    AiRouteFunctions routes;
    routes.evaluate = [&calls](std::uint32_t, float distance, AiVec3* position, AiVec3* direction) {
        calls.log.push_back("evaluate " + std::to_string(distance));
        *position = {0.0f, 0.0f, distance};
        if (direction != nullptr) *direction = {0.0f, 0.0f, 1.0f};
    };
    routes.ahead = [&calls](std::uint32_t, float ahead, float step, float from) {
        calls.log.push_back("ahead " + std::to_string(ahead) + " " + std::to_string(step));
        return from + ahead;
    };
    routes.nearest = [&calls](std::uint32_t, const AiVec3& position, unsigned samples, float scale) {
        calls.log.push_back("nearest " + std::to_string(samples) + " " + std::to_string(scale));
        return position[2];
    };
    routes.follow = [&calls](const AiVec3& probe, AiMatrix& matrix, AiVec3&, std::uint32_t&, float& distance) {
        calls.log.push_back("follow");
        matrix[2] = {0.0f, 0.0f, 1.0f};
        distance = probe[2];
        return calls.followResult;
    };
    routes.length = [](std::uint32_t) { return std::uint16_t{50}; };
    routes.signal = [](std::uint32_t route) { return route * 100u; };
    return routes;
}

AiDriverHooks recordingHooks(Calls& calls) {
    AiDriverHooks hooks;
    hooks.steerToward = [&calls](const AiMatrix& target, float dt) {
        calls.log.push_back("steer");
        calls.steerTarget = target;
        calls.steerDt = dt;
    };
    hooks.copyInputs = [&calls] { calls.log.push_back("copyInputs"); };
    hooks.setNodeMatrix = [&calls](const AiMatrix& m) {
        calls.log.push_back("setNode");
        calls.node = m;
    };
    hooks.signalLetsPass = [&calls](std::uint32_t) {
        calls.log.push_back("signalTest");
        return calls.signalPass;
    };
    hooks.leaveSignalQueue = [&calls](std::uint32_t) { calls.log.push_back("leaveSignal"); };
    hooks.setKinematic = [&calls](bool on) { calls.log.push_back(on ? "kinematic" : "dynamic"); };
    hooks.setListed = [&calls](bool on) { calls.log.push_back(on ? "listed" : "unlisted"); };
    hooks.snapTarget = [&calls] { calls.log.push_back("snapTarget"); };
    hooks.chooseNextRoute = [&calls](AiDriverRecord&, std::uint32_t& next) {
        calls.log.push_back("chooseNext");
        next = calls.nextRoute;
        return calls.nextResult;
    };
    hooks.leaveTraffic = [&calls] { calls.log.push_back("leaveTraffic"); };
    hooks.enterStopping = [&calls] { calls.log.push_back("enterStopping"); };
    hooks.destroy = [&calls] { calls.log.push_back("destroy"); };
    hooks.wakeBody = [&calls] { calls.log.push_back("wake"); };
    hooks.rearEffect = [&calls](const AiVec3& local) {
        calls.log.push_back("rearEffect");
        calls.effect = local;
    };
    hooks.stopAll = [&calls] {
        calls.log.push_back("stopAll");
        return false;
    };
    hooks.lights = [&calls](bool stopped, float) { calls.log.push_back(stopped ? "lightsOn" : "lightsOff"); };
    hooks.stopAtRouteEnd = [&calls] {
        calls.log.push_back("stopTest");
        return calls.stopAtEnd;
    };
    hooks.moveAlongRoute = [&calls](AiDriverRecord& driver) {
        calls.log.push_back("move");
        if (calls.moveTo >= 0.0f) driver.routeDistance = calls.moveTo;
    };
    hooks.normalGravity = [&calls] { calls.log.push_back("gravity"); };
    hooks.steerStep = [&calls] { calls.log.push_back("steerStep"); };
    hooks.nodeHidden = [&calls] {
        calls.log.push_back("hiddenTest");
        return calls.hidden;
    };
    return hooks;
}

bool logged(const Calls& calls, const std::string& entry) {
    for (const std::string& line : calls.log)
        if (line == entry) return true;
    return false;
}

void createRecord() {
    Calls calls;
    const AiDriverHooks hooks = recordingHooks(calls);
    const AiMatrix node = matrixAt({1.0f, 2.0f, 3.0f}, {0.0f, 0.0f, 1.0f});
    const AiDriverRecord driver = createAiDriverRecord(1.0f, node, hooks);
    require(driver.flags == kAiDriverFlagAi, "0x1796e8 sets only +0xaf bit 0 in the zero-filled record (0x179744..0x17974c)");
    require(driver.heightOffset == 1.0f && driver.path == node, "+0x74 is the attribute and +0x18 the node matrix (0x179720, 0x17973c)");
    require(driver.state == 0 && driver.requestedState == 0, "the record is zero-filled (0x25c030 with r2 = 1 calls 0x24d234)");
    require(calls.log == std::vector<std::string>({"kinematic"}), "the create ends with 0x22a270(entity, 1) (0x17975c)");
}

AiDriverStateFrame stateFrame(const AiRouteFunctions& routes) {
    AiDriverStateFrame frame;
    frame.node = matrixAt({0.0f, 0.0f, 10.0f}, {0.0f, 0.0f, 1.0f});
    const float c = std::cos(0.5235988f);
    const float s = std::sin(0.5235988f);
    frame.body[0] = {c, 0.0f, s};
    frame.body[1] = {0.0f, 1.0f, 0.0f};
    frame.body[2] = {-s, 0.0f, c};
    frame.body[3] = {5.0f, 6.0f, 7.0f};
    frame.bodyBase = 0.25f;
    frame.routes = &routes;
    return frame;
}

void spawnSequence() {
    Calls calls;
    const AiRouteFunctions routes = straightRoute(calls);
    const AiDriverHooks hooks = recordingHooks(calls);
    AiDriverRecord driver;
    driver.flags = kAiDriverFlagAi | kAiDriverFlagSignal;
    driver.heightOffset = 1.0f;
    driver.route = 3;
    driver.routeDistance = 10.0f;
    driver.path = matrixAt({0.0f, 0.0f, 10.0f}, {0.0f, 0.0f, 1.0f});
    driver.requestedState = kAiStateStart;
    AiDriverVehicle vehicle;
    vehicle.maxHearts = 5;
    vehicle.brakeInput = 1.0f;
    vehicle.entityFlag20000 = true;
    const AiDriverStateFrame frame = stateFrame(routes);
    GameRandom random(9);
    GameRandom expected(9);
    const float cruise = 8.0f + gameRandomUnit(expected) * 4.0f;
    const std::uint8_t keep = static_cast<std::uint8_t>(expected.next() % 60 + 30);

    runAiDriverStateChange(driver, vehicle, frame, hooks, random);
    require(driver.state == kAiStateStart && driver.requestedState == kAiStateRoute,
            "the first update after the spawn enters state 1 and requests 2 (0x1cb5e8, 0x1cb5f4..0x1cb5fc)");
    require(driver.stateUpdates == 1, "+0xa4 is cleared on the change and counted at the end (0x1cb5e4, 0x1cb67c..0x1cb684)");
    require(near(driver.cruiseSpeed, cruise), "state 1: +0x6c = 8 + 0x2345dc() x 4 (0x1cb220..0x1cb230), seed 9");
    require(driver.keep == keep, "state 1: +0xad = 0x235498(60) + 30 (0x1cb350..0x1cb360), drawn after +0x6c");
    require((driver.flags & kAiDriverFlagSignal) == 0, "state 1 clears +0xaf bit 3 (0x1cb214..0x1cb21c)");
    require(vehicle.hearts == 5 && vehicle.health == 100 && vehicle.brakeInput == 0.0f && !vehicle.entityFlag20000,
            "state 1: record +0x30d = +0x30e, +0x30c = 100, +0x290 = 0, entity +0xc bit 0x20000 cleared (0x1cb234..0x1cb240, 0x1cb36c)");
    require(near(driver.target[2], 10.1f) && near(driver.target[1], 0.75f),
            "state 1: +0xc = the route point at +0x64 + 0.1, y + (+0x74 - body +0x54) (0x1cb2e4..0x1cb324)");
    const AiVec3 axisX = {frame.body[0][0], frame.body[1][0], frame.body[2][0]};
    const AiVec3 axisZ = {frame.body[0][2], frame.body[1][2], frame.body[2][2]};
    require(driver.path[0] == axisX && driver.path[2] == axisZ && driver.path[3] == AiVec3({0.0f, 0.0f, 10.0f}),
            "state 1: the rigid basis (yaw 30 degrees) is transposed on the stack (0x1cb244..0x1cb2b0), 0x232788 copies its three rows into "
            "+0x18 and leaves the position (0x1cb34c)");
    require(calls.log == std::vector<std::string>({"evaluate 10.100000", "snapTarget", "kinematic"}),
            "state 1 calls the route function +0, 0x20ae2c and 0x22a270(entity, 1) in this order");

    calls.log.clear();
    const float cruise2 = 8.0f + gameRandomUnit(expected) * 4.0f;
    runAiDriverStateChange(driver, vehicle, frame, hooks, random);
    require(driver.state == kAiStateRoute && driver.requestedState == kAiStateRoute, "the next update enters state 2 and stays (servant: 0 -> 1 -> 2)");
    require(near(driver.cruiseSpeed, cruise2), "entering state 2 draws +0x6c again (0x1cb3a8..0x1cb3b8)");
    require(driver.entryPosition == frame.node[3], "state 2: +0x58 = the node position (0x1cb3fc..0x1cb40c)");
    require(calls.log == std::vector<std::string>({"evaluate 10.100000", "snapTarget"}), "state 2 refreshes the target point (0x1cb490..0x1cb4d8)");
    runAiDriverStateChange(driver, vehicle, frame, hooks, random);
    require(driver.state == kAiStateRoute && driver.stateUpdates == 2, "without a request the state stays and +0xa4 counts on");
}

void routeEnd() {
    Calls calls;
    const AiRouteFunctions routes = straightRoute(calls);
    const AiDriverHooks hooks = recordingHooks(calls);
    const AiDriverStateFrame frame = stateFrame(routes);
    AiDriverVehicle vehicle;
    GameRandom random(4);

    AiDriverRecord driver;
    driver.state = kAiStateStart;
    driver.requestedState = kAiStateRoute;
    driver.routeDistance = 55.0f;
    driver.targetSpeed = 9.0f;
    calls.nextRoute = 7;
    runAiDriverStateChange(driver, vehicle, frame, hooks, random);
    require(driver.routeDistance == 50.0f, "at or past the route length (hword +10) +0x64 becomes the length (0x1cb410..0x1cb448)");
    require(driver.state == kAiStateJunction && driver.requestedState == kAiStateJunction,
            "a next route from 0x2cb1e8 requests 3, and the copy at 0x1cb5e0 reads +0x79 after the entry (0x1cb460..0x1cb488)");

    calls.log.clear();
    driver.state = kAiStateStart;
    driver.requestedState = kAiStateRoute;
    calls.nextRoute = 0;
    runAiDriverStateChange(driver, vehicle, frame, hooks, random);
    require(logged(calls, "leaveTraffic") && driver.requestedState == kAiStateRoute, "0x2cb1e8 true with no route: 0x21d6b8 (0x1cb48c)");

    calls.log.clear();
    driver.state = kAiStateStart;
    driver.requestedState = kAiStateRoute;
    driver.targetSpeed = 0.0f;
    runAiDriverStateChange(driver, vehicle, frame, hooks, random);
    require(!logged(calls, "chooseNext"), "a stopped car (+0x70 <= 0) skips the junction (0x1cb450..0x1cb45c)");
}

void stateEntries() {
    Calls calls;
    const AiRouteFunctions routes = straightRoute(calls);
    const AiDriverHooks hooks = recordingHooks(calls);
    AiDriverStateFrame frame = stateFrame(routes);
    AiDriverVehicle vehicle;
    GameRandom random(4);

    AiDriverRecord driver;
    driver.state = kAiStateRoute;
    driver.requestedState = kAiStateSteer;
    runAiDriverStateChange(driver, vehicle, frame, hooks, random);
    require(calls.log.front() == "listed", "entering state 5 with +0xaf bit 4 clear runs 0x230dbc(entity, 1) (0x1cb3bc..0x1cb3f8)");

    calls.log.clear();
    driver.flags = kAiDriverFlagListed | kAiDriverFlagSignal;
    driver.signal = 12;
    driver.requestedState = kAiStateKnocked;
    runAiDriverStateChange(driver, vehicle, frame, hooks, random);
    require(calls.log == std::vector<std::string>({"unlisted", "leaveSignal", "dynamic"}),
            "leaving 5: 0x230dbc(entity, 0); entering 4: 0x22a080, 0x22a270(entity, 0) (0x1cb18c..0x1cb1bc, 0x1cb4fc..0x1cb51c)");
    require(driver.flags == 0, "leaving 5 clears bit 4 and 0x22a080 clears bit 3");

    calls.log.clear();
    frame.bodySpeed = 3.0f;
    frame.activationState = 2;
    runAiDriverStateChange(driver, vehicle, frame, hooks, random);
    require(vehicle.brakeInput == 1.0f && vehicle.handBrakeInput == 1.0f && calls.log == std::vector<std::string>({"wake"}),
            "state 4 brakes while the body moves and wakes a body in activation state 2 or 5 (0x1cb694..0x1cb6c4)");

    calls.log.clear();
    driver.requestedState = kAiStateStopping;
    runAiDriverStateChange(driver, vehicle, frame, hooks, random);
    require(calls.log == std::vector<std::string>({"dynamic", "enterStopping"}), "entering 6 (0x1cb52c..0x1cb5a0)");
    calls.log.clear();
    driver.requestedState = kAiStateBreak;
    runAiDriverStateChange(driver, vehicle, frame, hooks, random);
    require(calls.log == std::vector<std::string>({"destroy"}), "entering 7 sends message 0 with 1000 to itself (0x1cb5b0..0x1cb5dc)");
    calls.log.clear();
    driver.requestedState = kAiStateReturn;
    runAiDriverStateChange(driver, vehicle, frame, hooks, random);
    require(calls.log.empty() && driver.entryPosition == frame.node[3], "entering 8 stores the node position (0x1cb388..0x1cb398)");
    driver.requestedState = kAiStateSteer;
    runAiDriverStateChange(driver, vehicle, frame, hooks, random);
    require(calls.log.front() == "kinematic", "leaving 8 runs 0x22a270(entity, 1) (0x1cb1c4..0x1cb1cc)");

    calls.log.clear();
    driver.flags = kAiDriverFlagEffect | kAiDriverFlagListed;
    driver.stateUpdates = 64;
    GameRandom expected(4);
    GameRandom effectRandom(4);
    const float side = (gameRandomUnit(expected) - 0.5f) * 2.0f;
    runAiDriverStateChange(driver, vehicle, frame, hooks, effectRandom);
    require(calls.log == std::vector<std::string>({"rearEffect"}) && near(calls.effect[0], side) && calls.effect[1] == 0.5f && calls.effect[2] == -2.0f,
            "state 5 with +0xaf bit 1 runs 0x1bc660 at ((r - 0.5) x 2, 0.5, -2) when +0xa4 & 63 = 0 (0x1cb614..0x1cb678)");
    require(driver.stateUpdates == 65, "+0xa4 counts in state 5");
}

AiDriverInputFrame inputFrame(const AiRouteFunctions& routes) {
    AiDriverInputFrame frame;
    frame.node = matrixAt({0.0f, 0.0f, 10.0f}, {0.0f, 0.0f, 1.0f});
    frame.halfExtents = {1.0f, 0.8f, 2.5f};
    frame.bodyBase = 0.25f;
    frame.dt = 1.0f / 30.0f;
    frame.routes = &routes;
    return frame;
}

void obstacleRule() {
    Calls calls;
    const AiRouteFunctions routes = straightRoute(calls);
    const AiDriverHooks hooks = recordingHooks(calls);
    AiDriverInputFrame frame = inputFrame(routes);
    AiDriverVehicle vehicle;

    AiDriverRecord driver;
    driver.state = kAiStateRoute;
    driver.cruiseSpeed = 9.0f;
    driver.stateUpdates = 3;
    runAiDriverInput(driver, vehicle, frame, hooks);
    require(driver.targetSpeed == 9.0f && calls.log.empty(),
            "no hold and no brake: +0x70 = +0x6c; +0xa4 % 5 != 0 skips the check (0x26db40..0x26db78); no hook runs");

    driver.speed = 16.0f;
    driver.stateUpdates = 5;
    AiObstacle ahead;
    ahead.node = matrixAt({0.5f, 0.0f, 20.0f}, {0.0f, 0.0f, 1.0f});
    ahead.halfExtents = {1.0f, 0.8f, 2.5f};
    frame.obstacles = {ahead};
    runAiDriverInput(driver, vehicle, frame, hooks);
    require(driver.targetSpeed == 0.0f && driver.hold == 8,
            "a car 10 ahead (< 2.5 + 2.5 + sqrt(16) + 3) and 0.5 aside (< 1 + 0.5 + 1) stops it: +0x70 = 0, hold 8 (0x26dc34..0x26dce0)");

    driver.hold = 0;
    driver.targetSpeed = 9.0f;
    frame.obstacles[0].node[3] = {0.0f, 0.0f, 22.1f};
    runAiDriverInput(driver, vehicle, frame, hooks);
    require(driver.targetSpeed == 9.0f, "12.1 ahead is beyond the reach 12.0");
    frame.obstacles[0].node[3] = {2.6f, 0.0f, 20.0f};
    runAiDriverInput(driver, vehicle, frame, hooks);
    require(driver.targetSpeed == 9.0f, "2.6 aside is beyond |half x . own x| + 0.5 + own half x = 2.5");
    frame.obstacles[0].node[3] = {0.0f, 0.0f, 5.0f};
    runAiDriverInput(driver, vehicle, frame, hooks);
    require(driver.targetSpeed == 9.0f, "a car behind (forward dot <= 0) is ignored");

    driver.hold = 3;
    driver.stateUpdates = 7;
    frame.obstacles.clear();
    runAiDriverInput(driver, vehicle, frame, hooks);
    require(driver.hold == 2 && driver.targetSpeed == 9.0f, "a running hold counts down and skips the resume (0x26db1c..0x26db3c)");
    driver.hold = 1;
    driver.targetSpeed = 0.0f;
    vehicle.brakeInput = 1.0f;
    runAiDriverInput(driver, vehicle, frame, hooks);
    require(driver.hold == 0 && driver.targetSpeed == 0.0f, "the hold ends, but a brake input (record +0x290) keeps the car stopped");

    driver.signal = 4;
    driver.flags = kAiDriverFlagSignal;
    driver.stateUpdates = 10;
    calls.signalPass = false;
    frame.obstacles = {ahead};
    vehicle.brakeInput = 0.0f;
    driver.targetSpeed = 9.0f;
    runAiDriverInput(driver, vehicle, frame, hooks);
    require(driver.targetSpeed == 0.0f && driver.hold == 8 && calls.log == std::vector<std::string>({"signalTest"}),
            "a signal (+0, +0xaf bit 3) for which 0x2cdde4 is 0 stops the car first (0x26db88..0x26dbb4)");
}

void steerState() {
    Calls calls;
    const AiRouteFunctions routes = straightRoute(calls);
    const AiDriverHooks hooks = recordingHooks(calls);
    AiDriverInputFrame frame = inputFrame(routes);
    AiDriverVehicle vehicle;
    AiDriverRecord driver;
    driver.state = kAiStateSteer;
    driver.cruiseSpeed = 30.0f;
    driver.heightOffset = 1.0f;
    driver.hold = 2;
    frame.playerInVehicle = true;
    frame.gearSpeed = 20.0f;
    frame.collisionFlags = 0x302;
    frame.layerBit = true;
    const AiDriverInputResult result = runAiDriverInput(driver, vehicle, frame, hooks);
    require(driver.targetSpeed == 16.0f, "with the player in a vehicle +0x70 = min(+0x6c, 0x20362c x 0.8) (0x26dd2c..0x26dd78)");
    require(result.clearCollisionFlag200, "rigid flag 0x200 is cleared when the layer table allows it (0x26dd7c..0x26dddc)");
    require(calls.log == std::vector<std::string>({"nearest 10 1.000000", "ahead 8.000000 0.500000", "evaluate 18.000000", "steer", "copyInputs"}),
            "nearest (+0xc, 10 samples, 1.0), 8 m ahead in 0.5 steps (+8), the point (+0), 0x27872c, 0x23019c (0x26dde4..0x26dec8)");
    require(driver.routeDistance == 18.0f && driver.path[3] == AiVec3({0.0f, 0.0f, 18.0f}), "+0x64 and +0x18 take the point ahead");
    require(near(calls.steerTarget[3][1], 0.75f) && calls.steerDt == frame.dt, "the steer target is lifted by +0x74 - body +0x54 (0x26de98..0x26deb4)");
    require(driver.hold == 1, "state 5 counts the hold down after the input copy (0x26decc..0x26ded8)");
    driver.flags = kAiDriverFlagFullSpeed;
    runAiDriverInput(driver, vehicle, frame, hooks);
    require(driver.targetSpeed == 20.0f, "+0xaf bit 2 uses factor 1.0");
}

void returnState() {
    Calls calls;
    const AiRouteFunctions routes = straightRoute(calls);
    const AiDriverHooks hooks = recordingHooks(calls);
    AiDriverInputFrame frame = inputFrame(routes);
    AiDriverVehicle vehicle;
    AiDriverRecord driver;
    driver.state = kAiStateReturn;
    driver.path = matrixAt({0.0f, 0.0f, 19.0f}, {0.0f, 0.0f, 1.0f});
    runAiDriverInput(driver, vehicle, frame, hooks);
    require(driver.requestedState == kAiStateRoute && calls.log == std::vector<std::string>({"steer", "copyInputs"}),
            "facing the path (dot >= 0.9) within 10 m of +0x48: +0x79 = 2 (0x26dab4..0x26daf8)");
    driver.requestedState = kAiStateReturn;
    driver.path = matrixAt({0.0f, 0.0f, 19.0f}, {0.6f, 0.0f, 0.8f});
    runAiDriverInput(driver, vehicle, frame, hooks);
    require(driver.requestedState == kAiStateReturn, "below 0.9 the radius is 4 m");

    calls.log.clear();
    driver.path = matrixAt({0.0f, 0.0f, 19.0f}, {0.0f, 0.0f, -1.0f});
    runAiDriverInput(driver, vehicle, frame, hooks);
    require(calls.log == std::vector<std::string>({"steer", "follow", "setNode", "evaluate 18.000000", "copyInputs"}),
            "facing away: 0x2bf2ec from 8 m ahead, the node matrix, the route point (0x26da04..0x26daa4)");
    require(driver.signal == 0 && driver.path[2] == AiVec3({0.0f, 0.0f, 1.0f}), "+0 = 0x20ff40(route) and +0x18 rebuilt by 0x237a0c");

    calls.log.clear();
    for (std::uint8_t state : {std::uint8_t{0}, std::uint8_t{1}, std::uint8_t{4}, std::uint8_t{6}, std::uint8_t{7}, std::uint8_t{9}}) {
        driver.state = state;
        runAiDriverInput(driver, vehicle, frame, AiDriverHooks{});
    }
    require(calls.log.empty(), "states 0, 1, 4, 6 (0x26dee8 tail-calls the empty 0x1d8df0), 7 and above 8 give no input");
}

AiDriverPostFrame postFrame(const AiRouteFunctions& routes) {
    AiDriverPostFrame frame;
    frame.node = matrixAt({0.0f, 0.0f, 10.0f}, {0.0f, 0.0f, 1.0f});
    frame.dt = 1.0f / 30.0f;
    frame.routes = &routes;
    return frame;
}

void postStep() {
    Calls calls;
    const AiRouteFunctions routes = straightRoute(calls);
    const AiDriverHooks hooks = recordingHooks(calls);
    AiDriverPostFrame frame = postFrame(routes);
    AiDriverVehicle vehicle;
    GameRandom random(5);

    AiDriverRecord driver;
    driver.state = kAiStateHold;
    driver.path = matrixAt({1.0f, 2.0f, 3.0f}, {0.0f, 0.0f, 1.0f});
    runAiDriverPostStep(driver, vehicle, frame, hooks, random);
    require(calls.log == std::vector<std::string>({"stopAll", "lightsOn", "setNode", "hiddenTest"}) && calls.node == driver.path,
            "state 0 holds the node at +0x18; +0x70 = 0 turns the lights on (0x1bc7bc..0x1bc858)");
    require(driver.keep == 30, "a visible car keeps +0xad at least 30 (0x1bcb0c..0x1bcb14)");
    calls.hidden = true;
    runAiDriverPostStep(driver, vehicle, frame, hooks, random);
    require(driver.keep == 29, "0x23de70 true counts +0xad down (0x1bcac4..0x1bcad4)");

    calls.log.clear();
    driver.state = kAiStateRoute;
    driver.targetSpeed = 9.0f;
    driver.speed = 9.0f;
    driver.routeDistance = 47.0f;
    vehicle.brakeInput = 1.0f;
    runAiDriverPostStep(driver, vehicle, frame, hooks, random);
    require(calls.log == std::vector<std::string>({"stopAll", "lightsOff", "move", "hiddenTest"}) && vehicle.brakeInput == 1.0f,
            "state 2 tests the route end only within 2 m of the length (0x1bc868..0x1bc88c), then 0x2613ac");
    calls.log.clear();
    driver.routeDistance = 48.0f;
    runAiDriverPostStep(driver, vehicle, frame, hooks, random);
    require(vehicle.brakeInput == 0.0f && logged(calls, "stopTest"), "no stop and no hold: the brake input 1.0 is released (0x1bc8a8..0x1bc8b8)");
    calls.stopAtEnd = true;
    runAiDriverPostStep(driver, vehicle, frame, hooks, random);
    require(vehicle.brakeInput == 1.0f && driver.targetSpeed == 0.0f, "0x2ca504 true: brake 1.0, +0x70 = 0 (0x1bc890..0x1bc8a0)");

    calls.stopAtEnd = false;
    calls.log.clear();
    driver.state = kAiStateJunction;
    driver.flags = kAiDriverFlagSignal;
    driver.route = 3;
    driver.nextRoute = 6;
    calls.moveTo = 2.9f;
    GameRandom expected(5);
    GameRandom junctionRandom(5);
    const float start = gameRandomUnit(expected) * 0.4f;
    runAiDriverPostStep(driver, vehicle, frame, hooks, junctionRandom);
    require(driver.route == 6 && driver.signal == 600 && driver.requestedState == kAiStateRoute && near(driver.routeDistance, start),
            "state 3 past 2.8 after 0x2613ac: +4 = +8, +0 = 0x20ff40, +0x79 = 2, +0x64 = random x 0.4 (0x1bc904..0x1bc944)");
    require(logged(calls, "leaveSignal") && driver.flags == 0, "and 0x22a080 leaves the signal queue");
    calls.moveTo = -1.0f;

    calls.log.clear();
    driver.state = kAiStateStopping;
    driver.requestedState = kAiStateStopping;
    driver.stateUpdates = 90;
    frame.bodySpeed = 0.1f;
    runAiDriverPostStep(driver, vehicle, frame, hooks, random);
    require(driver.requestedState == kAiStateBreak && logged(calls, "copyInputs"),
            "state 6: 0x23019c, then at body speed <= 0.1 after 90 updates +0x79 = 7 (0x1bca84..0x1bcab4)");
    driver.requestedState = kAiStateStopping;
    driver.stateUpdates = 89;
    runAiDriverPostStep(driver, vehicle, frame, hooks, random);
    require(driver.requestedState == kAiStateStopping, "89 updates are not enough");

    calls.log.clear();
    driver.state = kAiStateKnocked;
    driver.requestedState = kAiStateKnocked;
    driver.contactTime = 1.0f;
    frame.now = 1.5f;
    frame.bodyMoveSpeed = 0.4f;
    frame.wheelCount = 4;
    frame.groundedWheels = 3;
    runAiDriverPostStep(driver, vehicle, frame, hooks, random);
    require(driver.requestedState == kAiStateReturn && logged(calls, "follow") && logged(calls, "setNode"),
            "state 4: slow, 0.5 s after the contact and back on the route with wheels - 1 on the ground: +0x79 = 8 (0x1bc97c..0x1bca6c)");
    driver.requestedState = kAiStateKnocked;
    frame.now = 1.4f;
    calls.log.clear();
    runAiDriverPostStep(driver, vehicle, frame, hooks, random);
    require(driver.requestedState == kAiStateKnocked && !logged(calls, "follow") && logged(calls, "copyInputs"), "0.4 s is too early");

    calls.log.clear();
    driver.state = kAiStateSteer;
    frame.bodySpeed = 12.0f;
    runAiDriverPostStep(driver, vehicle, frame, hooks, random);
    require(driver.speed == 12.0f && logged(calls, "gravity") && logged(calls, "steerStep"),
            "states 5 and 8: +0x68 = 0x229948(body), gravity back, 0x29cc38 (0x1bc94c..0x1bc96c)");
}

void missingHook() {
    Calls calls;
    const AiRouteFunctions routes = straightRoute(calls);
    AiDriverInputFrame frame = inputFrame(routes);
    AiDriverRecord driver;
    driver.state = kAiStateSteer;
    bool threw = false;
    try {
        runAiDriverInput(driver, AiDriverVehicle{}, frame, AiDriverHooks{});
    } catch (const std::logic_error&) {
        threw = true;
    }
    require(threw, "a hook the taken path needs is refused when it is not set");
}

}

int main() {
    try {
        createRecord();
        spawnSequence();
        routeEnd();
        stateEntries();
        obstacleRule();
        steerState();
        returnState();
        postStep();
        missingHook();
    } catch (const std::exception& error) {
        std::cerr << "ai-driver-input: " << error.what() << '\n';
        return 1;
    }
    std::cout << "ai-driver-input: ok\n";
    return 0;
}
