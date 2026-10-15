// Vehicle records and flag natives (src/runtime/vehicle.cpp, docs/re/vehicles-system.md). The synthetic part pins the parameter reader 0x23ed0c and the
// five handlers (0x26fe78, 0x29ea7c, 0x2b9df0, 0x27c014, 0x29eaa8) as read from the disassembly. With FIB NAMES the real part checks lvl_01 against
// the vehicle records read from the original in the oracle state street-after-1-1 (GDB reads of the three parked vehicles, 2026-10-15, no pokes) and
// the collision the vehicles register. Exits with 77 without data.
#include "assets/assets.hpp"
#include "assets/object_classes.hpp"
#include "assets/object_collision.hpp"
#include "runtime/collision.hpp"
#include "runtime/level_start.hpp"
#include "runtime/script_runner.hpp"
#include "runtime/vehicle.hpp"
#include <cmath>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;
using runtime::VehicleNativeKind;
using runtime::VehicleSet;
using runtime::VehicleTableRow;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

assets::LevelParam floatParam(const char* name, float value, std::uint16_t flags = 2) {
    assets::LevelParam param;
    param.hash = assets::nameHash(name);
    param.flags = flags;
    param.type = 0;
    std::memcpy(&param.value, &value, 4);
    return param;
}

assets::LevelParam intParam(const char* name, std::uint32_t value, std::uint16_t type = 5, std::uint16_t flags = 2) {
    assets::LevelParam param;
    param.hash = assets::nameHash(name);
    param.flags = flags;
    param.type = type;
    param.value = value;
    return param;
}

assets::LevelParam float3Param(const char* name, float x, float y, float z) {
    assets::LevelParam param;
    param.hash = assets::nameHash(name);
    param.flags = 2;
    param.type = 2;
    const float v[3] = {x, y, z};
    param.blocks.emplace_back(12);
    std::memcpy(param.blocks[0].data(), v, 12);
    return param;
}

assets::LevelParam stringParam(const char* name, const std::string& text) {
    assets::LevelParam param;
    param.hash = assets::nameHash(name);
    param.flags = 2;
    param.type = 1;
    param.blocks.push_back(assets::Bytes(text.begin(), text.end()));
    param.blocks[0].push_back(0);
    return param;
}

struct Fixture {
    assets::Level vehicleClass, wheelClass;
    assets::Level level;
    std::vector<VehicleTableRow> table;

    Fixture() {
        vehicleClass.classParams = {intParam("ObjectType", 183, 7, 5)};
        vehicleClass.defaults = {intParam("VehicleType", 2, 7), intParam("MusicTrack", 0, 7), intParam("NoMusic", 0), floatParam("HealthOverride", -1.0f),
                                 intParam("Invulnerable", 0), intParam("ATTR_CameraOverride", 1, 5, 1), floatParam("ATTR_CameraPosY", 0.5f),
                                 floatParam("ATTR_PanScale", 1.0f), intParam("RC_Vehicle", 0), intParam("BigHitOnContact", 0), intParam("PlayerDisable", 0),
                                 intParam("SpawnUnshovable", 0), intParam("SpawnParked", 0), floatParam("numWheels", 2.0f, 1),
                                 float3Param("DriverPosition", 0.1f, 0.2f, 0.3f)};
        for (int n = 1; n <= 3; ++n) {
            assets::LevelObject wheel;
            wheel.className = "vehicles/wheel";
            wheel.instance = "Wheel0" + std::to_string(n);
            wheel.params = {stringParam("meshname", "Veh_Wheel"), float3Param("initialposition", float(n), -0.7f, 1.5f)};
            vehicleClass.objects.push_back(wheel);
        }
        wheelClass.defaults = {stringParam("meshname", ""), float3Param("initialposition", 0, 0, 0), float3Param("initialorientation", 0, 0, 0)};
        table = {VehicleTableRow{}, VehicleTableRow{}, VehicleTableRow{}};
        table[2].health = 4;
        table[1].health = 5;
        assets::LevelObject plain;
        plain.className = "vehicles/test";
        plain.instance = "Plain";
        level.objects.push_back(plain);
        assets::LevelObject parked = plain;
        parked.instance = "Parked";
        parked.params = {intParam("SpawnParked", 1)};
        level.objects.push_back(parked);
        assets::LevelObject both = plain;
        both.instance = "Both";
        both.params = {intParam("SpawnParked", 1), intParam("SpawnUnshovable", 1), intParam("VehicleType", 1, 7), intParam("NoMusic", 1),
                       floatParam("HealthOverride", 9.0f), intParam("Invulnerable", 1), intParam("PlayerDisable", 1), intParam("RC_Vehicle", 1),
                       intParam("BigHitOnContact", 1), floatParam("ATTR_PanScale", 2.0f)};
        level.objects.push_back(both);
        assets::LevelObject prop;
        prop.className = "props/prop_crate";
        prop.instance = "Crate";
        level.objects.push_back(prop);
    }

