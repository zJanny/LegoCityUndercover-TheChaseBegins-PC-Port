// The node fade of a spawned pedestrian (src/runtime/node_fade.cpp, pedestrian_fade.cpp, docs/re/pedestrians-fade.md). Provenance of the
// pinned values: llvm-objdump of artifacts/game/game.arm.elf. 0x275434 stores +0xdd = 1 and +0xdc = 0xff on a node of type *0x37674c
// and on its children. 0x10eb70 (words 0x52a1dc / 0x52a1e8 + 4 tier = 36 / 72 / 108 and 38 / 76 / 114, radius words 3 / 6 / 12): a
// step below 0 turns to 16 when (fade-in distance)^2 >= distance^2 - radius^2, a step of 0 or more turns to -16 (240) when
// (fade-out distance)^2 <= that; the alpha adds the signed step and clamps to 0 .. 255, or jumps to 255 / 0 when byte 0x52a1cc is
// set, and byte 0x52a1cd forces the step to 16. *0x37674c = 2: the table 0x5303c4 gets slot 0 from 0x10332c (call 0x102174), then
// 0x105b78 (call 0x102180) takes 1 and 0x103290 (call 0x10218c) takes 2. Expected numbers are worked by hand from these rules.
#include "assets/assets.hpp"
#include "assets/pregen_traffic.hpp"
#include "runtime/detective.hpp"
#include "runtime/pedestrian_fade.hpp"
#include "runtime/pedestrian_system.hpp"
#include "runtime/room_streaming.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace chase::runtime;
namespace assets = chase::assets;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

NodeFadeFrame frameAt(float distance, float radius, int tier) {
    NodeFadeFrame frame;
    frame.tier = tier;
    frame.radius = radius;
    frame.distanceSquared = (distance + radius) * (distance + radius);
    return frame;
}

void startValues() {
    NodeFade fade;
    require(fade.alpha == 255 && fade.step == 0, "a created node is opaque (0x10d2d4 stores 255)");
    nodeFadeStart(fade);
    require(fade.alpha == 1 && fade.step == -1, "0x275434: alpha 1, step 0xff");
}

void fadesInWithinTheFadeInDistance() {
    const DistanceFadeTable table;
    NodeFade fade;
    nodeFadeStart(fade);
    nodeFadeStep(fade, frameAt(30.0f, 1.0f, 0), table);
    require(fade.step == 16 && fade.alpha == 17, "inside 36: the step turns to +16 and the alpha moves in the same update");
    for (int i = 0; i < 14; ++i) nodeFadeStep(fade, frameAt(30.0f, 1.0f, 0), table);
    require(fade.alpha == 241, "1 + 15 x 16");
    nodeFadeStep(fade, frameAt(30.0f, 1.0f, 0), table);
    require(fade.alpha == 255, "clamped to 255");
    nodeFadeStep(fade, frameAt(30.0f, 1.0f, 0), table);
    require(fade.alpha == 255 && fade.step == 16, "stays opaque");
}

void fadesOutBeyondTheFadeInDistance() {
    const DistanceFadeTable table;
    NodeFade fade;
    nodeFadeStart(fade);
    nodeFadeStep(fade, frameAt(50.0f, 1.0f, 0), table);
    require(fade.step == -1 && fade.alpha == 0, "beyond 36 the step stays -1 and the alpha 1 falls to 0");
    nodeFadeStep(fade, frameAt(50.0f, 1.0f, 0), table);
    require(fade.alpha == 0, "stays at 0");
    nodeFadeStep(fade, frameAt(20.0f, 1.0f, 0), table);
    require(fade.step == 16 && fade.alpha == 16, "walking inside 36 fades it in again");
}

void fadesOutAtTheFadeOutDistance() {
    const DistanceFadeTable table;
    NodeFade fade;
    nodeFadeStep(fade, frameAt(37.0f, 1.0f, 0), table);
    require(fade.step == 0 && fade.alpha == 255, "below 38 an opaque node stays");
    nodeFadeStep(fade, frameAt(38.0f, 1.0f, 0), table);
    require(fade.step == -16 && fade.alpha == 239, "at 38 it turns to -16 and falls");
    for (int i = 0; i < 14; ++i) nodeFadeStep(fade, frameAt(38.5f, 1.0f, 0), table);
    require(fade.alpha == 15, "239 - 14 x 16");
    nodeFadeStep(fade, frameAt(38.5f, 1.0f, 0), table);
    require(fade.alpha == 0, "clamped to 0");
}

