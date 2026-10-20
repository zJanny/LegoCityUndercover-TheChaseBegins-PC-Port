// The pedestrian manager 0x51b33c (docs/re/pedestrians.md). Provenance of the pinned values: llvm-objdump of
// artifacts/game/game.arm.elf: slot names "Pedestrian%02u" 1..20 (0x2ee4c8, string 0x2ee558; the lvl_01 range Pedestrian02..15
// from `tools/chase_native.py level_objects` on lvl_01_cherrytreehills_main.blvl); init 0x2ee4b4 (bits 0 and 1 of
// +0xa4); update 0x2ee5e8 (spawn only while limit > live and limit <= slots, countdown - 1 clamped to 0, one random slot
// 0x235498(count), countdown 7 x live after a spawn, at most one removal per update, countdown 7 x (live - 1) after it,
// disabled manager removes every live pedestrian without the animation stop 0x172754); spawn 0x2ee068 (random % 20 >= 10
// keeps the direction, lateral (unit - 0.5) x 0.5, 11 tries, facing 0.8 at 0x2ee40c, window x 0.5 / x 1.5 at 0x2ee3f8 /
// 0x2ee400, spacing 64.0 squared xz at 0x2ee404 with 0x20af40, speed 2.0 + unit x 1.5 at 0x2ee414); the fade table words
// 3, 6, 12, 36, 72, 108, 38, 76, 114 read in the oracle (docs/re/script-natives-world.md) and their formula 0x1077dc; the
// placement rows 0x237a0c / 0x23b960.
#include "runtime/game_random.hpp"
#include "runtime/pedestrian_manager.hpp"

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

bool near(float a, float b) { return std::fabs(a - b) < 1e-5f; }

PedestrianManager managerWith(std::size_t slots, std::uint32_t limit) {
    PedestrianManager manager;
    manager.init();
    std::size_t made = 0;
    manager.loadLevel([&](const std::string&) { return made++ < slots; }, limit);
    manager.setSpawnDataLoaded(true);
    return manager;
}

struct Scene {
    std::vector<PedestrianSpawnPoint> points;
    PedestrianWorld world;
    int tier = 0;
    bool blocked = false;
    bool roomActive = true;
    bool onRoomRoot = false;
    bool hidden = false;

    Scene() {
        world.view.position = {0.0f, 0.0f, 0.0f};
        world.view.row2 = {0.0f, 0.0f, 1.0f};
        world.fadeTier = [this](std::size_t) { return tier; };
        world.spawnBlocked = [this](std::size_t, const PedestrianVec3&) { return blocked; };
        world.roomActive = [this](std::size_t) { return roomActive; };
        world.onRoomRoot = [this](std::size_t) { return onRoomRoot; };
        world.hidden = [this](std::size_t) { return hidden; };
        world.fadeNode = [](std::size_t) { return true; };
        points.push_back({{1.0f, 0.0f, 30.0f}, {0.0f, 0.0f, 1.0f}, 7, 0.25f, true});
        world.spawnPoints = &points;
    }
};

void testSlotNamesAndLevelLoad() {
    require(pedestrianSlotName(1) == "Pedestrian01" && pedestrianSlotName(20) == "Pedestrian20", "Pedestrian%02u");
    PedestrianManager manager;
    manager.init();
    const std::vector<std::size_t> numbers = manager.loadLevel(
        [](const std::string& name) { return name >= "Pedestrian02" && name <= "Pedestrian15"; }, 6);
    require(numbers.size() == 14 && numbers.front() == 2 && numbers.back() == 15, "lvl_01 has Pedestrian02 .. Pedestrian15");
    require(manager.pedestrians().size() == 14 && manager.liveCount() == 0, "every slot starts disabled");
    require(manager.limit() == 6, "Pedestrians_Max");
}

void testInitFlagsAndEnable() {
    PedestrianManager manager;
    manager.setSpawnDataLoaded(true);
    manager.init();
    require(manager.enabled() && !manager.spawnDataLoaded(), "0x2ee4b4 clears bit 0 and sets bit 1");
    manager.setSpawnDataLoaded(true);
    manager.setEnabled(false);
    require(!manager.enabled() && manager.spawnDataLoaded(), "0x2b476c changes bit 1 only");
}

void testWithoutSpawnDataNothingHappens() {
    PedestrianManager manager = managerWith(3, 2);
    manager.setSpawnDataLoaded(false);
    manager.setCountdown(5);
    Scene scene;
    GameRandom random(11);
    const GameRandom before = random;
    const PedestrianUpdate update = manager.update(scene.world, random);
    require(!update.spawned && update.removed.empty(), "no spawn data, no spawn");
    require(manager.countdown() == 5 && random.table() == before.table(), "no countdown step and no random draw");
}