    assets::ClassLookup lookup() const {
        return [this](const std::string& name) -> const assets::Level* {
            if (name == "vehicles/test") return &vehicleClass;
            if (name == "vehicles/wheel") return &wheelClass;
            return nullptr;
        };
    }
};

void parameterReader() {
    Fixture fixture;
    VehicleSet set;
    set.load(fixture.level, fixture.lookup(), fixture.table);
    require(set.records().size() == 3, "only objects of a class with ObjectType 183 get a record");
    const runtime::VehicleRecord* plain = set.find("plain");
    const runtime::VehicleRecord* parked = set.find("parked");
    const runtime::VehicleRecord* both = set.find("both");
    require(plain && parked && both && !set.find("crate"), "records by lower-case name");

    require(plain->requestedState == 0 && plain->state == 0, "no spawn parameter leaves the state words zero (record zero-filled by the create function)");
    require(parked->requestedState == runtime::kVehicleStateParked, "SpawnParked sets +4 = 4");
    require(both->requestedState == runtime::kVehicleStateUnshovable, "SpawnUnshovable wins over SpawnParked: +4 = 10");

    require(plain->vehicleType == 2 && plain->health == 4 && plain->healthStart == 4, "health comes from the vehicle table row of VehicleType");
    require(both->vehicleType == 1 && both->health == 9 && both->healthStart == 9, "HealthOverride above zero replaces both health bytes");
    require(plain->musicEnabled() && !both->musicEnabled(), "NoMusic clears bit 2 of +0x312, no NoMusic sets it");
    require(plain->flags311 == runtime::kVehicleCameraOverride && plain->flags312 == runtime::kVehicleMusicEnabled,
            "a parked or traffic vehicle reads +0x311 = 1 and +0x312 = 4 (oracle, street-after-1-1)");
    require(both->flags311 == (runtime::kVehicleCameraOverride | runtime::kVehicleInvulnerable | runtime::kVehicleLocked | runtime::kVehicleRemoteControl | runtime::kVehicleBigHitOnContact),
            "Invulnerable, PlayerDisable (locked), RC_Vehicle and BigHitOnContact set bits 2, 1, 4 and 5");
    require(plain->cameraPosY == 0.5f && plain->cameraPanScale == 1.0f && both->cameraPanScale == 2.0f, "camera floats");
    require(plain->hasDriverPosition && plain->driverPosition[2] == 0.3f, "DriverPosition float3");

    require(plain->wheelCount == 2 && plain->wheels.size() == 2 && plain->wheels[1].instance == "Wheel02", "numWheels children Wheel01..Wheel02 are found by name");
    require(plain->wheels[1].local.position[0] == 2.0f, "wheel pose from the class file child");

    set.settle();
    require(set.find("parked")->state == runtime::kVehicleStateParked && set.find("plain")->state == 0, "the first update copies +4 to +2");
}