void tiersAndGates() {
    const DistanceFadeTable table;
    NodeFade fade;
    nodeFadeStep(fade, frameAt(70.0f, 1.0f, 1), table);
    require(fade.step == 0 && fade.alpha == 255, "tier 1 fades out from 76");
    nodeFadeStep(fade, frameAt(76.0f, 1.0f, 1), table);
    require(fade.step == -16, "tier 1 at 76");
    NodeFade untouched;
    untouched.alpha = 100;
    untouched.step = -3;
    nodeFadeStep(untouched, frameAt(500.0f, 1.0f, -1), table);
    require(untouched.alpha == 100 && untouched.step == -3, "tier -1: no change");
    NodeFade forced;
    forced.alpha = 100;
    NodeFadeFrame frame = frameAt(500.0f, 1.0f, 2);
    frame.forceFadeIn = true;
    nodeFadeStep(forced, frame, table);
    require(forced.step == 16 && forced.alpha == 116, "0x52a1cd forces +16");
    frame.forceFadeIn = false;
    frame.jumpToTarget = true;
    forced.step = -1;
    nodeFadeStep(forced, frame, table);
    require(forced.step == -1 && forced.alpha == 0, "0x52a1cc: step below 0 jumps to 0");
    frame = frameAt(1.0f, 1.0f, 2);
    frame.jumpToTarget = true;
    nodeFadeStep(forced, frame, table);
    require(forced.step == 16 && forced.alpha == 255, "0x52a1cc: step of 0 or more jumps to 255");
    require(std::fabs(nodeFadeOpacity(forced) - 1.0f) < 1e-6f, "opacity");
}

void pedestrianNodes() {
    const DistanceFadeTable table;
    const PedestrianNode node = pedestrianMeshNode(1.5f);
    require((node.word & 31u) == kMeshNodeType && kMeshNodeType == 2, "*0x37674c");
    require(pedestrianFadeTier(node, table) == 0, "a minifig radius below 3 is tier 0");
    require(pedestrianFadeTier(pedestrianMeshNode(5.0f), table) == 1 && pedestrianFadeTier(pedestrianMeshNode(12.0f), table) == -1,
            "radius tiers 6 and 12");
    require(!pedestrianNodeHidden(node), "not hidden");
    const PedestrianSpawnWindow window = pedestrianSpawnWindow(table, pedestrianFadeTier(node, table));
    require(window.minimum == 18.0f && window.maximum == 57.0f, "tier 0 spawn window 36 x 0.5 .. 38 x 1.5");

    PedestrianNodeBounds bounds;
    bounds.centre = {0.0f, 1.0f, 0.0f};
    bounds.radius = 1.5f;
    const std::array<PedestrianVec3, 4> rows{PedestrianVec3{0, 0, 1}, PedestrianVec3{0, 1, 0}, PedestrianVec3{-1, 0, 0}, PedestrianVec3{10, 0, 20}};
    const PedestrianVec3 centre = pedestrianNodeCentre(bounds, rows);
    require(centre[0] == 10.0f && centre[1] == 1.0f && centre[2] == 20.0f, "centre through the rows");

    PedestrianFades fades;
    fades.load(2);
    fades.setBounds(1, bounds);
    require(fades.fade(1).alpha == 255, "created opaque");
    fades.start(1);
    PedestrianFadeInputs inputs;
    inputs.fadePoint = {10.0f, 1.0f, 50.0f};
    fades.step(1, node, rows, inputs);
    require(fades.fade(1).step == 16 && fades.fade(1).alpha == 17, "30 m from the fade point: 900 is inside 36^2 = 1296");
    inputs.fadePoint = {10.0f, 1.0f, 70.0f};
    fades.start(1);
    fades.step(1, node, rows, inputs);
    require(fades.fade(1).step == -1 && fades.fade(1).alpha == 0, "50 m: 2500 is beyond 36^2, the alpha 1 falls to 0");
}

void drawTestFollowsTheFadeInDistance() {
    const DistanceFadeTable table;
    const PedestrianNode node = pedestrianMeshNode(1.0f);
    const PedestrianDrawTest near = pedestrianDrawTest(node, {0, 0, 0}, table);
    require(pedestrianNodeDrawn(near.node, near.meshNodeType, near.fade, 35.0f * 35.0f), "inside 36 is drawn");
    require(!pedestrianNodeDrawn(near.node, near.meshNodeType, near.fade, 40.0f * 40.0f), "beyond 36 is not drawn");
}


