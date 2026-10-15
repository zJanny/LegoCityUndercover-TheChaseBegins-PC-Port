// The player side of boarding a vehicle (src/runtime/vehicle_boarding.cpp, docs/re/vehicles.md, entry 2026-10-15T08:15:49). The synthetic part pins the vehicle search rule
// 0x225d94, the jump in and jump out paths against positions read from the original in the oracle (Azahar, RPC reads, 2026-10-15) and the state
// requests. With FIB NAMES the real part checks the entity boxes, the centre of mass offsets and the length of the jump animation in lvl_01.
// Exits with 77 without data.
#include "assets/assets.hpp"
#include "assets/object_classes.hpp"
#include "assets/object_collision.hpp"
#include "runtime/level_start.hpp"
#include "runtime/script_runner.hpp"
#include "runtime/vehicle.hpp"
#include "runtime/vehicle_boarding.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;
using runtime::BoardingEventKind;
using runtime::BoardingPhase;
using runtime::BoardingVehicle;
using runtime::VehicleBoarding;

constexpr float kPi = 3.14159265358979f;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void requireNear(float actual, float expected, float tolerance, const std::string& what) {
    require(std::fabs(actual - expected) <= tolerance, what + ": " + std::to_string(actual) + " against " + std::to_string(expected));
}

assets::Matrix yawMatrix(float yawDegrees, float x, float y, float z) {
    const float h = yawDegrees * kPi / 180.0f, s = std::sin(h), c = std::cos(h);
    return {c, 0, -s, 0, 0, 1, 0, 0, s, 0, c, 0, x, y, z, 1};
}

BoardingVehicle box(float x, float y, float z) {
    BoardingVehicle vehicle;
    vehicle.instance = "test";
    vehicle.matrix = yawMatrix(0, x, y, z);
    const float half[3] = {1.0f, 1.0f, 2.5f};
    std::copy(half, half + 3, vehicle.boxHalf);
    return vehicle;
}

runtime::BoardingInput inputAt(float x, float y, float z, bool action) {
    runtime::BoardingInput input;
    input.action = action;
    input.player.position[0] = x;
    input.player.position[1] = y;
    input.player.position[2] = z;
    input.ground = [](float, float, float, float) { return std::optional<float>(0.0f); };
    return input;
}

void search() {
    std::vector<BoardingVehicle> vehicles = {box(0, 0, 0)};
    require(runtime::findVehicleToEnter(vehicles, inputAt(2.9f, 0, 0, true).player) == std::optional<std::size_t>(0), "1.9 m from the box side is in reach");
    require(!runtime::findVehicleToEnter(vehicles, inputAt(3.1f, 0, 0, true).player), "2.1 m from the box side is out of reach (squared distance 4.41 is not below 4.0)");
    require(runtime::findVehicleToEnter(vehicles, inputAt(2.99f, 0, 0, true).player).has_value() && !runtime::findVehicleToEnter(vehicles, inputAt(3.01f, 0, 0, true).player),
            "the limit is 2.0 m: the oracle entered at a gap of 1.99 m and did not at 2.10 m (the height above the floor does not count)");
    require(!runtime::findVehicleToEnter(vehicles, inputAt(0, 0, 4.6f, true).player), "2.1 m beyond the long side is out of reach");
    require(runtime::findVehicleToEnter(vehicles, inputAt(0, 0, 0, true).player).has_value(), "inside the box the distance is 0");

    vehicles[0].locked = true;
    require(!runtime::findVehicleToEnter(vehicles, inputAt(2, 0, 0, true).player), "a locked vehicle is skipped (record +0x311 bit 1)");
    vehicles[0].locked = false;
    vehicles[0].disabled = true;
    require(!runtime::findVehicleToEnter(vehicles, inputAt(2, 0, 0, true).player), "a disabled vehicle is skipped (owner flag bit 0)");

    vehicles = {box(0, 0, 0), box(6, 0, 0)};
    vehicles[1].instance = "near";
    require(runtime::findVehicleToEnter(vehicles, inputAt(4.4f, 0, 0, true).player) == std::optional<std::size_t>(1), "the nearer of two vehicles wins");
    runtime::BoardingPlayer carrying = inputAt(2, 0, 0, true).player;
    carrying.carryMode = 2;
    require(!runtime::findVehicleToEnter(vehicles, carrying), "a big carried object (0x225754 == 2) blocks the search");
    carrying.carryMode = 1;
    require(runtime::findVehicleToEnter(vehicles, carrying).has_value(), "a small carried object does not");

    float centre[3] = {0, 0, 0}, half[3] = {1, 1, 1}, point[3] = {3, 0, 0};
    requireNear(runtime::distanceToBox(point, centre, half), 2.0f, 1e-6f, "distance to a box side");
    const float corner[3] = {4, 5, 3};
    requireNear(runtime::distanceToBox(corner, centre, half), std::sqrt(9.0f + 4.0f), 1e-6f, "distance to a box corner ignores the y axis (0x270498 reads x and z only)");
}