void testDisabledRemovesEveryLivePedestrian() {
    PedestrianManager manager = managerWith(3, 2);
    manager.pedestrians()[0].live = true;
    manager.pedestrians()[2].live = true;
    manager.setEnabled(false);
    Scene scene;
    GameRandom random(3);
    const PedestrianUpdate update = manager.update(scene.world, random);
    require(update.removed.size() == 2 && update.removed[0].slot == 0 && update.removed[1].slot == 2, "both removed");
    require(!update.removed[0].stopAnimations && !update.removed[1].stopAnimations, "0x2428c0 only, no 0x172754");
    require(manager.liveCount() == 0, "nothing left");
}

void testLimitAboveSlotCountNeverSpawns() {
    PedestrianManager manager = managerWith(2, 3);
    manager.setCountdown(4);
    Scene scene;
    GameRandom random(5);
    const PedestrianUpdate update = manager.update(scene.world, random);
    require(!update.spawned && manager.countdown() == 4, "limit > slots: no countdown step");
}

void testCountdownStepsBeforeSpawning() {
    PedestrianManager manager = managerWith(1, 1);
    manager.setCountdown(2);
    Scene scene;
    GameRandom random(5);
    require(!manager.update(scene.world, random).spawned && manager.countdown() == 1, "2 -> 1");
    const PedestrianUpdate update = manager.update(scene.world, random);
    require(update.spawned && *update.spawned == 0, "1 -> 0 spawns");
}

void testSpawnPlacesThePedestrian() {
    PedestrianManager manager = managerWith(1, 1);
    Scene scene;
    GameRandom random(21);
    GameRandom expected = random;
    expected.next();
    const bool forward = expected.next() % 20 >= 10;
    const float lateral = (gameRandomUnit(expected) - 0.5f) * 0.5f;
    expected.next();
    const float speed = 2.0f + gameRandomUnit(expected) * 1.5f;
    const PedestrianUpdate update = manager.update(scene.world, random);
    require(update.spawned && *update.spawned == 0, "spawned in slot 0");
    require(random.table() == expected.table(), "draws: slot, direction, lateral, point, speed");
    const Pedestrian& pedestrian = manager.pedestrians()[0];
    require(pedestrian.live && pedestrian.requested == 1 && pedestrian.state == 0 && pedestrian.hold == 0, "state 1 requested");
    require(pedestrian.path == 7 && near(pedestrian.pathParameter, 0.25f), "path and parameter of the point");
    require(pedestrian.forward == forward && near(pedestrian.lateral, lateral) && near(pedestrian.speed, speed), "direction, lateral, speed");
    require(pedestrian.fadeStarted, "fade node");
    const float facing = forward ? 1.0f : -1.0f;
    require(near(pedestrian.placement[2][2], facing) && near(pedestrian.placement[3][2], 30.0f), "facing and position");
    require(manager.countdown() == 7, "7 x one live pedestrian");
}

void testSpawnWindowOfEachTier() {
    const DistanceFadeTable table;
    const PedestrianSpawnWindow none = pedestrianSpawnWindow(table, -1);
    require(near(none.minimum, 6.0f) && near(none.maximum, 162.0f), "tier -1 reads 0x52a1d8 and 0x52a1e4");
    const PedestrianSpawnWindow zero = pedestrianSpawnWindow(table, 0);
    require(near(zero.minimum, 18.0f) && near(zero.maximum, 57.0f), "tier 0");
    const PedestrianSpawnWindow two = pedestrianSpawnWindow(table, 2);
    require(near(two.minimum, 54.0f) && near(two.maximum, 171.0f), "tier 2");
    bool rejected = false;
    try {
        pedestrianSpawnWindow(table, 3);
    } catch (const std::out_of_range&) {
        rejected = true;
    }
    require(rejected, "tier 3 does not exist");
}

void requireNoSpawn(Scene& scene, PedestrianManager manager, const std::string& what) {
    GameRandom random(9);
    GameRandom expected = random;
    for (unsigned draw = 0; draw < 3 + kPedestrianSpawnTries; ++draw) expected.next();
    const PedestrianUpdate update = manager.update(scene.world, random);
    require(!update.spawned, what + ": no spawn");
    require(random.table() == expected.table(), what + ": slot, direction, lateral and 11 tries drawn");
    require(manager.countdown() == 0, what + ": countdown stays 0");
}

