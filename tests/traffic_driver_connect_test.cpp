// Tests of src/runtime/traffic_driver_connect.*: the AI driver of a spawned traffic car run with the ported route mover, stop test, junction
// choice and queue on made-up routes. The routes, junction and light are made up (straight roads along +z, 10 m per unit of the path
// parameter, a lane of 5 m per unit); every step the test expects follows from the rules read from the disassembly by earlier rounds:
// the spawn 0x2def8c and the state change 0x1cb148 (docs/re/traffic-ai-driver.md §2026-10-25T12:26:09), the post step 0x1bc79c with the stop
// test 0x2ca504, the route mover 0x2613ac, the junction choice 0x2cb1e8 and the queue 0x22a080 (§2026-11-02T23:44:38), the leave call
// 0x21d6b8 (§2026-11-03T08:37:32) and the path follower 0x2bf2ec (§2026-10-25T12:19:38). Written by 20261108-traffic-B, 2026-11-08; no
// emulator was used.
#include "runtime/traffic_driver_connect.hpp"

#include "runtime/pedestrian_manager.hpp"

#include <algorithm>
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

constexpr std::uint32_t kRoadIn = 101;
constexpr std::uint32_t kRoadOut = 102;
constexpr std::uint32_t kLane = 201;
constexpr float kHeight = 0.5f;
constexpr float kDt = 1.0f / 30.0f;

float startOf(std::uint32_t route) { return route == kRoadIn ? 0.0f : route == kLane ? 60.0f : 80.0f; }
float scaleOf(std::uint32_t route) { return route == kLane ? 5.0f : 10.0f; }

AiRouteFunctions routes() {
    AiRouteFunctions functions;
    functions.evaluate = [](std::uint32_t route, float distance, AiVec3* position, AiVec3* direction) {
        *position = {0.0f, 0.0f, startOf(route) + distance * scaleOf(route)};
        if (direction != nullptr) *direction = {0.0f, 0.0f, 1.0f};
    };
    functions.nearest = [](std::uint32_t route, const AiVec3& position, unsigned, float) {
        const float length = route == kLane ? 4.0f : 6.0f;
        return std::min(std::max((position[2] - startOf(route)) / scaleOf(route), 0.1f), length);
    };
    functions.length = [](std::uint32_t route) { return static_cast<std::uint16_t>(route == kLane ? 4 : 6); };
    functions.signal = [](std::uint32_t route) { return route == kRoadIn ? 1u : 0u; };
    return functions;
}

TrafficJunctionWorld junctionWorld() {
    TrafficJunctionRecord junction;
    junction.layout.inputs = {kRoadIn};
    junction.layout.outputs = {kRoadOut};
    junction.layout.lanes = {kLane};
    junction.choices = {{0}};
    junction.laneEnds = {{0, 0}};
    TrafficLight light;
    light.routes = {kRoadIn};
    junction.lightPairs = {{light, std::nullopt}};
    junction.passPair = 0;
    junction.lightPhase = kTrafficLightRed;
    TrafficJunctionWorld world;
    world.junctions = {junction};
    world.signals.inputs = {{kRoadIn, 0}};
    world.routes.loaded = true;
    world.routes.routes = {kRoadIn, kRoadOut};
    world.routes.enabled = {true, true};
    return world;
}

struct Run {
    TrafficManager manager;
    TrafficDrivers drivers;
    TrafficCarBodies bodies;
    TrafficCarBodies::World bodyWorld;
    TrafficJunctionWorld world = junctionWorld();
    AiRouteFunctions functions = routes();
    GameRandom random{0x1234u};
    AiMatrix node{};
    std::vector<TrafficDriveEvent> events;
    std::vector<TrafficEvent> traffic;
    std::vector<TrafficObstacle> obstacles;