struct Sample {
    float x, y, z, forwardX;
};

// Police car 1-1_Police_Car parked at (887.377, 17.812, 1117.312), yaw 224.8, DriverPosition (0, -0.3, 0), CentreOfMass (0, -0.75, 0). The player stood at
// (886.0, 16.6, 1115.6) facing (0.284, 0, 0.959) and pressed X. Positions of updates 1..21 of the state Drive Jump In, read in the oracle (pc_X2
// run of 2026-10-15, one RPC sample per update; 100 ticks per update, so u = k / 22); forward x is the x of the node's forward axis.
const Sample kPoliceJumpIn[21] = {
    {886.063f, 16.941f, 1115.678f, 0.17f},  {886.127f, 17.286f, 1115.758f, 0.04f},  {886.188f, 17.598f, 1115.834f, -0.07f},
    {886.250f, 17.903f, 1115.911f, -0.19f}, {886.313f, 18.185f, 1115.989f, -0.31f}, {886.376f, 18.439f, 1116.067f, -0.42f},
    {886.439f, 18.665f, 1116.147f, -0.53f}, {886.501f, 18.848f, 1116.224f, -0.63f}, {886.564f, 18.994f, 1116.302f, -0.71f},
    {886.627f, 19.100f, 1116.380f, -0.79f}, {886.690f, 19.163f, 1116.458f, -0.86f}, {886.752f, 19.183f, 1116.536f, -0.92f},
    {886.815f, 19.160f, 1116.614f, -0.96f}, {886.879f, 19.095f, 1116.694f, -0.99f}, {886.941f, 18.993f, 1116.771f, -1.00f},
    {887.003f, 18.855f, 1116.849f, -1.00f}, {887.066f, 18.684f, 1116.927f, -0.98f}, {887.129f, 18.485f, 1117.004f, -0.95f},
    {887.192f, 18.261f, 1117.083f, -0.91f}, {887.254f, 18.022f, 1117.161f, -0.85f}, {887.318f, 17.768f, 1117.240f, -0.78f}};

BoardingVehicle policeCar() {
    BoardingVehicle vehicle;
    vehicle.instance = "1-1_police_car";
    vehicle.matrix = yawMatrix(224.8f, 887.377f, 17.812f, 1117.312f);
    const float centre[3] = {0.0f, 0.0801f, -0.0953f}, half[3] = {0.9696f, 0.826f, 2.7641f}, driver[3] = {0, -0.3f, 0}, mass[3] = {0, -0.75f, 0};
    std::copy(centre, centre + 3, vehicle.boxCentre);
    std::copy(half, half + 3, vehicle.boxHalf);
    std::copy(driver, driver + 3, vehicle.driverPosition);
    std::copy(mass, mass + 3, vehicle.centreOfMass);
    return vehicle;
}

float forwardX(std::int16_t facing) { return std::sin(float(facing) * 2.0f * kPi / 65536.0f); }

