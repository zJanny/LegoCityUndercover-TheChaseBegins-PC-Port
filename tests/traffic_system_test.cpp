// The traffic system (src/runtime/traffic_system.cpp, docs/re/traffic.md). Provenance: llvm-objdump of artifacts/game/game.arm.elf: level
// load 0x2dfa78, room set-up 0x2dedbc (pregen loader 0x2e02d4), update 0x2dfe88, spawn 0x2def8c (point +0x20 is the path parameter and
// driver +0x64, 0x2df380), the height offset 0x20af24 (driver +0x74 = the class value TrafficHeightOffRoad, 0x1796e8). The measured values come
// from the Azahar oracle: 20261025-traffic-C-1 and 20261025-traffic-C-2 (servant RPC), recorded from a save state (mission 1-1 done, in the
// street), 2026-10-25. With <fib> <csv> it also runs lvl_01's shipped traffic file (traffic-system-real-data, 77 without the RomFS). The cars'
// vehicle update (TrafficSystem::driveCars) gets made-up vehicles: half extents (1, 0.75, 2), gear speed 40, no ground under the probe.
#include "assets/assets.hpp"
#include "assets/pregen_traffic.hpp"
#include "runtime/dcam.hpp"
#include "runtime/game_random.hpp"
#include "runtime/pedestrian_manager.hpp"
#include "runtime/pedestrian_system.hpp"
#include "runtime/room_streaming.hpp"
#include "runtime/traffic_system.hpp"
#include "runtime/vehicle.hpp"

#include <cmath>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace chase;
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b, float tolerance) { return std::fabs(a - b) <= tolerance; }

struct Writer {
    std::vector<std::uint8_t> bytes;
    void u16(std::uint16_t v) {
        bytes.push_back(std::uint8_t(v));
        bytes.push_back(std::uint8_t(v >> 8));
    }
    void u32(std::uint32_t v) {
        for (int i = 0; i < 4; ++i) bytes.push_back(std::uint8_t(v >> (8 * i)));
    }
    void f32(float v) {
        std::uint32_t bits;
        std::memcpy(&bits, &v, sizeof bits);
        u32(bits);
    }
};

assets::LevelObject car(const std::string& instance) {
    assets::LevelObject result;
    result.className = "vehicles/vehicle_test";
    result.instance = instance;
    result.room = "Room1";
    return result;
}

assets::LevelParam floatParameter(const std::string& name, float value) {
    assets::LevelParam param;
    param.hash = assets::nameHash(name);
    param.flags = 2;
    param.type = 0;
    std::memcpy(&param.value, &value, sizeof value);
    return param;
}

assets::LevelParam bytesParameter(const std::string& name, std::vector<std::uint8_t> bytes) {
    assets::LevelParam param;
    param.hash = assets::nameHash(name);
    param.type = 2;
    param.blocks.push_back(std::move(bytes));
    return param;
}

assets::LevelObject junction(const std::string& input, const std::string& output) {
    assets::LevelObject result;
    result.className = "props/traffic_junction";
    result.instance = "Junction01";
    result.params.push_back(bytesParameter("initialposition", std::vector<std::uint8_t>(12, 0)));
    result.params.push_back(bytesParameter("InputPath01", std::vector<std::uint8_t>(input.begin(), input.end())));
    result.params.push_back(bytesParameter("OutputPath01", std::vector<std::uint8_t>(output.begin(), output.end())));
    return result;
}

assets::Level syntheticLevel() {
    assets::Level level;
    level.objects.push_back(junction("RouteA", "RouteB"));
    for (const char* name : {"Traffic01", "Traffic02", "Traffic03"}) level.objects.push_back(car(name));
    level.defaults.push_back(floatParameter("Traffic_MaxVehicles", 2.0f));
    assets::LevelPath path;
    path.nameHash = assets::nameHash("RouteA");
    path.points = {{0, 0, 0}, {10, 0, 0}, {20, 0, 0}, {30, 0, 0}};
    level.paths.push_back(path);
    path.nameHash = assets::nameHash("RouteB");
    path.points = {{40, 0, 0}, {50, 0, 0}, {60, 0, 0}, {70, 0, 0}};
    level.paths.push_back(path);
    return level;
}