void testSpawnRejections() {
    {
        Scene scene;
        scene.points[0].position = {0.0f, 0.0f, 10.0f};
        requireNoSpawn(scene, managerWith(1, 1), "closer than 18");
    }
    {
        Scene scene;
        scene.points[0].position = {0.0f, 0.0f, 60.0f};
        requireNoSpawn(scene, managerWith(1, 1), "farther than 57");
    }
    {
        Scene scene;
        scene.points[0].position = {25.0f, 0.0f, 25.0f};
        requireNoSpawn(scene, managerWith(1, 1), "facing below 0.8");
    }
    {
        Scene scene;
        scene.points[0].path = 0;
        requireNoSpawn(scene, managerWith(1, 1), "no path");
    }
    {
        Scene scene;
        scene.points[0].roomActive = false;
        requireNoSpawn(scene, managerWith(1, 1), "room not active");
    }
    {
        Scene scene;
        scene.blocked = true;
        requireNoSpawn(scene, managerWith(1, 1), "0x26cd38 blocks");
    }
}

void testSpacingOnTheSamePath() {
    Scene scene;
    PedestrianManager manager = managerWith(2, 2);
    Pedestrian& other = manager.pedestrians()[1];
    other.live = true;
    other.state = 1;
    other.path = 7;
    other.placement[3] = {1.0f, 5.0f, 37.9f};
    GameRandom random(4);
    GameRandom probe = random;
    std::uint32_t slot = probe.next() % 2;
    while (slot != 0) {
        random.next();
        probe = random;
        slot = probe.next() % 2;
    }
    require(!manager.update(scene.world, random).spawned, "within 8 xz of a pedestrian on the same path");
    other.path = 8;
    GameRandom again(4);
    probe = again;
    while (probe.next() % 2 != 0) {
        again.next();
        probe = again;
    }
    const PedestrianUpdate update = manager.update(scene.world, again);
    require(update.spawned && *update.spawned == 0, "another path does not count");
}

void testOneRemovalPerUpdate() {
    PedestrianManager manager = managerWith(4, 4);
    for (Pedestrian& pedestrian : manager.pedestrians()) pedestrian.live = true;
    manager.pedestrians()[0].state = 1;
    manager.pedestrians()[1].requested = 2;
    manager.pedestrians()[1].hold = 5;
    manager.pedestrians()[2].requested = 2;
    manager.pedestrians()[3].requested = 2;
    Scene scene;
    GameRandom random(1);
    PedestrianUpdate update = manager.update(scene.world, random);
    require(update.removed.empty(), "state 1 walks on, a held one stays, requested 2 with no other reason stays");
    scene.hidden = true;
    manager.setCountdown(0);
    update = manager.update(scene.world, random);
    require(update.removed.size() == 1 && update.removed[0].slot == 2 && update.removed[0].stopAnimations, "first hidden one");
    require(manager.countdown() == 7 * 3, "7 x (live - 1)");
    scene.hidden = false;
    scene.roomActive = false;
    update = manager.update(scene.world, random);
    require(update.removed.size() == 1 && update.removed[0].slot == 1, "room not active removes even a held one");
}

void testIdleStateIsRemoved() {
    PedestrianManager manager = managerWith(2, 1);
    manager.pedestrians()[1].live = true;
    Scene scene;
    GameRandom random(1);
    const PedestrianUpdate update = manager.update(scene.world, random);
    require(update.removed.size() == 1 && update.removed[0].slot == 1, "state 0 with nothing requested is removed");
}

void testPlacementRows() {
    const std::array<PedestrianVec3, 4> flat = pedestrianPlacement({1.0f, 2.0f, 3.0f}, {0.0f, 0.0f, 1.0f});
    require(near(flat[0][0], 1.0f) && near(flat[0][1], 0.0f) && near(flat[0][2], 0.0f), "row 0 = (z, 0, -x)");
    require(near(flat[1][0], 0.0f) && near(flat[1][1], 1.0f) && near(flat[1][2], 0.0f), "row 1 = row 2 x row 0");
    require(near(flat[3][1], 2.0f), "row 3 is the position");
    const std::array<PedestrianVec3, 4> up = pedestrianPlacement({0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
    require(near(up[0][2], 1.0f) && near(up[1][0], 1.0f), "|y| > 0.98: row 0 = (0, -z, y)");
}
}

int main() {
    try {
        testSlotNamesAndLevelLoad();
        testInitFlagsAndEnable();
        testWithoutSpawnDataNothingHappens();
        testDisabledRemovesEveryLivePedestrian();
        testLimitAboveSlotCountNeverSpawns();
        testCountdownStepsBeforeSpawning();
        testSpawnPlacesThePedestrian();
        testSpawnWindowOfEachTier();
        testSpawnRejections();
        testSpacingOnTheSamePath();
        testOneRemovalPerUpdate();
        testIdleStateIsRemoved();
        testPlacementRows();
    } catch (const std::exception& error) {
        std::cerr << "pedestrian-manager-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "pedestrian-manager-test ok\n";
    return 0;
}
