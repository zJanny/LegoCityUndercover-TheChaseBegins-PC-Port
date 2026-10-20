// The pedestrian system (src/runtime/pedestrian_system.cpp, docs/re/pedestrians.md). Provenance: llvm-objdump of
// artifacts/game/game.arm.elf. Step 0x2edc94: "PedestrianPath%03u" (0x358a3c) for 1 .. 128 in each scene of 0x589a18 with
// 0x22a3b0 (path section by name hash), count +0x250, no load without a path (0x2edd54); the pregen load 0x2ee868 returns 0 for a
// missing file. Spawn 0x2ee068: the node attached to room +0x14 (0x245008), byte +0x5d = 0; the manager update before the
// character updates of the entity pass. With <fib> <csv> it also spawns on lvl_01's shipped pregen file (pedestrian-system-real-data,
// 77 without the RomFS): the spawn points of the leaf at point 0, the view 20 units behind point 0 looking along its direction.
#include "assets/assets.hpp"
#include "assets/pregen_traffic.hpp"
#include "runtime/dcam.hpp"
#include "runtime/detective.hpp"
#include "runtime/game_random.hpp"
#include "runtime/pedestrian_system.hpp"
#include "runtime/room_streaming.hpp"

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

assets::LevelObject object(const std::string& className, const std::string& instance) {
    assets::LevelObject result;
    result.className = className;
    result.instance = instance;
    result.room = "Room1";
    return result;
}

assets::Level syntheticLevel(bool withPath) {
    assets::Level level;
    level.objects = {object(kPedestrianSystemClass, "Pedestrians")};
    for (const char* name : {"Pedestrian01", "Pedestrian03", "Pedestrian04", "Pedestrian05", "Pedestrian06"})
        level.objects.push_back(object(kPedestrianCharacterClass, name));
    if (withPath) {
        assets::LevelPath path;
        path.nameHash = assets::nameHash("PedestrianPath001");
        path.closed = true;
        path.points = {{0, 0, 0}, {0, 0, 10}, {0, 0, 20}, {0, 0, 30}};
        level.paths.push_back(path);
    }
    return level;
}

std::vector<std::uint8_t> onePointFile(const assets::LevelPath& path, float parameter) {
    float position[3];
    sampleDirectorPath(path, parameter, position);
    Writer w;
    w.u32(1);
    for (const float axis : position) w.f32(axis);
    w.u32(0);
    w.u32(path.nameHash);
    w.f32(parameter);
    w.u16(64);
    w.u16(1);
    w.f32(0);
    w.f32(0);
    w.u32(0);
    w.u32(0);
    w.u16(0);
    return w.bytes;
}

PedestrianSystemStep stepFacing(const PedestrianVec3& from, const PedestrianVec3& along, std::vector<bool>& enabled, std::set<int>& loadedRooms) {
    PedestrianSystemStep step;
    step.view.position = from;
    step.view.row2 = along;
    step.playerPosition = from;
    step.entityEnabled = [&enabled](std::size_t slot) { return bool(enabled[slot]); };
    step.room = [&loadedRooms](int room) { return std::optional<PedestrianRoom>(pedestrianRoomObject(room, loadedRooms.count(room) != 0)); };
    step.layers.fill(2);
    step.spawnBlocked = [](std::size_t, const PedestrianVec3&) { return false; };
    return step;
}

void applyUpdate(const PedestrianUpdate& update, std::vector<bool>& enabled) {
    if (update.spawned) enabled[*update.spawned] = true;
    for (const PedestrianRemoval& removal : update.removed) enabled[removal.slot] = false;
}

