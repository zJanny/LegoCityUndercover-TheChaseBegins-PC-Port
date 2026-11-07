// Tests of src/runtime/traffic_route_mover.*: the movement of a traffic car in AI driver states 2 and 3. Every expected value follows from
// the disassembly (llvm-objdump --triple=armv6k-none-eabi --mattr=+vfp2 of artifacts/game/game.arm.elf, read by 20261102-traffic-B on
// 2026-11-02, docs/re/traffic-ai-driver.md): the route mover 0x2613ac with its literals 0x261690..0x2616b4, the stop test 0x2ca504 with the
// light test 0x2786a8, the junction choice 0x2cb1e8 (literals 0x2cb420, 0x2cb424) and the junction queue 0x22a080. The turned rows are those
// rules evaluated in single precision by hand (look-at 0x22859c, lerp 0x221e5c, orthonormalisation 0x2361d4); no emulator was used.
#include "runtime/traffic_route_mover.hpp"

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

bool nearVec(const AiVec3& a, const AiVec3& b) { return near(a[0], b[0]) && near(a[1], b[1]) && near(a[2], b[2]); }

template <typename Function>
bool throws(Function function) {
    try {
        function();
    } catch (const std::exception&) {
        return true;
    }
    return false;
}

AiMatrix identityAt(const AiVec3& position) {
    AiMatrix m{};
    m[0] = {1.0f, 0.0f, 0.0f};
    m[1] = {0.0f, 1.0f, 0.0f};
    m[2] = {0.0f, 0.0f, 1.0f};
    m[3] = position;
    return m;
}

// A route along +z with 10 m per unit of the path parameter; the nearest sample is the node's z / 10.
struct Routes {
    std::vector<std::string> log;
    std::uint16_t length = 6;
    AiRouteFunctions functions;

    Routes() {
        functions.evaluate = [this](std::uint32_t route, float distance, AiVec3* position, AiVec3* direction) {
            log.push_back("evaluate " + std::to_string(route) + " " + std::to_string(distance));
            *position = {0.0f, 0.0f, distance * 10.0f};
            if (direction != nullptr) *direction = {0.0f, 0.0f, 1.0f};
        };
        functions.length = [this](std::uint32_t) { return length; };
        functions.nearest = [this](std::uint32_t route, const AiVec3& position, unsigned samples, float scale) {
            log.push_back("nearest " + std::to_string(route) + " " + std::to_string(samples) + " " + std::to_string(scale));
            return position[2] / 10.0f;
        };
    }
};

struct Hooks {
    int snaps = 0;
    int leaves = 0;
    AiDriverHooks hooks;

    Hooks() {
        hooks.snapTarget = [this] { ++snaps; };
        hooks.leaveTraffic = [this] { ++leaves; };
    }
};

GameRandom fixedRandom(const std::array<std::uint32_t, kGameRandomTableSize>& table) { return GameRandom(table, 0, 7); }

TrafficMoverFrame frameAt(const AiVec3& position, const Routes& routes) {
    TrafficMoverFrame frame;
    frame.car = 7;
    frame.node = identityAt(position);
    frame.dt = 0.05f;
    frame.radius = 2.0f;
    frame.bodyBase = 0.25f;
    frame.gearSpeed = 20.0f;
    frame.engineFrom = 1000.0f;
    frame.engineTo = 3000.0f;
    frame.routes = &routes.functions;
    return frame;
}

AiDriverRecord driverOn(std::uint32_t route, float distance) {
    AiDriverRecord driver;
    driver.route = route;
    driver.routeDistance = distance;
    driver.heightOffset = 1.0f;
    driver.flags = kAiDriverFlagAi;
    driver.state = kAiStateRoute;
    driver.requestedState = kAiStateRoute;
    return driver;
}