void natives() {
    Fixture fixture;
    VehicleSet set;
    set.load(fixture.level, fixture.lookup(), fixture.table);
    require(runtime::vehicleNativeKind("vehiclesetlocked") == VehicleNativeKind::SetLocked && runtime::vehicleNativeKind("vehiclejumpout") == VehicleNativeKind::JumpOut &&
                runtime::vehicleNativeKind("stopplayervehicle") == VehicleNativeKind::StopPlayerVehicle && !runtime::vehicleNativeKind("vehiclestop"),
            "native names");
    require(runtime::vehicleNativeArgs(VehicleNativeKind::SetLocked) == runtime::VehicleNativeArgs::NameValue &&
                runtime::vehicleNativeArgs(VehicleNativeKind::SetUnshovable) == runtime::VehicleNativeArgs::Name &&
                runtime::vehicleNativeArgs(VehicleNativeKind::JumpIn) == runtime::VehicleNativeArgs::NameName &&
                runtime::vehicleNativeArgs(VehicleNativeKind::StopPlayerVehicle) == runtime::VehicleNativeArgs::Value,
            "VehicleSetUnshovable has only the vehicle argument, StopPlayerVehicle only a value (handler 0x27c040 reads the first value)");

    require(set.apply(VehicleNativeKind::SetLocked, "plain", 1.0f) && set.find("plain")->locked(), "VehicleSetLocked 1: bit 1 of +0x311");
    set.apply(VehicleNativeKind::SetLocked, "plain", 0.0f);
    require(!set.find("plain")->locked() && set.find("plain")->flags311 == runtime::kVehicleCameraOverride, "VehicleSetLocked 0 clears only bit 1");

    set.apply(VehicleNativeKind::SetForcedStop, "plain", 1.0f);
    require(set.find("plain")->forcedStop == 1, "VehicleSetForcedStop writes byte +0x310");
    set.apply(VehicleNativeKind::SetForcedStop, "plain", 0.0f);
    require(set.find("plain")->forcedStop == 0, "VehicleSetForcedStop 0");

    set.apply(VehicleNativeKind::AllowJumpOut, "plain", 0.0f);
    require(!set.find("plain")->jumpOutAllowed() && (set.find("plain")->flags311 & runtime::kVehicleJumpOutDenied), "VehicleAllowJumpOut 0 sets bit 6");
    set.apply(VehicleNativeKind::AllowJumpOut, "plain", 1.0f);
    require(set.find("plain")->jumpOutAllowed(), "VehicleAllowJumpOut 1 clears bit 6");

    require(set.apply(VehicleNativeKind::SetUnshovable, "plain", 0.0f) && set.find("plain")->requestedState == runtime::kVehicleStateUnshovable,
            "VehicleSetUnshovable writes +4 = 10");
    VehicleSet blocked;
    blocked.load(fixture.level, fixture.lookup(), fixture.table);
    const_cast<runtime::VehicleRecord&>(*blocked.find("plain")).state = 6;
    require(!blocked.apply(VehicleNativeKind::SetUnshovable, "plain", 0.0f) && blocked.find("plain")->requestedState == 0,
            "VehicleSetUnshovable returns 0 and writes nothing while the state is 6 (or 7)");
    const_cast<runtime::VehicleRecord&>(*blocked.find("plain")).state = 7;
    require(!blocked.apply(VehicleNativeKind::SetUnshovable, "plain", 0.0f), "state 7");

    require(set.apply(VehicleNativeKind::SetForcedStopOnMyVehicle, "player1", 1.0f) && set.find("parked")->forcedStop == 0,
            "SetForcedStopOnMyVehicle does nothing for a character on foot");
    set.setOccupantLookup([](const std::string& character) { return character == "player1" ? std::string("Parked") : std::string(); });
    set.apply(VehicleNativeKind::SetForcedStopOnMyVehicle, "player1", 1.0f);
    require(set.find("parked")->forcedStop == 1 && set.find("plain")->forcedStop == 0, "SetForcedStopOnMyVehicle sets +0x310 of the character's vehicle");
    set.apply(VehicleNativeKind::SetForcedStopOnMyVehicle, "player1", 0.0f);
    require(set.find("parked")->forcedStop == 0, "and clears it");

    require(set.apply(VehicleNativeKind::SetLocked, "nobody", 1.0f), "an unknown vehicle is ignored");
}

void boardingNatives() {
    Fixture fixture;
    VehicleSet set;
    set.load(fixture.level, fixture.lookup(), fixture.table);
    std::vector<std::pair<runtime::BoardingRequestKind, std::string>> requests;
    set.setBoardingHandler([&](runtime::BoardingRequestKind kind, const std::string& vehicle) { requests.emplace_back(kind, vehicle); });
    require(set.apply(VehicleNativeKind::JumpOut, "player1", 0.0f) && requests.empty(), "VehicleJumpOut does nothing for a player on foot (0x24255c is 0, the state test 0x17f fails)");
    set.setOccupantLookup([](const std::string& character) { return character == "player1" ? std::string("Parked") : std::string(); });
    set.apply(VehicleNativeKind::JumpOut, "someone_else", 0.0f);
    require(requests.empty(), "only the player's seat is modelled; other characters are ignored");
    set.apply(VehicleNativeKind::JumpOut, "player1", 0.0f);
    require(requests.size() == 1 && requests[0].first == runtime::BoardingRequestKind::JumpOut && requests[0].second == "parked",
            "VehicleJumpOut for the seated player posts event 200 (0x1da8d8)");
    set.apply(VehicleNativeKind::JumpIn, "player1", 0.0f, "plain");
    require(requests.size() == 2 && requests[1].first == runtime::BoardingRequestKind::JumpIn && requests[1].second == "plain",
            "VehicleJumpIn(player, vehicle) posts event 200 with the vehicle (0x1cdec0)");
    set.apply(VehicleNativeKind::JumpIn, "player1", 0.0f, "nobody");
    require(requests.size() == 2, "VehicleJumpIn with an unknown vehicle is ignored");

    set.apply(VehicleNativeKind::StopPlayerVehicle, "", 1.0f);
    require(set.find("parked")->forcedStop == 1 && set.find("parked")->pendingSpeedLimit && *set.find("parked")->pendingSpeedLimit == 0.0f &&
                set.find("plain")->forcedStop == 0,
            "StopPlayerVehicle 1 zeroes the velocities of the player's vehicle and sets +0x310 (0x27c040; the vectors 0x56d3a0 and 0x56d400 read zero in the oracle)");
    set.apply(VehicleNativeKind::StopPlayerVehicle, "", 0.0f);
    require(set.find("parked")->forcedStop == 0, "StopPlayerVehicle 0 clears +0x310");

    set.apply(VehicleNativeKind::SetTopSpeed, "plain", 32.0f);
    require(set.find("plain")->topSpeed == 32.0f, "VehicleSetTopSpeed writes the top speed of the body (0x1afec4 stores +0x2a0)");
}