int shipped(const char* fib, const char* csv) {
    if (!std::filesystem::exists(fib) || !std::filesystem::exists(csv)) {
        std::cout << "pedestrian-fade-test: data not available, skipped\n";
        return 77;
    }
    assets::FuseArchive archive(fib);
    archive.loadNames(csv);
    const assets::FuseEntry* levelEntry = archive.find("levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl");
    const assets::FuseEntry* pregenEntry = archive.find(assets::pregenSpawnPath(assets::PregenSpawnKind::Pedestrians, "lvl_01_cherrytreehills"));
    require(levelEntry && pregenEntry, "lvl_01 files");
    const assets::Level level = assets::parseLevel(archive.read(*levelEntry));
    const std::vector<std::uint8_t> file = archive.read(*pregenEntry);
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
    for (Pedestrian& pedestrian : system.manager().pedestrians()) pedestrian.node = pedestrianMeshNode(1.0f);
    PedestrianFades fades;
    fades.load(slots.size());

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
    PedestrianSystemStep step;
    step.view.position = from;
    step.view.row2 = along;
    step.playerPosition = {known.position[0], known.position[1], known.position[2]};
    step.entityEnabled = [&enabled](std::size_t slot) { return bool(enabled[slot]); };
    step.room = [&loadedRooms](int room) { return std::optional<PedestrianRoom>(pedestrianRoomObject(room, loadedRooms.count(room) != 0)); };
    step.layers.fill(2);
    step.spawnBlocked = [](std::size_t, const PedestrianVec3&) { return false; };
    step.meshNodeType = kMeshNodeType;
    GameRandom random(0x2f6b9d41u);
    PedestrianUpdate update;
    for (int i = 0; i < 30 && !update.spawned; ++i) {
        update = system.update(step, random);
        if (update.spawned) enabled[*update.spawned] = true;
    }
    require(update.spawned.has_value(), "a pedestrian spawns in lvl_01 in front of the view");
    const std::size_t slot = *update.spawned;
    require(enabled[slot], "the caller enables the entity after the spawn");
    require(system.manager().pedestrians()[slot].fadeStarted, "the spawned node has the mesh node type: the fade starts");
    fades.start(slot);
    const PedestrianVec3 position = system.body(slot).node[3];
    const float distance = std::sqrt(pedestrianDistanceSquared(position, from));
    require(distance >= 18.0f && distance <= 57.0f, "the spawn lies in the tier 0 window 18 .. 57");
    const PedestrianVec3 toPoint{(position[0] - from[0]) / distance, (position[1] - from[1]) / distance, (position[2] - from[2]) / distance};
    require(toPoint[0] * along[0] + toPoint[1] * along[1] + toPoint[2] * along[2] >= 0.8f, "the spawn lies in the view cone");
    PedestrianFadeInputs inputs;
    inputs.fadePoint = from;
    for (int i = 0; i < 20; ++i) fades.step(slot, system.manager().pedestrians()[slot].node, system.body(slot).node, inputs);
    const float beyond = pedestrianDistanceSquared(position, from) - 1.0f;
    const bool inside = beyond <= 36.0f * 36.0f;
    require(inside ? fades.fade(slot).alpha == 255 : fades.fade(slot).alpha == 0,
            "a spawn inside 36 m fades in to 255, one beyond fades out to 0 (the original parks it and removes it)");
    std::cout << "pedestrian-fade-test: lvl_01 slot " << slot << " at " << distance << " m, alpha " << int(fades.fade(slot).alpha) << '\n';
    return 0;
}

}

int main(int argc, char** argv) {
    try {
        startValues();
        fadesInWithinTheFadeInDistance();
        fadesOutBeyondTheFadeInDistance();
        fadesOutAtTheFadeOutDistance();
        tiersAndGates();
        pedestrianNodes();
        drawTestFollowsTheFadeInDistance();
        if (argc == 3) return shipped(argv[1], argv[2]);
        if (argc != 1) throw std::runtime_error("usage: pedestrian-fade-test [<fib> <csv>]");
    } catch (const std::exception& error) {
        std::cerr << "pedestrian-fade-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "pedestrian-fade-test ok\n";
    return 0;
}