// One junction: input route 100, outputs 200..202, lanes 300..302 joining the input with each output, light pairs given per test.
TrafficJunctionWorld junctionWorld() {
    TrafficJunctionWorld world;
    TrafficJunctionRecord junction;
    junction.layout.inputs = {100};
    junction.layout.outputs = {200, 201, 202};
    junction.layout.lanes = {300, 301, 302};
    junction.laneEnds = {{0, 0}, {0, 1}, {0, 2}};
    junction.choices = {{0, 1, 2}};
    world.junctions.push_back(junction);
    world.signals.inputs.push_back({100, 0});
    world.routes.loaded = true;
    world.routes.routes = {100, 200, 201, 202};
    world.routes.enabled = {true, true, false, true};
    return world;
}

void counterRunsSnapEverySixthUpdate() {
    Routes routes;
    Hooks hooks;
    TrafficJunctionWorld world;
    GameRandom random(1);
    AiDriverRecord driver = driverOn(1, 0.0f);
    driver.target = {0.0f, 0.0f, 100.0f};
    driver.moveCounter = 4;
    runTrafficRouteMover(driver, frameAt({0.0f, 0.0f, 0.0f}, routes), world, hooks.hooks, random);
    require(driver.moveCounter == 5 && hooks.snaps == 0, "0x2613d4: +0xae 5 does not snap");
    runTrafficRouteMover(driver, frameAt({0.0f, 0.0f, 0.0f}, routes), world, hooks.hooks, random);
    require(driver.moveCounter == 0 && hooks.snaps == 1, "0x2613d4: +0xae above 5 runs 0x20ae2c and restarts at 0");
}

void slowCarMovesWithoutTurning() {
    Routes routes;
    Hooks hooks;
    TrafficJunctionWorld world;
    GameRandom random(1);
    AiDriverRecord driver = driverOn(1, 0.0f);
    driver.speed = 1.0f;
    driver.targetSpeed = 1.0f;
    driver.target = {30.0f, 0.0f, 40.0f};
    const AiMatrix path = driver.path;
    const TrafficMoverResult result = runTrafficRouteMover(driver, frameAt({0.0f, 0.0f, 0.0f}, routes), world, hooks.hooks, random);
    require(nearVec(result.node[2], {0.0f, 0.0f, 1.0f}), "0x261410: no turn at speed 1.0");
    require(nearVec(result.node[3], {0.0f, 0.0f, 0.05f}), "0x2614b0: the node moves dt x speed along row 2");
    require(driver.path == path, "0x2614a8: +0x18 keeps its rows without a turn");
    require(!result.velocity, "0x26161c: no velocity for a body that is not kinematic");
}

void turnTowardTarget() {
    Routes routes;
    Hooks hooks;
    TrafficJunctionWorld world;
    GameRandom random(1);
    AiDriverRecord driver = driverOn(1, 0.0f);
    driver.speed = 10.0f;
    driver.targetSpeed = 12.0f;
    driver.target = {3.0f, 0.0f, 4.0f};
    TrafficMoverFrame frame = frameAt({0.0f, 0.0f, 0.0f}, routes);
    frame.radius = 1.0f;
    frame.kinematic = true;
    const TrafficMoverResult result = runTrafficRouteMover(driver, frame, world, hooks.hooks, random);
    require(nearVec(result.node[0], {0.9922777f, 0.0f, -0.1240347f}), "turn by dt x 4: row 0");
    require(nearVec(result.node[1], {0.0f, 1.0f, 0.0f}), "turn by dt x 4: row 1");
    require(nearVec(result.node[2], {0.1240347f, 0.0f, 0.9922778f}), "turn by dt x 4: row 2");
    require(nearVec(result.node[3], {0.06201737f, 0.0f, 0.4961389f}), "the move follows the turned row 2");
    require(nearVec(driver.path[2], result.node[2]) && nearVec(driver.path[0], result.node[0]), "0x2614a8: rows 0..2 copied to +0x18");
    require(nearVec(driver.path[3], {0.0f, 0.0f, 0.0f}), "0x232788 copies three rows only");
    require(result.velocity && nearVec(*result.velocity, {1.240347f, 0.0f, 9.922778f}), "0x229920: row 2 x speed");
    require(near(result.engineBlend, 2000.0f), "0x2286d4: lerp(+0x26c, +0x264, speed / 0x20362c)");
    require(near(driver.speed, 10.36f), "0x26167c: speed moves 0.18 of the way to +0x70");
    require(routes.log.empty(), "no target refresh outside the radius");
}