    Run() {
        manager.loadLevel(1, 1, random);
        manager.routeTable() = world.routes;
        drivers.load({kHeight});
        node = pedestrianPlacement({0.0f, kHeight, 10.0f}, {0.0f, 0.0f, 1.0f});
        bodyWorld.node = [this](std::size_t) { return node; };
        bodies.load({TrafficCarBodies::Car{}}, {node}, -39.0f);
        drivers.start(0, kRoadIn, 1, 1.0f, node);
        manager.cars()[0].disabled = false;
    }

    void step() {
        TrafficDriveStep drive;
        drive.dt = kDt;
        drive.obstacles = obstacles;
        drive.ground.cast = [](const TrafficGroundSegment&) { return std::optional<TrafficGroundHit>(); };
        drive.ground.sendColour = [](std::uint32_t) {};
        drive.body = [this](std::size_t) {
            TrafficCarBody body;
            body.node = node;
            body.halfExtents = {1.0f, 0.75f, 2.0f};
            body.radius = std::sqrt(1.0f + 0.75f * 0.75f + 4.0f);
            body.gearSpeed = 40.0f;
            return std::optional<TrafficCarBody>(body);
        };
        drive.bodies = bodies.hooks(bodyWorld);
        TrafficDriveResult result = drivers.drive(manager, world, functions, drive, random);
        for (const TrafficCarMove& move : result.moves) node = move.node;
        events.insert(events.end(), result.events.begin(), result.events.end());
        traffic.insert(traffic.end(), result.traffic.begin(), result.traffic.end());
    }

    const AiDriverRecord& driver() const { return drivers.car(0).driver; }
    bool saw(TrafficDriveEventKind kind) const {
        return std::any_of(events.begin(), events.end(), [kind](const TrafficDriveEvent& event) { return event.kind == kind; });
    }
};

void spawnSteps() {
    Run run;
    require(run.bodies.body(0).rigidFlags == 0x182u, "0x1796e8 makes the car kinematic");
    require(run.driver().state == kAiStateHold && run.driver().requestedState == kAiStateStart && run.driver().route == kRoadIn &&
                run.driver().signal == 1 && run.driver().routeDistance == 1.0f && run.driver().heightOffset == kHeight,
            "the spawn writes 0/1, the route, its signal object, the parameter and +0x74");
    run.step();
    require(run.driver().state == kAiStateStart && run.driver().requestedState == kAiStateRoute, "0 -> 1, then 1 requests 2");
    require(run.driver().keep >= 29 && run.driver().keep <= 88, "+0xad = random % 60 + 30, less the post step's count down");
    require(run.driver().target[2] == 11.0f && run.driver().target[1] == kHeight, "+0xc is the route point at +0x64 + 0.1, raised by 0x20af24");
    require(run.manager.cars()[0].driveState == 1 && run.manager.cars()[0].driveStage == 2, "the manager's view takes +0x78 / +0x79");
    run.step();
    require(run.driver().state == kAiStateRoute && run.driver().cruiseSpeed >= 8.0f && run.driver().cruiseSpeed < 12.0f, "state 2 with +0x6c in 8 .. 12");
    require(run.manager.cars()[0].route == 0u, "the manager's view of the route is its index in the route list");
    for (int update = 0; update < 5; ++update) run.step();
    const AiVec3& velocity = run.bodies.body(0).velocity;
    require(velocity[2] > 0.0f && velocity[0] == 0.0f && velocity[1] == 0.0f, "the mover stores the kinematic body's velocity along the node's forward row");
}