void jumpIn() {
    const std::vector<BoardingVehicle> vehicles = {policeCar()};
    VehicleBoarding boarding(2200);
    runtime::BoardingInput input = inputAt(886.0f, 16.6f, 1115.6f, false);
    input.facing = 3004;
    runtime::BoardingStep step = boarding.update(vehicles, input);
    require(step.phase == BoardingPhase::OnFoot && !step.takesControl && step.events.empty(), "no press, no boarding");

    input.action = true;
    step = boarding.update(vehicles, input);
    require(step.phase == BoardingPhase::JumpIn && step.takesControl && step.events.size() == 1 && step.events[0].kind == BoardingEventKind::JumpInStarted,
            "the press starts state 0x11a");
    requireNear(step.position[0], 886.0f, 1e-6f, "the begin update does not move");
    require(!step.jumpAnimation, "the animation starts in the next update");

    input.action = false;
    for (int k = 1; k <= 21; ++k) {
        step = boarding.update(vehicles, input);
        const Sample& expected = kPoliceJumpIn[k - 1];
        const std::string at = " at update " + std::to_string(k);
        require(step.phase == BoardingPhase::JumpIn && step.jumpAnimation && step.visible, "jump in" + at);
        requireNear(step.position[0], expected.x, 0.02f, "x" + at);
        requireNear(step.position[1], expected.y, 0.02f, "y" + at);
        requireNear(step.position[2], expected.z, 0.02f, "z" + at);
        requireNear(forwardX(step.facing), expected.forwardX, 0.02f, "heading" + at);
    }
    step = boarding.update(vehicles, input);
    require(step.phase == BoardingPhase::Seated && step.events.size() == 1 && step.events[0].kind == BoardingEventKind::Entered,
            "update 22 reaches 2200 ticks: Drive begins and fires PlayerVehicleEntered");
    require(!step.visible, "the player node is hidden while seated (0x23f734(node, 0))");
    const float body[3] = {887.377f + 0.0f, 17.812f - 0.75f, 1117.312f};
    requireNear(step.position[0], body[0], 0.003f, "seated x (oracle 887.378)");
    requireNear(step.position[1], body[1], 0.003f, "seated y (oracle 17.074 at the node height 17.824)");
    requireNear(step.position[2], body[2], 0.003f, "seated z (oracle 1117.315)");
    require(boarding.seatedVehicle() == std::optional<std::size_t>(0), "0x24255c returns the vehicle while the state is 0x11c");
    requireNear(forwardX(step.facing), -0.7046f, 0.002f, "the player faces the vehicle heading");
}

void seatedAndLeave() {
    std::vector<BoardingVehicle> vehicles = {policeCar()};
    VehicleBoarding boarding(2200);
    runtime::BoardingInput input = inputAt(886.0f, 16.6f, 1115.6f, true);
    boarding.update(vehicles, input);
    input.action = false;
    for (int i = 0; i < 22; ++i) boarding.update(vehicles, input);
    require(boarding.phase() == BoardingPhase::Seated, "seated");
    for (int i = 0; i < 100; ++i) require(boarding.update(vehicles, input).phase == BoardingPhase::Seated, "stays seated without a press");

    input.action = true;
    input.playerFree = false;
    require(boarding.update(vehicles, input).phase == BoardingPhase::Seated,
            "a script that holds the player's controls refuses the leave (oracle: X did nothing for 5 s while the mission's camera script ran in the police car)");
    input.playerFree = true;
    vehicles[0].jumpOutAllowed = false;
    require(boarding.update(vehicles, input).phase == BoardingPhase::Seated, "VehicleAllowJumpOut 0 refuses the leave (record +0x311 bit 6)");
    vehicles[0].jumpOutAllowed = true;
    const runtime::BoardingStep step = boarding.update(vehicles, input);
    require(step.phase == BoardingPhase::JumpOut && step.events.size() == 1 && step.events[0].kind == BoardingEventKind::Exiting && step.visible,
            "the press starts state 0x11b, fires PlayerVehicleExiting and shows the player");
    require(!boarding.seatedVehicle(), "0x24255c returns 0 during the jump out");
}