void fastTurnRate() {
    Routes routes;
    Hooks hooks;
    TrafficJunctionWorld world;
    GameRandom random(1);
    AiDriverRecord driver = driverOn(1, 0.0f);
    driver.speed = 25.0f;
    driver.targetSpeed = 25.0005f;
    driver.target = {3.0f, 0.0f, 4.0f};
    const TrafficMoverResult result = runTrafficRouteMover(driver, frameAt({0.0f, 0.0f, 0.0f}, routes), world, hooks.hooks, random);
    require(nearVec(result.node[2], {0.1880723f, 0.0f, 0.9821552f}), "above 20 the turn is dt x 6");
    require(nearVec(result.node[3], {0.2350903f, 0.0f, 1.227694f}), "fast move");
    require(driver.speed == 25.0005f, "0x261670: within 0.001 the speed takes +0x70");
}

void targetAdvancesAlongRoute() {
    Routes routes;
    Hooks hooks;
    TrafficJunctionWorld world;
    GameRandom random(1);
    AiDriverRecord driver = driverOn(1, 2.0f);
    driver.speed = 1.0f;
    driver.targetSpeed = 1.0f;
    driver.target = {0.0f, 0.0f, 20.0f};
    runTrafficRouteMover(driver, frameAt({0.0f, 0.0f, 19.5f}, routes), world, hooks.hooks, random);
    require(near(driver.routeDistance, 2.4f), "0x2616a8: +0x64 grows by 0.4");
    require(nearVec(driver.target, {0.0f, 0.75f, 24.0f}), "0x26159c: the route point at +0x64 with y + 0x20af24");

    routes.length = 4;
    driver = driverOn(1, 2.0f);
    driver.speed = 1.0f;
    driver.target = {0.0f, 0.0f, 20.0f};
    runTrafficRouteMover(driver, frameAt({0.0f, 0.0f, 19.5f}, routes), world, hooks.hooks, random);
    require(near(driver.routeDistance, 2.15f), "0x2616a4: a route of length 4 steps 0.15");
}

void targetPastRouteEndFollowsRow2() {
    Routes routes;
    Hooks hooks;
    TrafficJunctionWorld world;
    GameRandom random(1);
    AiDriverRecord driver = driverOn(1, 7.0f);
    driver.speed = 1.0f;
    driver.targetSpeed = 1.0f;
    driver.target = {0.0f, 0.0f, 20.0f};
    runTrafficRouteMover(driver, frameAt({0.0f, 0.0f, 19.5f}, routes), world, hooks.hooks, random);
    require(driver.routeDistance == 7.0f, "0x2615dc: +0x64 stays past the length");
    require(nearVec(driver.target, {0.0f, 0.0f, 21.0f}), "0x2615e4: the target moves on by row 2");
    require(routes.log.empty(), "past the length the route is not evaluated");
}

void routeEndWithoutSpeedTakesNoJunction() {
    Routes routes;
    Hooks hooks;
    TrafficJunctionWorld world = junctionWorld();
    GameRandom random(1);
    AiDriverRecord driver = driverOn(100, 5.8f);
    driver.speed = 1.0f;
    driver.target = {0.0f, 0.0f, 20.0f};
    runTrafficRouteMover(driver, frameAt({0.0f, 0.0f, 19.5f}, routes), world, hooks.hooks, random);
    require(driver.routeDistance == 6.0f, "0x261554: +0x64 = the length at the end");
    require(driver.route == 100 && driver.requestedState == kAiStateRoute, "0x261568: no choice while +0x70 is 0");
    require(nearVec(driver.target, {0.0f, 0.75f, 60.0f}), "the target is the route end");
}