void synthetic() {
    require(pedestrianPathName(1) == "PedestrianPath001" && pedestrianPathName(128) == "PedestrianPath128", "path names");
    const assets::Level level = syntheticLevel(true);
    const assets::Level noPaths = syntheticLevel(false);
    require(pedestrianPathCount({&level}) == 1 && pedestrianPathCount({&noPaths}) == 0 && pedestrianPathCount({&level, &level}) == 1,
            "a path name counts once");
    require(levelHasPedestrianSystem(level), "pedestriansystem object");

    PedestrianSystem system;
    system.init();
    const auto slots = system.loadLevel(level, [](const std::string&) { return nullptr; }, [](const assets::LevelObject&) { return 0.5f; });
    require(slots.size() == 5 && slots[0].instance == "Pedestrian01" && slots[1].instance == "Pedestrian03" && slots[0].isMale, "slots");
    require(system.manager().limit() == 5 && system.room(0) == 1, "limit and the object's room");

    PedestrianSceneEnter enter;
    enter.loadedLevels = {&level};
    system.enterScene(enter);
    require(!system.manager().spawnDataLoaded(), "no pedestriansystem: bit 0 keeps its value");
    enter.pedestrianSystem = true;
    enter.loadedLevels = {&noPaths};
    system.enterScene(enter);
    require(!system.manager().spawnDataLoaded(), "no PedestrianPath: no load");
    enter.loadedLevels = {&level};
    system.enterScene(enter);
    require(!system.manager().spawnDataLoaded(), "a missing pregen file loads nothing");

    const std::vector<std::uint8_t> file = onePointFile(level.paths[0], 1.5f);
    StreamRooms rooms;
    rooms[1];
    std::map<int, RoomBox> boxes;
    boxes[1] = {{-50, -10, -50}, {50, 10, 50}};
    enter.pregenFile = &file;
    enter.rooms = {&rooms, &boxes, 1};
    system.enterScene(enter);
    require(system.manager().spawnDataLoaded() && system.spawnPointCount() == 1 && system.manager().countdown() == 0, "spawn data");

    std::vector<bool> enabled(slots.size(), false);
    std::set<int> loadedRooms{1};
    PedestrianSystemStep step = stepFacing({0, 0, -10}, {0, 0, 1}, enabled, loadedRooms);
    require(system.leafAt(0, 0, step.room).size() == 1, "root leaf");
    GameRandom random(0x1234u);
    const PedestrianUpdate spawned = system.update(step, random);
    require(spawned.spawned.has_value() && spawned.removed.empty(), "spawn in the first update");
    applyUpdate(spawned, enabled);
    const std::size_t slot = *spawned.spawned;
    const PedestrianBody& body = system.body(slot);
    float point[3];
    sampleDirectorPath(level.paths[0], 1.5f, point);
    require(body.requested == 1 && body.state == 0 && body.avoidDelay == 0 && body.pathParameter == 1.5f, "spawn fields");
    require(body.node[3][0] == point[0] && body.node[3][1] == point[1] && body.node[3][2] == point[2], "spawn position");
    require(system.room(slot) == 1, "the pedestrian hangs on the point's room");

    PedestrianCharacterStep characters;
    characters.dt = 1.0f / 30.0f;
    characters.playerPosition = {40, 0, 40};
    auto first = system.updateCharacters(characters, random);
    require(first.size() == 1 && first[0].slot == slot && first[0].walk.animations.size() == 1 &&
                first[0].walk.animations[0].clip == PedestrianClip::Walk && first[0].walk.animations[0].mode == 1,
            "entry 1 requests the walk clip in mode 1");
    require(body.state == 1 && body.requested == 2, "state 1 requests 2");
    applyUpdate(system.update(step, random), enabled);
    system.updateCharacters(characters, random);
    require(body.state == 2, "state 2 entered");
    const PedestrianVec3 before = body.node[3];
    applyUpdate(system.update(step, random), enabled);
    const auto walked = system.updateCharacters(characters, random);
    require(walked.size() == 1 && walked[0].walk.walked, "the walk runs in state 2");
    const float moved = std::hypot(body.node[3][0] - before[0], body.node[3][2] - before[2]);
    require(moved > 0.9f * body.speed * characters.dt && moved <= body.speed * characters.dt + 1e-4f, "moves up to speed x dt along row 2");
    require(system.manager().pedestrians()[slot].placement[3] == body.node[3], "the manager sees the walked position");

    loadedRooms.clear();
    const PedestrianUpdate removed = system.update(step, random);
    require(removed.removed.size() == 1 && removed.removed[0].slot == slot && removed.removed[0].stopAnimations,
            "an unloaded room removes the pedestrian and stops its animations");
    applyUpdate(removed, enabled);
    require(system.updateCharacters(characters, random).empty(), "no update of a removed pedestrian");

    loadedRooms.insert(1);
    PedestrianUpdate again;
    for (int i = 0; i < 20 && !again.spawned; ++i) applyUpdate(again = system.update(step, random), enabled);
    require(again.spawned.has_value(), "spawns again");
    system.setEnabled(false);
    const PedestrianUpdate disabled = system.update(step, random);
    require(disabled.removed.size() == 1 && !disabled.removed[0].stopAnimations, "a disabled manager removes without the stop");
}