// van_dulmen as the user's state in-vehicle had it: node (613.369, 17.972, 1062.516), forward (-0.1995, 0, 0.9795), DriverPosition (0, -0.2, 0). X pressed
// at 0.3 s; the player started at the seat (613.369, 17.772, 1062.516) and landed at (615.329, 16.604, 1062.915) 22 updates later.
const Sample kVanJumpOut[21] = {
    {613.458f, 18.015f, 1062.534f, 0}, {613.552f, 18.269f, 1062.553f, 0}, {613.637f, 18.485f, 1062.570f, 0}, {613.726f, 18.695f, 1062.589f, 0},
    {613.815f, 18.882f, 1062.607f, 0}, {613.906f, 19.043f, 1062.625f, 0}, {613.994f, 19.168f, 1062.643f, 0}, {614.083f, 19.258f, 1062.661f, 0},
    {614.181f, 19.312f, 1062.681f, 0}, {614.262f, 19.319f, 1062.698f, 0}, {614.351f, 19.287f, 1062.716f, 0}, {614.440f, 19.212f, 1062.734f, 0},
    {614.530f, 19.093f, 1062.752f, 0}, {614.619f, 18.934f, 1062.771f, 0}, {614.708f, 18.736f, 1062.789f, 0}, {614.797f, 18.502f, 1062.807f, 0},
    {614.887f, 18.234f, 1062.825f, 0}, {614.976f, 17.940f, 1062.843f, 0}, {615.065f, 17.623f, 1062.862f, 0}, {615.154f, 17.288f, 1062.880f, 0},
    {615.246f, 16.932f, 1062.898f, 0}};

void jumpOut() {
    BoardingVehicle van;
    van.instance = "van";
    van.matrix = yawMatrix(-11.5f, 613.369f, 17.972f, 1062.516f);
    const float half[3] = {0.7726f, 1.0601f, 2.1907f}, driver[3] = {0, -0.2f, 0};
    std::copy(half, half + 3, van.boxHalf);
    std::copy(driver, driver + 3, van.driverPosition);
    const std::vector<BoardingVehicle> vehicles = {van};
    VehicleBoarding boarding(2200);
    runtime::BoardingInput input = inputAt(613.0f, 17.0f, 1062.0f, true);
    boarding.update(vehicles, input);
    input.action = false;
    for (int i = 0; i < 22; ++i) boarding.update(vehicles, input);
    input.action = true;
    input.ground = [](float, float, float, float) { return std::optional<float>(16.604f); };
    runtime::BoardingStep step = boarding.update(vehicles, input);
    require(step.phase == BoardingPhase::JumpOut, "leave starts");
    requireNear(step.position[0], 613.369f, 0.003f, "begin x");
    requireNear(step.position[1], 17.772f, 0.003f, "begin y: the player starts at the seat (oracle 17.772)");
    requireNear(step.position[2], 1062.516f, 0.003f, "begin z");
    input.action = false;
    for (int k = 1; k <= 21; ++k) {
        step = boarding.update(vehicles, input);
        const Sample& expected = kVanJumpOut[k - 1];
        const std::string at = " at update " + std::to_string(k);
        require(step.phase == BoardingPhase::JumpOut && step.jumpAnimation, "jump out" + at);
        requireNear(step.position[0], expected.x, 0.03f, "x" + at);
        requireNear(step.position[1], expected.y, 0.03f, "y" + at);
        requireNear(step.position[2], expected.z, 0.03f, "z" + at);
    }
    step = boarding.update(vehicles, input);
    require(step.phase == BoardingPhase::OnFoot && step.takesControl && step.events.size() == 1 && step.events[0].kind == BoardingEventKind::Left,
            "update 22 ends the state: the player is on foot at the exit position");
    requireNear(step.position[0], 615.329f, 0.01f, "exit x (oracle 615.329: 2.0 m to the vehicle's right)");
    requireNear(step.position[1], 16.604f, 1e-4f, "exit y is the floor");
    requireNear(step.position[2], 1062.915f, 0.01f, "exit z (oracle 1062.915)");
    require(!boarding.update(vehicles, input).takesControl, "after the landing the player moves on his own");
}