void routeEndChoosesJunction() {
    Routes routes;
    Hooks hooks;
    TrafficJunctionWorld world = junctionWorld();
    // 0x25b2b8 with cursors 0 / 7: ror(1 + 0, 29) = 8 picks output 1 (8 % 24 >> 3), whose route bit is clear; then cursors 10 / 6:
    // ror(2 + 0, 29) = 16 picks output 2.
    GameRandom random = fixedRandom({1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2});
    AiDriverRecord driver = driverOn(100, 5.8f);
    driver.speed = 1.0f;
    driver.targetSpeed = 1.0f;
    driver.target = {0.0f, 0.0f, 20.0f};
    runTrafficRouteMover(driver, frameAt({0.0f, 0.0f, 19.5f}, routes), world, hooks.hooks, random);
    require(driver.requestedState == kAiStateJunction, "0x261594: a junction requests state 3");
    require(driver.route == 302 && driver.nextRoute == 202, "0x2cb35c: the lane of the input and output 2, the output next");
    require(near(driver.routeDistance, 1.95f), "0x2cb388: the nearest sample to the node before the move");
    require(driver.signal == 1 && (driver.flags & kAiDriverFlagSignal) != 0, "0x2cb3c8: +0 = the junction, bit 3 set");
    require(world.junctions[0].queuedCars == std::vector<std::uint32_t>{7}, "0x2cb3e0: the car is queued");
    require(world.junctions[0].layout.listedLanes == std::vector<std::uint32_t>{2}, "0x2cb3ec: with its lane");
    require(random.firstCursor() == 9, "two draws");
    require(nearVec(driver.target, {0.0f, 0.75f, 19.5f}), "the target is the lane point at +0x64");
    require(routes.log.size() == 3 && routes.log[0] == "nearest 302 10 1.000000", "0x21d5b0 with 10 samples and scale 1.0");
    require(hooks.leaves == 0, "no removal");
}

void routeEndWithoutJunctionLeavesTraffic() {
    Routes routes;
    Hooks hooks;
    TrafficJunctionWorld world = junctionWorld();
    GameRandom random(1);
    AiDriverRecord driver = driverOn(150, 5.8f);
    driver.speed = 1.0f;
    driver.targetSpeed = 1.0f;
    driver.target = {0.0f, 0.0f, 20.0f};
    runTrafficRouteMover(driver, frameAt({0.0f, 0.0f, 19.5f}, routes), world, hooks.hooks, random);
    require(hooks.leaves == 1 && driver.requestedState == kAiStateRoute, "0x261598: no junction calls 0x21d6b8");
    require(driver.signal == 0 && driver.route == 150, "the driver keeps its route");
}

void choiceRules() {
    Routes routes;
    TrafficJunctionWorld world = junctionWorld();
    GameRandom random(1);
    TrafficJunctionChoiceFrame frame{7, {0.0f, 0.0f, 50.0f}, 0.25f, &routes.functions};
    AiDriverRecord driver = driverOn(100, 5.4f);
    std::uint32_t signal = 99;
    require(!chooseTrafficJunction(driver, frame, world, random, signal) && signal == 99, "0x2cb224: before length - 0.5 nothing");

    driver = driverOn(100, 5.5f);
    world.junctions[0].choices = {{1}};
    const auto before = random.table();
    require(chooseTrafficJunction(driver, frame, world, random, signal) && signal == 1, "0x2cb404: the junction");
    require(driver.route == 301 && random.table() == before, "0x2cb2a0: a single choice is taken without a draw or bit test");

    world = junctionWorld();
    world.junctions[0].choices = {{1, 1}};
    driver = driverOn(100, 6.0f);
    require(throws([&] { chooseTrafficJunction(driver, frame, world, random, signal); }), "STAND-IN TRAFFIC-S11: no enabled output");

    world = junctionWorld();
    world.junctions[0].choices = {{}};
    require(throws([&] { chooseTrafficJunction(driver, frame, world, random, signal); }), "STAND-IN TRAFFIC-S11: no lane for count 0");

    world = junctionWorld();
    world.junctions[0].choices = {{0}};
    world.junctions[0].laneEnds = {{0, 1}, {0, 2}, {0, 2}};
    require(throws([&] { chooseTrafficJunction(driver, frame, world, random, signal); }), "STAND-IN TRAFFIC-S11: no lane for the pair");
}