void movesStopsAndGoes() {
    Run run;
    for (int i = 0; i < 30; ++i) run.step();
    require(run.node[3][2] > 11.0f && run.node[3][0] == 0.0f && run.node[3][1] == kHeight, "the car moves along the road");
    require(run.driver().speed > 1.0f && run.driver().speed <= run.driver().cruiseSpeed, "the speed moves to +0x70 = +0x6c");
    require(run.manager.cars()[0].position == run.node[3] && run.manager.cars()[0].driveValue == run.driver().speed, "the manager sees the move");

    for (int i = 0; i < 600 && !run.saw(TrafficDriveEventKind::Stop); ++i) run.step();
    require(run.saw(TrafficDriveEventKind::Stop) && run.events.back().signal == 1, "a red light of the junction stops the car near the road end");
    require(run.driver().routeDistance >= 4.0f, "the stop test runs from length - 2 on");
    for (int i = 0; i < 300; ++i) run.step();
    require(run.driver().speed == 0.0f && run.driver().targetSpeed == 0.0f && run.drivers.car(0).vehicle.brakeInput == 1.0f,
            "the car stands with the brake input 1.0");
    const float stoppedAt = run.node[3][2];
    require(stoppedAt < 60.0f && run.driver().state == kAiStateRoute, "it stays on its road before the junction");
    run.step();
    require(run.node[3][2] == stoppedAt, "and does not move while the light is red");

    run.world.junctions[0].lightPhase = kTrafficLightGreen;
    for (int i = 0; i < 600 && !run.saw(TrafficDriveEventKind::Junction); ++i) run.step();
    require(run.saw(TrafficDriveEventKind::Go), "the green light of the passing pair releases the brake");
    require(run.saw(TrafficDriveEventKind::Junction) && run.driver().route == kLane && run.driver().nextRoute == kRoadOut,
            "at the road end the car takes the lane to the chosen output (state 3)");
    require(run.world.junctions[0].queuedCars == std::vector<std::uint32_t>{0} && (run.driver().flags & kAiDriverFlagSignal) != 0,
            "the car is queued at the junction");

    for (int i = 0; i < 600 && !run.saw(TrafficDriveEventKind::Road); ++i) run.step();
    require(run.saw(TrafficDriveEventKind::Road) && run.driver().route == kRoadOut && run.driver().signal == 0, "past 2.8 on the lane the next road");
    require(run.world.junctions[0].queuedCars.empty() && (run.driver().flags & kAiDriverFlagSignal) == 0, "the car left the junction queue");
    require(run.manager.cars()[0].route == 1u, "the manager's view has the output road");

    for (int i = 0; i < 900 && run.traffic.empty(); ++i) run.step();
    require(run.traffic.size() == 1 && run.traffic[0].kind == TrafficEventKind::Despawn && run.manager.cars()[0].disabled,
            "a road into no junction ends in the leave call: the manager removes the car");
    const AiMatrix last = run.node;
    run.step();
    require(run.node == last, "a removed car is not updated");
}

void removalLeavesQueue() {
    Run run;
    run.world.junctions[0].lightPhase = kTrafficLightGreen;
    for (int i = 0; i < 900 && !run.saw(TrafficDriveEventKind::Junction); ++i) run.step();
    require(run.world.junctions[0].queuedCars.size() == 1, "the car is queued on the lane");
    run.drivers.removed({{TrafficEventKind::Spawn, 0}}, run.world);
    require(run.world.junctions[0].queuedCars.size() == 1, "only a removal sends 0x80000006");
    run.drivers.removed({{TrafficEventKind::Despawn, 0}}, run.world);
    require(run.world.junctions[0].queuedCars.empty() && (run.driver().flags & kAiDriverFlagSignal) == 0,
            "the removal message 0x80000006 takes the car out of the junction queue (0x19293c -> 0x22a080)");
    run.drivers.removed(0, run.world);
    require(run.world.junctions[0].queuedCars.empty(), "without +0xaf bit 3 the queue is left alone");
}

