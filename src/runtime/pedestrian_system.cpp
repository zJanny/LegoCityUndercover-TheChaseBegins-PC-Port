#include "runtime/pedestrian_system.hpp"
#include "assets/path_name_hash.hpp"

#include "runtime/level_start.hpp"
#include "runtime/scene_start.hpp"

#include <cstdio>
#include <map>
#include <utility>

namespace chase::runtime {

namespace {

constexpr std::uint32_t kRoomNodeShift = 8;
constexpr std::uint32_t kRoomAttachField = 0x14;
constexpr std::uint32_t kRoomRemovalField = 0x7c;

const assets::LevelObject* findObject(const assets::Level& level, const std::string& name) {
    const std::uint32_t hash = assets::nameHash(name);
    for (const assets::LevelObject& object : level.objects)
        if (assets::nameHash(object.instance) == hash) return &object;
    return nullptr;
}

bool isMaleOf(const assets::LevelObject& object, const assets::ClassLookup& classOf) {
    const assets::Level* cls = classOf(object.className);
    if (!cls) return true;
    const assets::LevelParam* param = effectiveParam(object, *cls, assets::nameHash("IsMale"));
    return !param || param->value != 0;
}

int roomOfAttachNode(std::uint32_t node) { return static_cast<int>(node >> kRoomNodeShift); }

}

std::string pedestrianPathName(std::size_t number) {
    char name[24];
    std::snprintf(name, sizeof name, "PedestrianPath%03u", static_cast<unsigned>(number));
    return name;
}

std::size_t pedestrianPathCount(const std::vector<const assets::Level*>& loadedLevels) {
    std::size_t count = 0;
    for (std::size_t number = 1; number <= kPedestrianPathNames; ++number) {
        const std::uint32_t hash = assets::pathNameHash(pedestrianPathName(number));
        for (const assets::Level* level : loadedLevels) {
            bool found = false;
            for (const assets::LevelPath& path : level->paths) found = found || path.nameHash == hash;
            if (found) {
                ++count;
                break;
            }
        }
    }
    return count;
}

bool levelHasPedestrianSystem(const assets::Level& level) {
    for (const assets::LevelObject& object : level.objects)
        if (object.className == kPedestrianSystemClass) return true;
    return false;
}

PedestrianRoom pedestrianRoomObject(int room, bool loaded) {
    PedestrianRoom object;
    object.state = loaded ? kPedestrianRoomLoadedState : 0;
    object.resource = loaded;
    object.node = (static_cast<std::uint32_t>(room) << kRoomNodeShift) | kRoomRemovalField;
    object.attachNode = pedestrianRoomAttachNode(room);
    return object;
}

std::uint32_t pedestrianRoomAttachNode(int room) { return (static_cast<std::uint32_t>(room) << kRoomNodeShift) | kRoomAttachField; }

const std::vector<PedestrianSlotInfo>& PedestrianSystem::loadLevel(const assets::Level& level, const assets::ClassLookup& classOf,
                                                                   const std::function<float(const assets::LevelObject& object)>& arrivalRadius) {
    const std::vector<std::size_t> numbers =
        manager_.loadLevel([&](const std::string& name) { return findObject(level, name) != nullptr; }, level);
    slots_.clear();
    bodies_.clear();
    rooms_.clear();
    for (const std::size_t number : numbers) {
        const assets::LevelObject& object = *findObject(level, pedestrianSlotName(number));
        PedestrianSlotInfo slot;
        slot.instance = object.instance;
        slot.isMale = isMaleOf(object, classOf);
        slot.arrivalRadius = arrivalRadius(object);
        std::array<PedestrianVec3, 4> node{PedestrianVec3{1, 0, 0}, PedestrianVec3{0, 1, 0}, PedestrianVec3{0, 0, 1}, PedestrianVec3{}};
        if (const std::optional<assets::Placement> start = assets::objectStart(level, object.instance))
            node[3] = {start->position[0], start->position[1], start->position[2]};
        bodies_.push_back(pedestrianCreate(slot.isMale, node));
        rooms_.push_back(roomNumber(object.room));
        slots_.push_back(std::move(slot));
    }
    return slots_;
}

void PedestrianSystem::enterScene(const PedestrianSceneEnter& enter) {
    std::optional<bool> loaded;
    if (enter.pedestrianSystem && pedestrianPathCount(enter.loadedLevels) != 0) {
        spawns_ = {};
        points_.clear();
        paths_.clear();
        if (enter.pregenFile) {
            spawns_ = assets::parsePregenSpawns(*enter.pregenFile, assets::PregenSpawnKind::Pedestrians);
            const std::vector<PregenDerivedFields> fields = deriveSpawnFields(spawns_, enter.loadedLevels, enter.rooms);
            std::map<std::pair<std::size_t, std::size_t>, std::uint32_t> pathIds;
            for (std::size_t i = 0; i < spawns_.points.size(); ++i) {
                SpawnPoint point;
                point.position = spawns_.points[i].position;
                point.direction = fields[i].tangent;
                point.pathParameter = spawns_.points[i].pathPosition;
                point.room = fields[i].room;
                if (fields[i].pathFound) {
                    const std::pair<std::size_t, std::size_t> key{fields[i].levelIndex, fields[i].pathIndex};
                    auto id = pathIds.find(key);
                    if (id == pathIds.end()) {
                        paths_.push_back(pedestrianLevelPath(enter.loadedLevels[key.first]->paths[key.second]));
                        id = pathIds.emplace(key, static_cast<std::uint32_t>(paths_.size())).first;
                    }
                    point.path = id->second;
                }
                points_.push_back(point);
            }
        }
        loaded = enter.pregenFile != nullptr;
    }
    manager_.enterScene(loaded);
}

void PedestrianSystem::leaveScene() {
    manager_.leaveScene();
    spawns_ = {};
    points_.clear();
}

std::vector<PedestrianSpawnPoint> PedestrianSystem::leafAt(float x, float z,
                                                           const std::function<std::optional<PedestrianRoom>(int room)>& room) const {
    std::vector<PedestrianSpawnPoint> leaf;
    if (spawns_.cells.empty()) return leaf;
    const std::int32_t cell = assets::findPregenSpawnCell(spawns_, x, z);
    if (cell < 0) return leaf;
    for (const std::uint16_t index : spawns_.cells[static_cast<std::size_t>(cell)].points) {
        const SpawnPoint& source = points_[index];
        PedestrianSpawnPoint point;
        point.position = source.position;
        point.direction = source.direction;
        point.path = source.path;
        point.pathParameter = source.pathParameter;
        if (source.room != 0) point.room = room(source.room);
        leaf.push_back(point);
    }
    return leaf;
}

PedestrianUpdate PedestrianSystem::update(const PedestrianSystemStep& step, GameRandom& random) {
    std::vector<Pedestrian>& pedestrians = manager_.pedestrians();
    for (std::size_t slot = 0; slot < pedestrians.size(); ++slot) {
        Pedestrian& pedestrian = pedestrians[slot];
        const PedestrianBody& body = bodies_[slot];
        pedestrian.live = step.entityEnabled(slot);
        pedestrian.animationPlayer = true;
        pedestrian.state = body.state;
        pedestrian.requested = body.requested;
        pedestrian.hold = body.hold;
        pedestrian.placement = body.node;
        pedestrian.room = rooms_[slot] != 0 ? step.room(rooms_[slot]) : std::nullopt;
        pedestrian.node.parent = rooms_[slot] != 0 ? pedestrianRoomAttachNode(rooms_[slot]) : 0;
    }
    const std::vector<PedestrianSpawnPoint> leaf = leafAt(step.playerPosition[0], step.playerPosition[2], step.room);
    PedestrianWorld world;
    world.view = step.view;
    world.spawnPoints = leaf.empty() ? nullptr : &leaf;
    world.fade = step.fade;
    world.layers = step.layers;
    world.meshNodeType = step.meshNodeType;
    world.spawnBlocked = step.spawnBlocked;
    const PedestrianUpdate result = manager_.update(world, random);
    if (result.spawned) {
        const std::size_t slot = *result.spawned;
        const Pedestrian& pedestrian = pedestrians[slot];
        PedestrianBody& body = bodies_[slot];
        body.requested = pedestrian.requested;
        body.state = pedestrian.state;
        body.forward = pedestrian.forward;
        body.hold = pedestrian.hold;
        body.speed = pedestrian.speed;
        body.lateral = pedestrian.lateral;
        body.avoidDelay = 0;
        body.pathParameter = pedestrian.pathParameter;
        body.node = pedestrian.placement;
        rooms_[slot] = roomOfAttachNode(pedestrian.node.parent);
    }
    return result;
}

std::vector<PedestrianCharacterUpdate> PedestrianSystem::updateCharacters(const PedestrianCharacterStep& step, GameRandom& random) {
    std::vector<PedestrianCharacterUpdate> updates;
    std::vector<Pedestrian>& pedestrians = manager_.pedestrians();
    for (std::size_t slot = 0; slot < pedestrians.size(); ++slot) {
        Pedestrian& pedestrian = pedestrians[slot];
        if (!pedestrian.live) continue;
        PedestrianBody& body = bodies_[slot];
        PedestrianWalkWorld world;
        world.dt = step.dt;
        world.path = pedestrian.path != 0 && pedestrian.path <= paths_.size() ? &paths_[pedestrian.path - 1] : nullptr;
        world.arrivalRadius = slots_[slot].arrivalRadius;
        world.playerPosition = step.playerPosition;
        world.playerInVehicle = step.playerInVehicle;
        world.playerState = step.playerState;
        world.otherPedestrians = manager_.otherLivePositions(slot);
        world.groundProbeTaken = manager_.walkToken();
        world.visible = step.visible && step.visible(slot);
        world.hidden = step.hidden && step.hidden(slot);
        if (step.drawTest) world.drawTest = step.drawTest(slot);
        if (step.blocked)
            world.blocked = [&step, slot](PedestrianBody& walker, const PedestrianVec3& position, PedestrianVec3& velocity, float dt) {
                return step.blocked(slot, walker, position, velocity, dt);
            };
        if (step.groundProbe) world.groundProbe = [&step, slot](PedestrianVec3& position) { step.groundProbe(slot, position); };
        world.groundRay = step.groundRay;
        if (step.clipFinished) world.clipFinished = [&step, slot](PedestrianClip clip) { return step.clipFinished(slot, clip); };
        PedestrianCharacterUpdate update;
        update.slot = slot;
        update.walk = PedestrianWalk::update(body, world, random);
        pedestrian.state = body.state;
        pedestrian.requested = body.requested;
        pedestrian.hold = body.hold;
        pedestrian.placement = body.node;
        pedestrian.forward = body.forward;
        pedestrian.pathParameter = body.pathParameter;
        updates.push_back(std::move(update));
    }
    return updates;
}

}