void stopTestRules() {
    TrafficJunctionWorld world = junctionWorld();
    AiDriverRecord driver = driverOn(100, 6.0f);
    require(!trafficStopAtLights(driver, world), "0x2ca518: no signal object");
    driver.signal = 1;
    require(!trafficStopAtLights(driver, world), "0x2ca530: no light pairs");

    TrafficJunctionRecord& junction = world.junctions[0];
    junction.lightPairs = {{TrafficLight{{100}}, std::nullopt}, {std::nullopt, TrafficLight{{999, 100}}}};
    junction.passPair = 0;
    junction.lightPhase = 2;
    require(trafficStopAtLights(driver, world), "0x2ca5f0: a light of pair 1 controls the route while +0x300 is 0");
    junction.lightPairs[1][1]->routes = {999};
    require(!trafficStopAtLights(driver, world), "the passing pair lets the car go");
    junction.lightPhase = 0;
    require(trafficStopAtLights(driver, world), "0x2ca5fc: byte +0x308 = 0 stops the passing pair too");

    driver.route = 301;
    junction.lightPairs[0][0]->routes = {0};
    require(!trafficStopAtLights(driver, world), "0x2ca5a4: a car on a lane is never stopped");
    driver.route = 150;
    require(trafficStopAtLights(driver, world), "a route that is no input or lane tests the lights with route 0");
    require(trafficLightControls(TrafficLight{{5, 6, 7}}, 7) && !trafficLightControls(TrafficLight{{5}}, 7), "0x2786a8");
}

void queueRules() {
    TrafficJunctionWorld world = junctionWorld();
    TrafficJunctionRecord& junction = world.junctions[0];
    junction.queuedCars = {7, 8, 9};
    junction.layout.listedLanes = {0, 1, 2};
    AiDriverRecord driver = driverOn(300, 3.0f);
    leaveTrafficJunctionQueue(junction, driver, 7);
    require(junction.queuedCars.size() == 3, "0x22a0a0: nothing without bit 3");

    driver.flags |= kAiDriverFlagSignal;
    leaveTrafficJunctionQueue(junction, driver, 7);
    require(junction.queuedCars == std::vector<std::uint32_t>{9, 8}, "0x22a104: the last entry moves into the found one");
    require(junction.layout.listedLanes == std::vector<std::uint32_t>{2, 1}, "both words of the entry move");
    require((driver.flags & kAiDriverFlagSignal) == 0, "0x22a114: bit 3 cleared");

    driver.flags |= kAiDriverFlagSignal;
    leaveTrafficJunctionQueue(junction, driver, 5);
    require(junction.queuedCars == std::vector<std::uint32_t>{9}, "0x22a0ec: a car not found still drops the last entry");

    driver.flags |= kAiDriverFlagSignal;
    leaveTrafficJunctionQueue(junction, driver, 9);
    require(junction.queuedCars.empty() && junction.layout.listedLanes.empty(), "the only entry leaves");
    driver.flags |= kAiDriverFlagSignal;
    require(throws([&] { leaveTrafficJunctionQueue(junction, driver, 9); }), "STAND-IN TRAFFIC-S11: an empty queue");
    require(trafficJunctionAllowsCar(junction.layout, 300), "the junction test reads the queue's lanes");
}

void signalIds() {
    TrafficJunctionWorld world = junctionWorld();
    require(trafficSignalId(std::nullopt) == 0 && trafficSignalId(0) == 1, "junction index + 1");
    require(trafficSignalJunction(world, 1) == &world.junctions[0] && trafficSignalJunction(world, 2) == nullptr, "lookup");
}

}

int main() {
    try {
        counterRunsSnapEverySixthUpdate();
        slowCarMovesWithoutTurning();
        turnTowardTarget();
        fastTurnRate();
        targetAdvancesAlongRoute();
        targetPastRouteEndFollowsRow2();
        routeEndWithoutSpeedTakesNoJunction();
        routeEndChoosesJunction();
        routeEndWithoutJunctionLeavesTraffic();
        choiceRules();
        stopTestRules();
        queueRules();
        signalIds();
    } catch (const std::exception& error) {
        std::cerr << "traffic-route-mover: " << error.what() << '\n';
        return 1;
    }
    std::cout << "traffic-route-mover: ok\n";
    return 0;
}