void whistleAndHeldCar() {
    Run run;
    for (int i = 0; i < 30; ++i) run.step();
    require(run.driver().targetSpeed > 0.0f, "the car drives");
    run.drivers.whistle().until = 1;
    run.step();
    require(run.driver().targetSpeed == 0.0f, "while the whistle stop runs (clock 0 below its end 1) every car's +0x70 is 0 (0x2c87a8)");
    run.drivers.whistle().until = 0;
    run.drivers.held().entity = 1;
    run.drivers.removed({{TrafficEventKind::Despawn, 0}}, run.world);
    require(run.drivers.held().entity == 0 && (run.driver().flags & kAiDriverFlagAi) == 0 && run.driver().requestedState == 0,
            "the removal message releases the held car (0x22a1f4: +0xaf bit 0 and +0x79 cleared)");
}

void obstacleStops() {
    Run run;
    for (int i = 0; i < 40; ++i) run.step();
    TrafficObstacle ahead;
    ahead.entity.node = pedestrianPlacement({0.0f, kHeight, run.node[3][2] + 6.0f}, {0.0f, 0.0f, 1.0f});
    ahead.entity.halfExtents = {1.0f, 0.75f, 2.0f};
    TrafficObstacle own = ahead;
    own.car = 0;
    own.entity.node[3][2] = run.node[3][2] + 1.0f;
    run.obstacles = {own};
    for (int i = 0; i < 5; ++i) run.step();
    require(run.driver().targetSpeed > 0.0f && run.driver().hold == 0, "the car's own entry of the list is skipped");
    run.obstacles = {own, ahead};
    for (int i = 0; i < 5; ++i) run.step();
    require(run.driver().targetSpeed == 0.0f && run.driver().hold > 0, "an entity ahead within reach stops the car (+0x70 = 0, hold 8)");
}

void follower() {
    TrafficJunctions junctions;
    chase::assets::PregenSpawns spawns;
    spawns.points = {{{0.0f, 0.0f, 20.0f}, 0, kRoadIn, 2.0f}, {{0.0f, 0.0f, 30.0f}, 0, kRoadOut, 3.0f}, {{0.0f, 0.0f, -5.0f}, 0, kRoadIn, 0.5f}};
    chase::assets::PregenSpawnCell root;
    root.halfSize = 1000;
    root.points = {0, 1, 2};
    spawns.cells = {root};
    TrafficRouteTable table;
    table.loaded = true;
    table.routes = {kRoadIn, kRoadOut};
    table.enabled = {true, true};
    const AiRouteFunctions functions = trafficDriverRouteFunctions(junctions, spawns, table);
    AiMatrix matrix = pedestrianPlacement({0.0f, 0.0f, 10.0f}, {0.0f, 0.0f, 1.0f});
    AiVec3 target{};
    std::uint32_t route = 0;
    float distance = 0.0f;
    require(functions.follow({0.0f, 0.0f, 18.0f}, matrix, target, route, distance) && route == kRoadIn && distance == 2.0f && target[2] == 20.0f,
            "the follower takes the nearest enabled point ahead of the probe");
    table.enabled[0] = false;
    require(functions.follow({0.0f, 0.0f, 18.0f}, matrix, target, route, distance) && route == kRoadOut && distance == 3.0f,
            "a disabled route is skipped");
    matrix[2] = {0.0f, 0.0f, -1.0f};
    table.enabled[0] = true;
    route = 7;
    require(functions.follow({0.0f, 0.0f, 18.0f}, matrix, target, route, distance) && route == kRoadIn && distance == 0.5f,
            "only points ahead of the forward axis count");
    matrix[2] = {1.0f, 0.0f, 0.0f};
    route = 7;
    require(!functions.follow({0.0f, 0.0f, 18.0f}, matrix, target, route, distance) && route == 7, "no point: nothing written");
}

}

int main() {
    try {
        spawnSteps();
        movesStopsAndGoes();
        removalLeavesQueue();
        whistleAndHeldCar();
        obstacleStops();
        follower();
    } catch (const std::exception& error) {
        std::cerr << "traffic-driver-connect: " << error.what() << '\n';
        return 1;
    }
    std::cout << "traffic-driver-connect: ok\n";
    return 0;
}