std::vector<std::uint8_t> twoPointFile(const assets::LevelPath& path) {
    Writer w;
    w.u32(2);
    float parameters[2] = {1.0f, 2.8f};
    for (const float parameter : parameters) {
        float position[3];
        sampleDirectorPath(path, parameter, position);
        for (const float axis : position) w.f32(axis);
        w.u32(0);
        w.u32(path.nameHash);
        w.f32(parameter);
    }
    w.u16(1000);
    w.u16(2);
    w.f32(0);
    w.f32(0);
    w.u32(1);
    w.u32(0);
    w.u16(0);
    w.u16(1);
    return w.bytes;
}

struct World {
    std::vector<bool> enabled;
    std::set<int> loadedRooms{1};
    std::vector<TrafficCarPlacement> placements;
    std::vector<AiMatrix> nodes;
    bool refuse = false;
};

TrafficSystemStep stepAt(World& world, const TrafficVec3& player) {
    TrafficSystemStep step;
    step.player = player;
    step.camera = player;
    step.entityEnabled = [&world](std::size_t car) { return bool(world.enabled[car]); };
    step.room = [&world](int room) { return std::optional<PedestrianRoom>(pedestrianRoomObject(room, world.loadedRooms.count(room) != 0)); };
    step.layers.fill(2);
    step.place = [&world](const TrafficCarPlacement& placement) {
        if (world.refuse) return false;
        world.placements.push_back(placement);
        world.enabled[placement.car] = true;
        if (world.nodes.size() <= placement.car) world.nodes.resize(placement.car + 1);
        world.nodes[placement.car] = pedestrianPlacement(placement.position, placement.direction);
        return true;
    };
    return step;
}

void applyEvents(const std::vector<TrafficEvent>& events, World& world) {
    for (const TrafficEvent& event : events)
        if (event.kind == TrafficEventKind::Despawn) world.enabled[event.car] = false;
}

std::vector<TrafficEvent> drive(TrafficSystem& system, World& world, GameRandom& random) {
    TrafficDriveStep step;
    step.dt = 1.0f / 30.0f;
    step.ground.cast = [](const TrafficGroundSegment&) { return std::optional<TrafficGroundHit>(); };
    step.ground.sendColour = [](std::uint32_t) {};
    step.body = [&world](std::size_t car) {
        if (car >= world.nodes.size()) return std::optional<TrafficCarBody>();
        TrafficCarBody body;
        body.node = world.nodes[car];
        body.halfExtents = {1.0f, 0.75f, 2.0f};
        body.radius = std::sqrt(1.0f + 0.75f * 0.75f + 4.0f);
        body.gearSpeed = 40.0f;
        return std::optional<TrafficCarBody>(body);
    };
    const TrafficDriveResult result = system.driveCars(step, random);
    for (const TrafficCarMove& move : result.moves) world.nodes[move.car] = move.node;
    applyEvents(result.traffic, world);
    return result.traffic;
}

std::size_t count(const std::vector<TrafficEvent>& events, TrafficEventKind kind) {
    std::size_t total = 0;
    for (const TrafficEvent& event : events) total += event.kind == kind;
    return total;
}