int shipped(const char* fib, const char* csv) {
    if (!std::filesystem::exists(fib) || !std::filesystem::exists(csv)) {
        std::cout << "pedestrian-system-test: data not available, skipped\n";
        return 77;
    }
    assets::FuseArchive archive(fib);
    archive.loadNames(csv);
    const assets::FuseEntry* levelEntry = archive.find("levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl");
    const assets::FuseEntry* pregenEntry = archive.find(assets::pregenSpawnPath(assets::PregenSpawnKind::Pedestrians, "lvl_01_cherrytreehills"));
    require(levelEntry && pregenEntry, "lvl_01 files");
    const assets::Level level = assets::parseLevel(archive.read(*levelEntry));
    const std::vector<std::uint8_t> file = archive.read(*pregenEntry);
    require(levelHasPedestrianSystem(level) && pedestrianPathCount({&level}) > 0, "lvl_01 has the pedestriansystem and its paths");

    PedestrianSystem system;
    system.init();
    const auto slots = system.loadLevel(level, [](const std::string&) { return nullptr; }, [](const assets::LevelObject&) { return 0.5f; });
    require(slots.size() == 14, "lvl_01 slots");
    const std::map<int, RoomBox> boxes = roomBoxesOf(archive, "models/lvl_01_cherrytreehills");
    const StreamRooms rooms;
    PedestrianSceneEnter enter;
    enter.loadedLevels = {&level};
    enter.pedestrianSystem = true;
    enter.pregenFile = &file;
    enter.rooms = {&rooms, &boxes, 0};
    system.enterScene(enter);
    require(system.manager().spawnDataLoaded() && system.spawnPointCount() == 1115, "lvl_01 spawn points");

    const assets::PregenSpawns spawns = assets::parsePregenSpawns(file, assets::PregenSpawnKind::Pedestrians);
    const assets::PregenSpawnPoint& known = spawns.points[0];
    std::vector<bool> enabled(slots.size(), false);
    std::set<int> loadedRooms;
    for (const auto& [room, box] : boxes) loadedRooms.insert(room);
    float tangent[3] = {0, 0, 1};
    for (const assets::LevelPath& path : level.paths)
        if (path.nameHash == known.pathHash) detectivePathTangent(path, known.pathPosition, tangent);
    const PedestrianVec3 along{tangent[0], tangent[1], tangent[2]};
    const PedestrianVec3 from{known.position[0] - along[0] * 20.0f, known.position[1] - along[1] * 20.0f, known.position[2] - along[2] * 20.0f};
    PedestrianSystemStep step = stepFacing(from, along, enabled, loadedRooms);
    step.playerPosition = {known.position[0], known.position[1], known.position[2]};
    const std::vector<PedestrianSpawnPoint> leaf = system.leafAt(known.position[0], known.position[2], step.room);
    require(!leaf.empty(), "the leaf at point 0");
    GameRandom random(0x2f6b9d41u);
    PedestrianUpdate update;
    for (int i = 0; i < 30 && !update.spawned; ++i) applyUpdate(update = system.update(step, random), enabled);
    require(update.spawned.has_value(), "a pedestrian spawns in front of the view");
    const PedestrianBody& body = system.body(*update.spawned);
    bool onLeaf = false;
    for (const PedestrianSpawnPoint& point : leaf) onLeaf = onLeaf || point.position == body.node[3];
    require(onLeaf, "the spawn position is a point of the leaf");
    PedestrianCharacterStep characters;
    characters.dt = 1.0f / 30.0f;
    characters.playerPosition = step.view.position;
    for (int i = 0; i < 3; ++i) {
        applyUpdate(system.update(step, random), enabled);
        system.updateCharacters(characters, random);
    }
    require(body.state == 2, "walking after three updates");
    const PedestrianVec3 start = body.node[3];
    applyUpdate(system.update(step, random), enabled);
    system.updateCharacters(characters, random);
    const float moved = std::hypot(body.node[3][0] - start[0], body.node[3][2] - start[2]);
    require(moved > 0.0f && moved <= body.speed * characters.dt + 1e-4f, "walks along its path");
    std::cout << "pedestrian-system-test: lvl_01 slot " << *update.spawned << " spawned at " << body.node[3][0] << " " << body.node[3][2] << '\n';
    return 0;
}
}

int main(int argc, char** argv) {
    try {
        synthetic();
        if (argc == 3) return shipped(argv[1], argv[2]);
        if (argc != 1) throw std::runtime_error("usage: pedestrian-system-test [<fib> <csv>]");
    } catch (const std::exception& error) {
        std::cerr << "pedestrian-system-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "pedestrian-system-test ok\n";
    return 0;
}