void exitCandidates() {
    BoardingVehicle car = policeCar();
    std::vector<std::array<float, 4>> asked;
    const auto firstFloor = [&](float x, float y, float z, float depth) {
        asked.push_back({x, y, z, depth});
        return asked.size() < 3 ? std::optional<float>() : std::optional<float>(5.0f);
    };
    const std::array<float, 3> exit = runtime::chooseExitPosition(car, firstFloor, 0.85f);
    require(asked.size() == 3, "the candidates are tried until one has floor below it");
    const auto reach = [&](float angleDegrees, float local[3]) {
        local[0] = 2.0f * std::sin(angleDegrees * kPi / 180.0f);
        local[1] = 0;
        local[2] = (1.0f + 2.7641f) * std::cos(angleDegrees * kPi / 180.0f);
    };
    float local[3];
    const float angles[3] = {90, 270, 0};
    for (int i = 0; i < 3; ++i) {
        reach(angles[i], local);
        const std::array<float, 3> world = runtime::boardingToWorld(car.matrix, local);
        requireNear(asked[std::size_t(i)][0], world[0], 1e-3f, "candidate x " + std::to_string(i));
        requireNear(asked[std::size_t(i)][2], world[2], 1e-3f, "candidate z " + std::to_string(i));
        requireNear(asked[std::size_t(i)][3], runtime::kExitDepth, 0, "depth");
    }
    requireNear(exit[1], 5.0f, 0, "exit height is the floor found");

    const std::array<float, 3> top = runtime::chooseExitPosition(car, [](float, float, float, float) { return std::optional<float>(); }, 0.85f);
    requireNear(top[1], 17.812f + 0.0801f + 0.826f + 0.85f, 1e-3f, "without floor the player lands on top of the vehicle (0x3225a8)");
}