void scriptRunner() {
    Fixture fixture;
    assets::LevelScript script;
    script.nameHash = assets::nameHash("Go");
    script.calls = {{"VehicleSetLocked", {"Plain", "1"}}, {"VehicleSetForcedStop", {"Plain", "1"}}, {"VehicleAllowJumpOut", {"Plain", "0"}},
                    {"VehicleSetUnshovable", {"Plain"}}};
    fixture.level.scripts.push_back(script);
    assets::PlacedObjectState state;
    runtime::ScriptRunner runner(fixture.level, state);
    runner.loadVehicles(fixture.lookup(), fixture.table);
    require(runner.start("Go"), "script starts");
    for (int i = 0; i < 5; ++i) runner.update(0.033f);
    const runtime::VehicleRecord* plain = runner.vehicles().find("plain");
    require(plain->locked() && plain->forcedStop == 1 && !plain->jumpOutAllowed() && plain->requestedState == runtime::kVehicleStateUnshovable,
            "the script natives write the records");
    require(runtime::scriptFunctionSupport("VehicleSetLocked") == runtime::ScriptFunctionSupport::Implemented, "census support");
}

void requireNear(float actual, float expected, const std::string& what) {
    require(std::fabs(actual - expected) < 1e-5f, what + ": " + std::to_string(actual) + " against " + std::to_string(expected));
}

