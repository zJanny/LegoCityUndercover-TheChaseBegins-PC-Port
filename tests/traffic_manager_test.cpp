// Tests of src/runtime/traffic_manager.*: the traffic manager 0x527410. Every expected value follows from the disassembly
// (llvm-objdump --triple=armv6k-none-eabi --mattr=+vfp2 of artifacts/game/game.arm.elf, docs/re/traffic.md): level load 0x2dfa78, room set-up
// 0x2dedbc, update 0x2dfe88, spawn 0x2def8c, fade 0x2df8b8, SetTrafficMaxVehicles 0x2a6d70 / 0x286ca4.
#include "runtime/traffic_manager.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

TrafficFrame openFrame(TrafficManager& manager) {
    TrafficFrame frame;
    frame.player = {0.0f, 0.0f, 0.0f};
    frame.camera = {0.0f, 0.0f, 0.0f};
    for (int i = 0; i < 8; ++i) frame.cell.push_back({{40.0f * static_cast<float>(i + 1), 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, 0u, 8.0f, true});
    frame.bands = {TrafficSpawnBand{10.0f, 400.0f}, TrafficSpawnBand{10.0f, 400.0f}, TrafficSpawnBand{10.0f, 400.0f}};
    frame.place = [&manager](std::size_t car, const TrafficSpawnPoint& point) {
        manager.cars()[car].position = point.position;
        return true;
    };
    return frame;
}

void startWithOneRoute(TrafficManager& manager, const TrafficFrame& frame, GameRandom& random) {
    manager.routeTable().routes = {0x1111u};
    manager.routeTable().enabled = {true};
    manager.routeTable().loaded = true;
    manager.startRoom(frame, random, nullptr);
}

void levelLoad() {
    GameRandom random(1);
    TrafficManager manager;
    manager.loadLevel(10, std::nullopt, random);
    require(manager.maxVehicles() == 5, "Traffic_MaxVehicles defaults to 5 (0x2dfb24..0x2dfb30: 0x2451d0 default r2 = 5)");
    manager.loadLevel(10, 12u, random);
    require(manager.maxVehicles() == 10, "the attribute is limited to the count of Traffic%02u objects (0x2dfb3c: count < attribute stores the count)");
    manager.loadLevel(10, 3u, random);
    require(manager.maxVehicles() == 3, "a smaller attribute is kept (0x2dfb44..0x2dfb5c)");
    require(std::all_of(manager.fades().begin(), manager.fades().end(), [](std::uint8_t f) { return f == 255; }),
            "every car is faded in with 255 (0x2dfae0, message 0x80000007)");
    require(std::all_of(manager.cars().begin(), manager.cars().end(), [](const TrafficCar& c) { return c.disabled; }),
            "every car is disabled at the level load (0x2dfb0c: 0x2428c0)");

    GameRandom shuffle(77);
    manager.loadLevel(7, std::nullopt, shuffle);
    require(manager.order() == std::vector<std::uint32_t>({5, 1, 2, 4, 0, 3, 6}),
            "the order is the identity shuffled by count swaps of two draws of 0x25b2b8 modulo count (0x2dfb68..0x2dfbec), seed 77");
    bool threw = false;
    try {
        manager.loadLevel(33, std::nullopt, shuffle);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    require(threw, "more than 32 traffic cars are refused (the name loop 0x2dfb10 stops at 32)");
}

void maxVehicles() {
    GameRandom random(1);
    TrafficManager manager;
    manager.loadLevel(10, std::nullopt, random);
    manager.setMaxVehicles(7.0f);
    require(manager.maxVehicles() == 7, "SetTrafficMaxVehicles 7 (0x286ca4 stores the argument below the count)");
    manager.setMaxVehicles(20.0f);
    require(manager.maxVehicles() == 10, "SetTrafficMaxVehicles above the count gives the count (0x286cac movls)");
    manager.setMaxVehicles(3.9f);
    require(manager.maxVehicles() == 3, "the argument is truncated (0x2a6d7c vcvt.s32.f32)");
    manager.setMaxVehicles(-1.0f);
    require(manager.maxVehicles() == 10, "a negative argument is compared unsigned and gives the count (0x286cac cmp / movls)");
    manager.setMaxVehicles(std::numeric_limits<float>::quiet_NaN());
    require(manager.maxVehicles() == 0, "NaN converts to 0 (vcvt.s32.f32)");
    runSetTrafficMaxVehicles(manager, {4.0f});
    require(manager.maxVehicles() == 4, "the native takes its one number");
    for (const std::vector<float>& bad : {std::vector<float>{}, std::vector<float>{1.0f, 2.0f}}) {
        bool threw = false;
        try {
            runSetTrafficMaxVehicles(manager, bad);
        } catch (const std::invalid_argument& error) {
            threw = std::string(error.what()).find("SetTrafficMaxVehicles") != std::string::npos;
        }
        require(threw, "a call without exactly one number is refused with the native's name");
    }
}

std::vector<unsigned> spawnUpdates(TrafficManager& manager, TrafficFrame& frame, GameRandom& random, unsigned updates) {
    std::vector<unsigned> spawned;
    for (unsigned u = 0; u < updates; ++u) {
        std::vector<TrafficEvent> events;
        manager.update(frame, random, &events);
        for (const TrafficEvent& event : events)
            if (event.kind == TrafficEventKind::Spawn) spawned.push_back(u);
    }
    return spawned;
}

void countdown() {
    GameRandom random(5);
    TrafficManager manager;
    manager.loadLevel(10, std::nullopt, random);
    startWithOneRoute(manager, TrafficFrame{}, random);
    require(manager.countdown() == 0, "a prefill without spawn points leaves the countdown at 0 (0x2dedec, failed spawns at 0x2dffe4 keep it)");
    TrafficFrame frame = openFrame(manager);
    const std::vector<unsigned> at = spawnUpdates(manager, frame, random, 60);
    require(at == std::vector<unsigned>({0, 3, 12, 24, 42}),
            "one spawn per update; the countdown (live + live / 2) x 3 after each spawn is decremented before the test (0x2dfef8..0x2dff20, "
            "0x2e0014..0x2e0040)");
}

void prefillCount() {
    GameRandom random(11);
    TrafficManager manager;
    manager.loadLevel(10, 10u, random);
    TrafficFrame frame = openFrame(manager);
    startWithOneRoute(manager, frame, random);
    const auto live = std::count_if(manager.cars().begin(), manager.cars().end(), [](const TrafficCar& c) { return !c.disabled; });
    require(live == 6, "64 prefill updates with a limit of 10 spawn at updates 0, 3, 12, 24, 42 and 63: six cars");
    require(!manager.prefilling(), "the prefill byte +0xda4 is cleared after the 64 updates");
}

void disableRemovesAll() {
    GameRandom random(2);
    TrafficManager manager;
    manager.loadLevel(6, std::nullopt, random);
    TrafficFrame frame = openFrame(manager);
    startWithOneRoute(manager, frame, random);
    manager.cars()[manager.order().back()].pinned = true;
    manager.setEnabled(false);
    std::vector<TrafficEvent> events;
    manager.update(frame, random, &events);
    const auto live = std::count_if(manager.cars().begin(), manager.cars().end(), [](const TrafficCar& c) { return !c.disabled; });
    require(live == 1, "with bit 1 clear every live car is removed in one update except one with owner flag 0x20 (0x2dff28..0x2dff88)");
    require(events.size() == 4 && std::all_of(events.begin(), events.end(), [](const TrafficEvent& e) { return e.kind == TrafficEventKind::Despawn; }),
            "four despawn messages for the four unpinned cars");
}

TrafficManager drivingManager(GameRandom& random, TrafficFrame& frame) {
    TrafficManager manager;
    manager.loadLevel(3, std::nullopt, random);
    frame = TrafficFrame{};
    frame.bands = {TrafficSpawnBand{10.0f, 400.0f}, TrafficSpawnBand{10.0f, 400.0f}, TrafficSpawnBand{10.0f, 400.0f}};
    startWithOneRoute(manager, frame, random);
    for (TrafficCar& car : manager.cars()) {
        car.disabled = false;
        car.driveState = 2;
        car.driveStage = 2;
        car.despawnRadius = 50.0f;
        car.position = {10.0f, 0.0f, 0.0f};
    }
    return manager;
}

void despawnRules() {
    GameRandom random(4);
    TrafficFrame frame;
    {
        TrafficManager manager = drivingManager(random, frame);
        manager.cars()[1].position = {60.0f, 0.0f, 0.0f};
        manager.cars()[2].position = {70.0f, 0.0f, 0.0f};
        std::vector<TrafficEvent> events;
        manager.update(frame, random, &events);
        require(events.size() == 1 && events[0].car == 1 && manager.cars()[1].disabled && !manager.cars()[2].disabled,
                "a car beyond its despawn radius is removed, one per update (0x2e0208..0x2e02ac, return at 0x2e0274)");
        require(manager.countdown() == 9, "after a removal the countdown becomes (2 + 2 / 2) x 3 = 9 for 3 live cars minus 1 (0x2e0250..0x2e0274)");
    }
    {
        TrafficManager manager = drivingManager(random, frame);
        manager.cars()[0].pinned = true;
        manager.cars()[0].position = {90.0f, 0.0f, 0.0f};
        manager.cars()[1].position = {90.0f, 0.0f, 0.0f};
        std::vector<TrafficEvent> events;
        manager.update(frame, random, &events);
        require(events.empty() && !manager.cars()[0].disabled && !manager.cars()[1].disabled,
                "a car with owner flag 0x20 is not removed and ends the update's removal pass (0x2e0250)");
    }
    {
        TrafficManager manager = drivingManager(random, frame);
        manager.cars()[0].keepWhenFar = true;
        manager.cars()[0].position = {90.0f, 0.0f, 0.0f};
        manager.cars()[1].driveState = 1;
        manager.cars()[1].position = {90.0f, 0.0f, 0.0f};
        manager.cars()[2].driveStage = 1;
        manager.cars()[2].position = {90.0f, 0.0f, 0.0f};
        std::vector<TrafficEvent> events;
        manager.update(frame, random, &events);
        require(events.empty(), "driver +0xad keeps a far car; driver +0x78 = 1 or +0x79 = 1 skips the car (0x2e0094..0x2e00a8, 0x2e0208)");
    }
    {
        TrafficManager manager = drivingManager(random, frame);
        manager.cars()[0].driveState = 0;
        std::vector<TrafficEvent> events;
        manager.update(frame, random, &events);
        require(events.size() == 1 && events[0].car == 0, "a car whose driver +0x78 is 0 is removed near the player (0x2e0170..0x2e0180)");
    }
    {
        TrafficManager manager = drivingManager(random, frame);
        manager.cars()[0].roomActive = false;
        std::vector<TrafficEvent> events;
        manager.update(frame, random, &events);
        require(events.size() == 1 && events[0].car == 0, "a car outside the active rooms is removed (0x2e0128..0x2e0154)");
    }
    {
        TrafficManager manager = drivingManager(random, frame);
        frame.fadeBounds.push_back({{10.0f, 0.0f, -10.0f}, {5.0f, 5.0f, 40.0f}, std::nullopt});
        for (TrafficCar& car : manager.cars()) car.aiDriven = true;
        manager.cars()[1].driveValue = 1.0f;
        manager.cars()[1].position = {10.0f, 0.0f, -6.8f};
        manager.cars()[2].position = {10.0f, 0.0f, -6.8f};
        std::vector<TrafficEvent> events;
        manager.update(frame, random, &events);
        frame.fadeBounds.clear();
        require(events.size() == 4 && events[0].fade == 63 && events[1].fade == 20 && events[2].fade == 20 &&
                    events[3].kind == TrafficEventKind::Despawn && events[3].car == 2,
                "states 2 and 3 refresh the fade (10 / 40 x 255 = 63, 3.2 / 40 x 255 = 20); a fade of 32 or less removes the car unless driver +0x68 "
                "is above 0 (0x2e00ac..0x2e00c4, 0x2e0184..0x2e01a8)");
    }
}

void fadeRule() {
    GameRandom random(6);
    TrafficManager manager;
    manager.loadLevel(1, std::nullopt, random);
    TrafficFrame frame;
    frame.fadeBounds.push_back({{0.0f, 0.0f, 0.0f}, {2.0f, 2.0f, 10.0f}, std::nullopt});
    manager.cars()[0].position = {1.0f, 1.0f, 5.0f};
    std::vector<TrafficEvent> events;
    manager.refreshFade(0, frame, &events);
    require(events.empty() && manager.fades()[0] == 255, "a car never spawned (driver +0xaf bit 0 clear) keeps 255 (0x2df8fc)");
    manager.cars()[0].aiDriven = true;
    manager.refreshFade(0, frame, &events);
    require(manager.fades()[0] == 127 && events.size() == 1, "inside the box the fade is z / half z x 255 truncated: 5 / 10 x 255 = 127 (0x2df9cc..0x2dfa24)");
    manager.cars()[0].position = {1.0f, 1.0f, -5.0f};
    manager.refreshFade(0, frame, &events);
    require(manager.fades()[0] == 0, "behind the centre (z below 0) the fade is 0 (0x2df9b0..0x2df9bc)");
    manager.cars()[0].position = {2.0f, 1.0f, 5.0f};
    manager.refreshFade(0, frame, &events);
    require(manager.fades()[0] == 255, "on the box edge (|x| = half x) the car is outside: 255 (bhs at 0x2df974)");
    frame.fadeBounds[0].rows = std::array<TrafficVec3, 3>{TrafficVec3{0.0f, 0.0f, 1.0f}, TrafficVec3{0.0f, 1.0f, 0.0f}, TrafficVec3{1.0f, 0.0f, 0.0f}};
    manager.cars()[0].position = {5.0f, 0.0f, 1.0f};
    manager.refreshFade(0, frame, &events);
    require(manager.fades()[0] == 127, "the delta is turned by the room matrix rows first (0x229a40: out i = dot(delta, row i))");
    std::size_t before = events.size();
    manager.refreshFade(0, frame, &events);
    require(events.size() == before + 1, "the stored byte 127 differs from 127.5, so the message repeats every call (0x2dfa08 compares the float)");
    manager.cars()[0].position = {50.0f, 0.0f, 0.0f};
    manager.refreshFade(0, frame, &events);
    before = events.size();
    manager.refreshFade(0, frame, &events);
    require(manager.fades()[0] == 255 && events.size() == before, "an unchanged fade sends no message (0x2dfa08 vcmp)");
}

void spawnRule() {
    GameRandom random(8);
    TrafficManager manager;
    manager.loadLevel(2, std::nullopt, random);
    TrafficFrame frame;
    frame.bands = {TrafficSpawnBand{20.0f, 100.0f}, TrafficSpawnBand{20.0f, 100.0f}, TrafficSpawnBand{20.0f, 100.0f}};
    frame.cell.push_back({{10.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, 0u, 8.0f, true});
    TrafficFrame prefill = frame;
    startWithOneRoute(manager, prefill, random);
    require(!manager.cars()[0].disabled || !manager.cars()[1].disabled, "the prefill band starts at 0 m: a point 10 m from the player spawns (0x2df0e4..0x2df0ec)");
    require(manager.cars()[manager.order().back()].driveStage == 1 && manager.cars()[manager.order().back()].aiDriven &&
                manager.cars()[manager.order().back()].pathParameter == 8.0f,
            "a spawned car gets driver +0x79 = 1, +0x78 = 0, +0xaf bit 0 and the point's path parameter in +0x64 (0x2df224..0x2df264, 0x2df384)");

    TrafficManager plain;
    plain.loadLevel(2, std::nullopt, random);
    startWithOneRoute(plain, TrafficFrame{}, random);
    require(!plain.trySpawn(0, frame, random, nullptr), "outside the prefill 10 m is below 0.7 x near = 14 m (0x2df0f4..0x2df124)");
    frame.cell[0].position = {170.0f, 0.0f, 0.0f};
    require(plain.trySpawn(0, frame, random, nullptr), "170 m lies inside 14 m .. 1.75 x far = 175 m");
    frame.cell[0].position = {180.0f, 0.0f, 0.0f};
    require(!plain.trySpawn(1, frame, random, nullptr), "180 m is beyond 175 m (0x2df128)");

    frame.cell[0].position = {100.0f, 50.0f, 0.0f};
    plain.cars()[0].position = {100.0f, 0.0f, 15.0f};
    plain.cars()[0].driveStage = 2;
    require(!plain.trySpawn(1, frame, random, nullptr), "a driver on the same route within 16 m in x and z blocks the point; y is ignored (0x2df150..0x2df1a8, 0x20af40)");
    plain.cars()[0].disabled = true;
    require(!plain.trySpawn(1, frame, random, nullptr), "a disabled car still blocks by its driver record (the loop does not test owner bit 0)");
    plain.cars()[0].position = {100.0f, 0.0f, 16.0f};
    require(plain.trySpawn(1, frame, random, nullptr), "at 16 m the point is free (256 > d is false)");
    plain.cars()[1].disabled = true;
    plain.cars()[0].position = {100.0f, 0.0f, 1.0f};
    plain.cars()[0].driveStage = 0;
    plain.cars()[0].driveState = 0;
    require(plain.trySpawn(1, frame, random, nullptr), "a driver with +0x78 and +0x79 both 0 does not block (0x2df174..0x2df184)");

    plain.cars()[1].disabled = true;
    plain.setRouteEnabled(0, false);
    require(!plain.trySpawn(1, frame, random, nullptr), "a point whose route bit (+0x2c4) is clear is skipped (0x2df048..0x2df060)");
    plain.setRouteEnabled(0, true);
    frame.cell[0].roomActive = false;
    require(!plain.trySpawn(1, frame, random, nullptr), "a point whose room is not active is skipped (0x2df064..0x2df118)");
    frame.cell[0].roomActive = true;
    frame.place = [](std::size_t, const TrafficSpawnPoint&) { return false; };
    require(!plain.trySpawn(1, frame, random, nullptr) && plain.cars()[1].disabled && plain.cars()[1].driveStage == 1,
            "a refused placement (0x23091c) fails after the driver fields were written (0x2df48c..0x2df498)");
}

}

int main() {
    try {
        levelLoad();
        maxVehicles();
        countdown();
        prefillCount();
        disableRemovesAll();
        despawnRules();
        fadeRule();
        spawnRule();
    } catch (const std::exception& error) {
        std::cerr << "traffic-manager: " << error.what() << '\n';
        return 1;
    }
    std::cout << "traffic-manager: ok\n";
    return 0;
}