void synthetic() {
    search();
    jumpIn();
    seatedAndLeave();
    jumpOut();
    exitCandidates();
    require(runtime::animationLengthTicks(12, 15.0f) == 2200, "11 intervals at 15 frames per second are 2200 ticks");
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
    runtime::VehicleSet set;
    set.load(level, classOf, runtime::parseVehicleTable(assets::parseTable(archive.read(*tableEntry))));

    struct Expected {
        const char* instance;
        float box[6];
        float mass[3];
        float driver[3];
    };
    // Entity box (+0x60 / +0x6c) read in the oracle: police car 0x08a88440 in m11-2-patrol-car; FP_Vehicle_03 from docs/re/vehicles.md (street-after-1-1).
    // The body offset is where the hidden player stood while driving: police (887.378, 17.074, 1117.315) at node (887.377, 17.824, 1117.312);
    // worker 0.2 ahead and 1.15 below; donut van 0.22 ahead and 0.897 below; van_dulmen 0.9 below.
    const Expected expected[] = {
        {"1-1_police_car", {0.0f, 0.0801f, -0.0953f, 0.9696f, 0.8260f, 2.7641f}, {0, -0.75f, 0}, {0, -0.3f, 0}},
        {"fp_vehicle_03", {0.0f, 0.2362f, -0.0770f, 0.9674f, 1.2978f, 3.0767f}, {0, -1.15f, 0.2f}, {0, 0, 1.6f}},
        {"1-1_donutvan", {0, 0, 0, 0, 0, 0}, {0, -0.9f, 0.2f}, {0, -0.4f, 1.2f}},
        {"opponentracevehicle", {0, 0, 0, 0, 0, 0}, {0, -0.9f, 0}, {0, -0.2f, 0}},
    };
    for (const Expected& want : expected) {
        const runtime::VehicleRecord* record = set.find(want.instance);
        require(record != nullptr, std::string(want.instance) + " has a record");
        for (int a = 0; a < 3; ++a) {
            requireNear(record->centreOfMass[std::size_t(a)], want.mass[a], 1e-6f, std::string(want.instance) + " CentreOfMass");
            requireNear(record->driverPosition[std::size_t(a)], want.driver[a], 1e-6f, std::string(want.instance) + " DriverPosition");
        }
    }
    const runtime::VehicleRecord* police = set.find("1-1_police_car");
    const std::size_t policeIndex = std::size_t(police - set.records().data());
    set.settle();
    set.requestState(policeIndex, runtime::kVehicleRequestDriven);
    require(set.records()[policeIndex].requestedState == 5 && set.records()[policeIndex].state == runtime::kVehicleStateUnshovable,
            "the request writes +4 (the record reads 10 here: SpawnUnshovable)");
    set.promotePending();
    require(set.records()[policeIndex].state == 5, "+2 follows one update later (oracle: (4, 5) for one update, then (5, 5))");
    set.requestState(policeIndex, runtime::kVehicleRequestHandBrake);
    set.promotePending();
    require(set.records()[policeIndex].state == 3 && set.records()[policeIndex].requestedState == 3, "the hand brake request after the jump out: (5, 3) then (3, 3)");

    const std::vector<assets::Placement> placed = assets::objectPlacements(level, classOf);
    for (const Expected& want : expected) {
        if (want.box[3] == 0) continue;
        std::string mesh;
        for (const assets::Placement& placement : placed) {
            std::string lowered = placement.instance;
            for (char& c : lowered) c = char(std::tolower(static_cast<unsigned char>(c)));
            if (lowered == want.instance) mesh = placement.mesh;
        }
        float centre[3], half[3];
        require(assets::modelCollisionBox(archive, assets::meshModelPath(mesh), centre, half), std::string(want.instance) + " has a collision item");
        for (int a = 0; a < 3; ++a) {
            requireNear(centre[a], want.box[a], 6e-5f, std::string(want.instance) + " entity box centre");
            requireNear(half[a], want.box[3 + a], 6e-5f, std::string(want.instance) + " entity box half extents");
        }
    }
    assets::PlacedObjectState state = runtime::levelStartState(level, classOf);
    runtime::ScriptRunner scripts(level, state);
    const float standing[3] = {886.0f, 16.6f, 1115.6f};
    scripts.checkTriggers(standing);
    scripts.update(0.034f);
    require(!scripts.running("1-1_Release_Car_Script", false), "the mission script does not run before the car is entered");
    state.disabled.erase("1-1_police_car");
    scripts.fireObjectEvent("playervehicleentered", "1-1_police_car");
    scripts.checkTriggers(standing, true);
    scripts.update(0.034f);
    require(scripts.running("1-1_Release_Car_Script", false), "PlayerVehicleEntered of 1-1_Police_Car starts 1-1_Release_Car_Script (mission 1-1 step 4)");

    const assets::FuseEntry* jump = archive.find("models/char_minifig/class_police/jump_minifig.bfnanm");
    require(jump != nullptr, "jump animation of the police class");
    const assets::Animation animation = assets::parseAnimation(archive.read(*jump));
    const std::uint32_t ticks = runtime::animationLengthTicks(animation.frames, assets::animationFramesPerSecond(animation));
    require(ticks == 2200, "the jump animation is 2200 ticks long: the jump in and out took 22 updates of 100 ticks in the oracle, got " + std::to_string(ticks));
}
}

int main(int argc, char** argv) {
    try {
        synthetic();
        if (argc < 3) return 0;
        if (!std::filesystem::exists(argv[1])) return 77;
        real(argv[1], argv[2]);
    } catch (const std::exception& error) {
        std::cerr << "vehicle-boarding-test: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