void real(const char* fib, const char* names) {
    assets::FuseArchive archive(fib);
    archive.loadNames(names);
    const assets::FuseEntry* entry = archive.find("levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl");
    require(entry != nullptr, "level not in archive");
    const assets::Level level = assets::parseLevel(archive.read(*entry));
    const assets::ClassLookup classOf = assets::archiveClassLookup(archive);
    const assets::FuseEntry* tableEntry = archive.find("tables/tablescharacters.bxls");
    require(tableEntry != nullptr, "vehicle table not in archive");
    const std::vector<VehicleTableRow> table = runtime::parseVehicleTable(assets::parseTable(archive.read(*tableEntry)));
    require(table.size() == 49 && table[22].name == "police_trooper" && table[22].health == 5 && table[15].health == 4,
            "tablescharacters sheet 4: 49 rows, trooper health 5, worker health 4");

    VehicleSet set;
    set.load(level, classOf, table);
    require(set.records().size() == 26 && !set.find("transitionufo"),
            "lvl_01 places 27 objects of levels/vehicles; 26 have ObjectType 183, TransitionUFO (vehicle_aircraft_ufo) has type 243 (0xf3)");

    const runtime::VehicleRecord* police = set.find("1-1_police_car");
    require(police != nullptr, "1-1_Police_Car has a record");
    require(police->requestedState == runtime::kVehicleStateUnshovable, "SpawnParked and SpawnUnshovable of 1-1_Police_Car: state 10");
    require(police->vehicleType == 22 && police->musicTrack == 12 && police->wheelCount == 4 && police->wheels.size() == 4 && police->health == 5,
            "trooper: VehicleType 22, MusicTrack 12, four wheels, health 5");
    requireNear(police->wheels[0].local.position[0], -0.85f, "Wheel01 x");
    requireNear(police->wheels[0].local.position[2], 1.52f, "Wheel01 z");
    require(police->wheels[0].local.mesh == "Veh_Wheel_20x12", "wheel model");
    require(!police->locked(), "locked comes from the scripts, not from the class");

    struct Parked { const char* instance; std::uint16_t type; std::uint8_t wheels, health; float camera[4]; };
    // Oracle (street-after-1-1, GDB reads of the records of the three parked vehicles): +2 = +4 = 4, +0x311 = 1, +0x312 = 4, health bytes 100/4/4,
    // +0x2ec type, +0x314 wheels, floats +0x2f0..+0x2fc.
    const Parked parkedVehicles[] = {{"FP_Vehicle_03", 15, 4, 4, {0.1f, 5.75f, 0.1f, 0.0f}},
                                      {"FP_Vehicle_04", 12, 4, 4, {0.2f, 7.5f, -0.6f, 0.0f}},
                                      {"FP_Vehicle_05", 11, 8, 4, {0.5f, 12.6f, -0.88f, 0.0f}}};
    for (const Parked& expected : parkedVehicles) {
        std::string lowered = expected.instance;
        for (char& c : lowered) c = char(std::tolower(static_cast<unsigned char>(c)));
        const runtime::VehicleRecord* record = set.find(lowered);
        require(record != nullptr, std::string(expected.instance) + " has a record");
        require(record->vehicleType == expected.type && record->wheelCount == expected.wheels && record->health == expected.health,
                std::string(expected.instance) + ": type, wheels, health");
        require(record->flags311 == 1 && record->flags312 == 4, std::string(expected.instance) + ": +0x311 = 1, +0x312 = 4");
        requireNear(record->cameraPosY, expected.camera[0], expected.instance);
        requireNear(record->cameraPosZ, expected.camera[1], expected.instance);
        requireNear(record->cameraLookY, expected.camera[2], expected.instance);
        requireNear(record->cameraLookZ, expected.camera[3], expected.instance);
        requireNear(record->cameraPanScale, 1.0f, expected.instance);
        require(record->wheels.size() == expected.wheels, std::string(expected.instance) + ": every Wheel%02u child exists");
    }
    int parkedCount = 0, unshovableCount = 0;
    for (const runtime::VehicleRecord& record : set.records()) {
        parkedCount += record.requestedState == runtime::kVehicleStateParked;
        unshovableCount += record.requestedState == runtime::kVehicleStateUnshovable;
    }
    std::cout << "lvl_01 vehicles: " << set.records().size() << ", SpawnParked only " << parkedCount << ", SpawnUnshovable " << unshovableCount << '\n';

    const assets::PlacedObjectState state = runtime::levelStartState(level, classOf);
    const std::vector<assets::ObjectCollision> collisions = assets::objectCollisions(archive, level, state);
    const assets::ObjectCollision* worker = nullptr;
    for (const assets::ObjectCollision& item : collisions)
        if (item.instance == "FP_Vehicle_03") worker = &item;
    require(worker != nullptr && worker->mesh != nullptr, "a parked vehicle registers the first collision item of its model");
    require(worker->mask == 6 && worker->mesh->vertices.size() / 3 == 26 && worker->mesh->triangles.size() == 48 && worker->mesh->nodes.size() == 52,
            "oracle: entity+0x78 of FP_Vehicle_03 is the item with 26 vertices, 48 triangles, 52 nodes (type 15, mask 6)");
    require(state.isDisabled("1-1_Police_Car"), "1-1_Police_Car is disabled at the level start, so it registers no collision");
    for (const assets::ObjectCollision& item : collisions) require(item.instance != "1-1_Police_Car", "disabled vehicle");

    runtime::CollisionWorld world;
    world.addObject(*worker);
    require(world.triangleCount() == 48, "vehicle triangles in the world");
    const float forwardX = 1.0f, forwardZ = 0.0f;
    const float y = worker->matrix[13] + 0.1f;
    float x = worker->matrix[12] - 6.0f;
    const float z = worker->matrix[14];
    float dx = 6.0f, dz = 0.0f;
    runtime::CollisionWorld empty;
    float freeDx = dx, freeDz = dz;
    runtime::slideAlongWalls(empty, x, y, z, forwardX, forwardZ, freeDx, freeDz);
    require(freeDx == 6.0f, "without the vehicle the player walks through");
    runtime::slideAlongWalls(world, x, y, z, forwardX, forwardZ, dx, dz);
    std::cout << "player walking 6 units along +x at the parked vehicle: allowed " << dx << '\n';
    require(dx < 6.0f, "the player is stopped by the parked vehicle");
    (void)x;
}
}

int main(int argc, char** argv) {
    try {
        parameterReader();
        natives();
        boardingNatives();
        scriptRunner();
        if (argc == 3) {
            if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
                std::cout << "vehicle-test: data not available, skipped\n";
                return 77;
            }
            real(argv[1], argv[2]);
        }
    } catch (const std::exception& error) {
        std::cerr << "vehicle-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "vehicle-test: ok\n";
    return 0;
}