void synthetic() {
    require(trafficCarName(1) == "Traffic01" && trafficCarName(32) == "Traffic32" && trafficFadeBoundName(4) == "Traffic_FadeBound04", "names");
    const DistanceFadeTable table;
    // Reference: the LOD table 0x529e30 at +0x3a0 .. +0x3c0 read in the oracle (20261025-traffic-servant, request TRAFFIC-1, recorded on the
    // street after mission 1-1 from a save state, three loads that agree, 2026-10-25): draw distance thresholds 3 / 6 / 12, near 36 / 72 / 108, far 38 / 76 / 114.
    require(table.radius == std::array<float, 3>({3.0f, 6.0f, 12.0f}) && table.fadeIn == std::array<float, 3>({36.0f, 72.0f, 108.0f}) &&
                table.fadeOut == std::array<float, 3>({38.0f, 76.0f, 114.0f}),
            "the LOD table of the oracle");
    const PedestrianNode node = trafficCarNode();
    require(pedestrianFadeTier(node, DistanceFadeTable{}) == 1, "the measured car node is LOD class 1 (node word bit 9, byte +0xde 0x31)");

    assets::Level level = syntheticLevel();
    assets::LevelBound bound;
    bound.name = "Traffic_Fadebound01";
    bound.centre[0] = 100;
    bound.half[0] = bound.half[1] = bound.half[2] = 5;
    level.bounds.push_back(bound);
    GameRandom random(0x77u);
    TrafficSystem system;
    const auto slots = system.loadLevel(level, [](const assets::LevelObject&) { return 1.5f; }, random);
    require(slots.size() == 3 && slots[0].instance == "Traffic01" && slots[2].heightOffset == 1.5f, "slots");
    require(system.manager().maxVehicles() == 2 && system.manager().order().size() == 3, "the limit is the level attribute limited to the car count");

    World world;
    world.enabled.assign(3, false);
    const std::vector<std::uint8_t> file = twoPointFile(level.paths[0]);
    StreamRooms rooms;
    rooms[1];
    std::map<int, RoomBox> boxes;
    boxes[1] = {{-500, -10, -500}, {500, 10, 500}};
    TrafficSceneEnter enter;
    enter.loadedLevels = {&level};
    enter.rooms = {&rooms, &boxes, 1};
    TrafficSystemStep step = stepAt(world, {10, 0, 60});

    const std::vector<TrafficEvent> none = system.enterScene(enter, step, random);
    require(none.empty() && !system.manager().routesLoaded() && system.spawnPointCount() == 0, "a missing pregen file loads nothing and spawns nothing");

    enter.pregenFile = &file;
    const std::vector<TrafficEvent> prefill = system.enterScene(enter, step, random);
    require(system.manager().routesLoaded() && system.spawnPointCount() == 2 && system.routeCount() == 2, "the junction's input and output routes");
    require(count(prefill, TrafficEventKind::Spawn) == 2 && world.placements.size() == 2, "the prefill spawns up to the limit");
    for (const TrafficCarPlacement& placement : world.placements) {
        float position[3];
        sampleDirectorPath(level.paths[0], placement.pathParameter, position);
        require(placement.position[0] == position[0] && placement.position[2] == position[2], "placed at the point");
        require(placement.position[1] == position[1] + 1.5f, "raised by the height offset 0x20af24");
        require(near(placement.direction[0], 1.0f, 1e-5f) && placement.direction[1] == 0.0f && placement.room == 1,
                "direction along the path, the point's room");
        require(placement.floorCell == -1 && placement.route == 0, "no floor graph, first route");
    }
    const std::vector<TrafficCar>& cars = system.manager().cars();
    std::size_t live = 0;
    for (const TrafficCar& entry : cars) {
        if (entry.disabled) continue;
        ++live;
        require(entry.driveStage == 1 && entry.driveState == 0, "the spawn writes the driver bytes 0/1");
    }
    require(live == 2, "two cars live");
    require(drive(system, world, random).empty(), "no car leaves");
    for (const TrafficCar& entry : cars)
        if (!entry.disabled) require(entry.driveStage == 2 && entry.driveState == 1, "the vehicle update steps them 0/1 -> 1/2");

    applyEvents(system.update(step, random), world);
    drive(system, world, random);
    for (const TrafficCar& entry : cars)
        if (!entry.disabled) require(entry.driveState == 2 && entry.driveStage == 2 && entry.keepWhenFar, "then 2/2 with the grace counter");

    std::size_t removed = 0;
    for (int i = 0; i < 120 && removed == 0; ++i) {
        const std::vector<TrafficEvent> events = system.update(step, random);
        removed += count(events, TrafficEventKind::Despawn);
        applyEvents(events, world);
        drive(system, world, random);
    }
    require(removed == 1, "a car beyond the removal radius leaves when its grace counter ran out (one per update)");

    system.setEnabled(false);
    for (std::size_t i = 0; i < world.enabled.size(); ++i) world.enabled[i] = true;
    const std::vector<TrafficEvent> off = system.update(stepAt(world, {10, 0, 60}), random);
    require(count(off, TrafficEventKind::Despawn) == 3, "EnableTrafficAndPedestrians(0) removes every live car");
    system.setEnabled(true);

    system.setMaxVehicles({5.0f});
    require(system.manager().maxVehicles() == 3, "SetTrafficMaxVehicles is limited to the car count");
    bool threw = false;
    try {
        system.setMaxVehicles({1.0f, 2.0f});
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    require(threw, "SetTrafficMaxVehicles with two numbers is rejected");

    World unloaded;
    unloaded.enabled.assign(3, false);
    unloaded.loadedRooms.clear();
    TrafficSystem second;
    GameRandom secondRandom(0x77u);
    second.loadLevel(level, [](const assets::LevelObject&) { return 0.0f; }, secondRandom);
    const std::vector<TrafficEvent> blocked = second.enterScene(enter, stepAt(unloaded, {10, 0, 60}), secondRandom);
    require(count(blocked, TrafficEventKind::Spawn) == 0, "an unloaded room refuses every spawn point");

    World refusing;
    refusing.enabled.assign(3, false);
    refusing.refuse = true;
    TrafficSystem third;
    GameRandom thirdRandom(0x77u);
    third.loadLevel(level, [](const assets::LevelObject&) { return 0.0f; }, thirdRandom);
    const std::vector<TrafficEvent> refused = third.enterScene(enter, stepAt(refusing, {10, 0, 60}), thirdRandom);
    require(count(refused, TrafficEventKind::Spawn) == 0 && !refusing.enabled[0], "a refused placement spawns nothing");

    World closed;
    closed.enabled.assign(3, false);
    TrafficSystem fourth;
    GameRandom fourthRandom(0x77u);
    fourth.loadLevel(level, [](const assets::LevelObject&) { return 0.0f; }, fourthRandom);
    fourth.enablePath("routea", false);
    const TrafficSystemStep closedStep = stepAt(closed, {10, 0, 60});
    const std::vector<TrafficEvent> disabledAtLoad = fourth.enterScene(enter, closedStep, fourthRandom);
    require(fourth.manager().routesLoaded() && count(disabledAtLoad, TrafficEventKind::Spawn) == 0 && !fourth.manager().routeTable().enabled[0],
            "a disable queued before the scene enter is replayed by it and no car spawns on that route (0x2dedbc, 0x2def8c route bit)");
    fourth.enablePath("ROUTEA", true);
    std::size_t spawnedLater = 0;
    for (int i = 0; i < 10; ++i) spawnedLater += count(fourth.update(closedStep, fourthRandom), TrafficEventKind::Spawn);
    require(spawnedLater > 0, "EnableTrafficPath(route, 1) after the load lets cars spawn on it again");

    assets::Level withoutJunction = level;
    withoutJunction.objects.erase(withoutJunction.objects.begin());
    TrafficSceneEnter plainEnter = enter;
    plainEnter.loadedLevels = {&withoutJunction};
    World plainWorld;
    plainWorld.enabled.assign(3, false);
    TrafficSystem fifth;
    GameRandom fifthRandom(0x77u);
    fifth.loadLevel(level, [](const assets::LevelObject&) { return 0.0f; }, fifthRandom);
    const std::vector<TrafficEvent> noJunctions = fifth.enterScene(plainEnter, stepAt(plainWorld, {10, 0, 60}), fifthRandom);
    require(!fifth.manager().routesLoaded() && fifth.spawnPointCount() == 0 && noJunctions.empty(),
            "without a junction within reach the route list is empty, bit 0 stays clear and the pregen data is not loaded (0x2df574)");
}

void removalRadiusInput() {
    require(TrafficSystemStep{}.despawnRadius == 50.0f, "the removal radius input defaults to 50 (0x26e130 row of the ten lvl_01 cars, TRAFFIC-S8)");
    assets::Level level = syntheticLevel();
    const std::vector<std::uint8_t> file = twoPointFile(level.paths[0]);
    StreamRooms rooms;
    rooms[1];
    std::map<int, RoomBox> boxes;
    boxes[1] = {{-500, -10, -500}, {500, 10, 500}};
    TrafficSceneEnter enter;
    enter.loadedLevels = {&level};
    enter.rooms = {&rooms, &boxes, 1};
    enter.pregenFile = &file;
    for (const float radius : {50.0f, 100000.0f}) {
        GameRandom random(0x77u);
        TrafficSystem system;
        system.loadLevel(level, [](const assets::LevelObject&) { return 1.5f; }, random);
        World world;
        world.enabled.assign(3, false);
        TrafficSystemStep step = stepAt(world, {10, 0, 60});
        step.despawnRadius = radius;
        system.enterScene(enter, step, random);
        std::size_t removed = 0;
        for (int i = 0; i < 120; ++i) {
            const std::vector<TrafficEvent> events = system.update(step, random);
            removed += count(events, TrafficEventKind::Despawn);
            applyEvents(events, world);
            drive(system, world, random);
        }
        require((radius == 50.0f) == (removed > 0), "a larger removal radius keeps the cars the default radius removes");
    }
}

struct Measured {
    const char* instance;
    float heightOffRoad, centreOfMassY;
};

int shipped(const char* fib, const char* csv) {
    if (!std::filesystem::exists(fib) || !std::filesystem::exists(csv)) {
        std::cout << "traffic-system-test: data not available, skipped\n";
        return 77;
    }
    assets::FuseArchive archive(fib);
    archive.loadNames(csv);
    const assets::FuseEntry* levelEntry = archive.find("levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl");
    const assets::FuseEntry* pregenEntry = archive.find(assets::pregenSpawnPath(assets::PregenSpawnKind::Traffic, "lvl_01_cherrytreehills"));
    const assets::FuseEntry* tableEntry = archive.find("tables/tablescharacters.bxls");
    require(levelEntry && pregenEntry && tableEntry, "lvl_01 files");
    const assets::Level level = assets::parseLevel(archive.read(*levelEntry));
    const std::vector<std::uint8_t> file = archive.read(*pregenEntry);
    const assets::ClassLookup classOf = assets::archiveClassLookup(archive);

    VehicleSet vehicles;
    vehicles.load(level, classOf, parseVehicleTable(assets::parseTable(archive.read(*tableEntry))));
    // Reference: driver +0x74 and body +0x54 of the ten Traffic cars read at the flip of their spawn (20261025-traffic-C-2, recorded from a save state on the street after mission 1-1,
    // 2026-10-25); the node y at the flip minus the pregen point's y equals their difference to four decimals for the 12 flips matched to points.
    const Measured measured[] = {{"Traffic01", 0.3299f, -0.8f}, {"Traffic02", 0.4227f, -0.8f}, {"Traffic03", 0.3306f, -0.8f},
                                 {"Traffic04", 0.8289f, -0.9f}, {"Traffic05", 0.9795f, -1.0f}, {"Traffic06", 0.3797f, -0.8f},
                                 {"Traffic07", 0.4673f, -0.9f}, {"Traffic08", 0.919f, -0.9f},  {"Traffic09", 0.9802f, -0.9f},
                                 {"Traffic10", 0.5407f, -0.75f}};
    for (const Measured& want : measured) {
        std::string lower = want.instance;
        for (char& c : lower) c = char(std::tolower(static_cast<unsigned char>(c)));
        const VehicleRecord* record = vehicles.find(lower);
        require(record != nullptr, std::string(want.instance) + ": no vehicle record");
        require(near(record->trafficHeightOffRoad, want.heightOffRoad, 1e-4f) && near(record->centreOfMass[1], want.centreOfMassY, 1e-6f),
                std::string(want.instance) + ": TrafficHeightOffRoad / CentreOfMass y");
    }

    const assets::PregenSpawns spawns = assets::parsePregenSpawns(file, assets::PregenSpawnKind::Traffic);
    const PregenRoomContext noRooms;
    const std::vector<PregenDerivedFields> fields = deriveSpawnFields(spawns, {&level}, noRooms);
    // Reference: the node forward vector at the flip of spawns matched to these points by position and path parameter (20261025-traffic-C-2).
    struct Forward {
        std::size_t point;
        float x, y, z;
    };
    const Forward forwards[] = {{361, 0.0571f, -0.0126f, -0.9983f}, {354, 1.0f, 0.0f, 0.0079f}, {366, 0.0008f, 0.1802f, -0.9836f},
                                {413, -0.0126f, -0.2144f, 0.9767f}, {424, -0.9474f, -0.0002f, 0.3201f}, {426, -0.9999f, 0.0f, -0.0157f}};
    for (const Forward& want : forwards)
        require(near(fields[want.point].tangent[0], want.x, 2e-3f) && near(fields[want.point].tangent[1], want.y, 2e-3f) &&
                    near(fields[want.point].tangent[2], want.z, 2e-3f),
                "point " + std::to_string(want.point) + ": derived tangent against the node forward");

    TrafficSystem system;
    GameRandom random(0x2f6b9d41u);
    const auto slots = system.loadLevel(level, [](const assets::LevelObject&) { return 1.0f; }, random);
    require(slots.size() == 10 && system.manager().maxVehicles() == 10, "lvl_01: ten cars, limit 10 (Traffic_MaxVehicles 10.0, oracle limit +0x36c = 10)");
    const std::map<int, RoomBox> boxes = roomBoxesOf(archive, "models/lvl_01_cherrytreehills");
    const StreamRooms rooms;
    World world;
    world.enabled.assign(slots.size(), false);
    for (const auto& [room, box] : boxes) world.loadedRooms.insert(room);
    TrafficSceneEnter enter;
    enter.loadedLevels = {&level};
    enter.pregenFile = &file;
    enter.rooms = {&rooms, &boxes, 0};
    const TrafficVec3 player{900.5491f, 16.5672f, 1110.28f};
    TrafficSystemStep step = stepAt(world, player);
    const std::vector<TrafficEvent> prefill = system.enterScene(enter, step, random);
    require(system.spawnPointCount() == 649 && system.manager().routesLoaded(), "lvl_01 spawn points");
    require(count(prefill, TrafficEventKind::Spawn) >= 1, "the prefill spawns cars around the player");
    for (const TrafficCarPlacement& placement : world.placements) {
        const float dx = placement.position[0] - player[0], dy = placement.position[1] - player[1], dz = placement.position[2] - player[2];
        require(std::sqrt(dx * dx + dy * dy + dz * dz) <= 76.0f + 2.0f, "prefill spawns lie within the far distance of class 1 (0 .. 76)");
    }
    std::size_t spawns2 = 0, removals = 0;
    for (int i = 0; i < 300; ++i) {
        const std::vector<TrafficEvent> events = system.update(step, random);
        spawns2 += count(events, TrafficEventKind::Spawn);
        removals += count(events, TrafficEventKind::Despawn);
        applyEvents(events, world);
        removals += count(drive(system, world, random), TrafficEventKind::Despawn);
    }
    require(spawns2 > 0 && removals > 0, "cars spawn and leave around the player");
    std::cout << "traffic-system-test: lvl_01 prefill spawns " << count(prefill, TrafficEventKind::Spawn) << ", then " << spawns2 << " spawns and "
              << removals << " removals in 300 updates\n";
    return 0;
}

}

int main(int argc, char** argv) {
    try {
        synthetic();
        removalRadiusInput();
        if (argc == 3) return shipped(argv[1], argv[2]);
        if (argc != 1) throw std::runtime_error("usage: traffic-system-test [<fib> <csv>]");
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << "\n";
        return 1;
    }
    return 0;
}
