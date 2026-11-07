#include "runtime/traffic_system.hpp"

#include <cstdio>
#include <stdexcept>
#include <utility>

namespace chase::runtime {

namespace {

constexpr std::uint8_t kBoundNoNode = 0xff;

const assets::LevelObject* findObject(const assets::Level& level, const std::string& name) {
    const std::uint32_t hash = assets::nameHash(name);
    for (const assets::LevelObject& object : level.objects)
        if (assets::nameHash(object.instance) == hash) return &object;
    return nullptr;
}

const assets::LevelBound* findBound(const assets::Level& level, const std::string& name) {
    const std::uint32_t hash = assets::nameHash(name);
    for (const assets::LevelBound& bound : level.bounds)
        if (assets::nameHash(bound.name) == hash) return &bound;
    return nullptr;
}

TrafficFadeBound fadeBoundOf(const assets::Level& level, const assets::LevelBound& bound) {
    TrafficFadeBound result;
    result.centre = {bound.centre[0], bound.centre[1], bound.centre[2]};
    result.halfExtents = {bound.half[0], bound.half[1], bound.half[2]};
    if (bound.node != kBoundNoNode) {
        if (bound.node >= level.boundMatrices.size()) throw std::runtime_error("traffic fade bound " + bound.name + ": matrix index out of range");
        const std::array<float, 16>& m = level.boundMatrices[bound.node];
        result.rows = std::array<TrafficVec3, 3>{TrafficVec3{m[0], m[1], m[2]}, TrafficVec3{m[4], m[5], m[6]}, TrafficVec3{m[8], m[9], m[10]}};
    }
    return result;
}

}

std::string trafficCarName(std::size_t number) {
    char name[16];
    std::snprintf(name, sizeof name, "Traffic%02u", static_cast<unsigned>(number));
    return name;
}

std::string trafficFadeBoundName(std::size_t number) {
    char name[32];
    std::snprintf(name, sizeof name, "Traffic_FadeBound%02u", static_cast<unsigned>(number));
    return name;
}

PedestrianNode trafficCarNode() {
    PedestrianNode node;
    node.word = kTrafficCarNodeWord;
    node.fadeFlags = kTrafficCarNodeFadeFlags;
    return node;
}

const std::vector<TrafficCarSlot>& TrafficSystem::loadLevel(const assets::Level& level,
                                                            const std::function<float(const assets::LevelObject& object)>& heightOffset,
                                                            GameRandom& random) {
    slots_.clear();
    fadeBounds_.clear();
    for (std::size_t number = 1; number <= kTrafficCarNames; ++number) {
        const assets::LevelObject* object = findObject(level, trafficCarName(number));
        if (!object) continue;
        TrafficCarSlot slot;
        slot.instance = object->instance;
        slot.heightOffset = heightOffset(*object);
        slots_.push_back(std::move(slot));
    }
    const std::int32_t attribute =
        levelRootIntegerAttribute(level, "Traffic_MaxVehicles", static_cast<std::int32_t>(kTrafficDefaultMaxVehicles));
    manager_.loadLevel(slots_.size(), static_cast<std::uint32_t>(attribute), random);
    for (std::size_t number = 1; number <= kTrafficFadeBoundNames; ++number)
        if (const assets::LevelBound* bound = findBound(level, trafficFadeBoundName(number))) fadeBounds_.push_back(fadeBoundOf(level, *bound));
    carRooms_.assign(slots_.size(), 0);
    grace_.assign(slots_.size(), 0);
    return slots_;
}

std::size_t TrafficSystem::floorCellsFound() const {
    std::size_t found = 0;
    for (const SpawnPoint& point : points_) found += point.floorCell >= 0 ? 1 : 0;
    return found;
}

void TrafficSystem::syncCars(const TrafficSystemStep& step) {
    std::vector<TrafficCar>& cars = manager_.cars();
    const int lodClass = pedestrianFadeTier(trafficCarNode(), step.lod);
    if (lodClass < 0) throw std::runtime_error("traffic car without a LOD class");
    for (std::size_t i = 0; i < cars.size(); ++i) {
        TrafficCar& car = cars[i];
        car.disabled = !step.entityEnabled(i);
        car.pinned = false;
        car.lodClass = static_cast<unsigned>(lodClass);
        car.despawnRadius = step.despawnRadius;
        car.roomActive = carRooms_[i] != 0 && pedestrianRoomActive(step.room(carRooms_[i]), kTrafficCarLayerClass, step.layers);
    }
}

TrafficFrame TrafficSystem::makeFrame(const TrafficSystemStep& step) {
    TrafficFrame frame;
    frame.player = step.player;
    frame.camera = step.camera;
    for (std::size_t tier = 0; tier < frame.bands.size(); ++tier) frame.bands[tier] = {step.lod.fadeIn[tier], step.lod.fadeOut[tier]};
    frame.fadeBounds = fadeBounds_;
    cellPoints_.clear();
    if (!spawns_.cells.empty()) {
        const std::int32_t cell = assets::findPregenSpawnCell(spawns_, step.player[0], step.player[2]);
        if (cell >= 0)
            for (const std::uint16_t index : spawns_.cells[static_cast<std::size_t>(cell)].points) {
                const SpawnPoint& source = points_[index];
                TrafficSpawnPoint point;
                point.position = source.position;
                point.direction = source.direction;
                point.route = source.route;
                point.pathParameter = source.pathParameter;
                point.roomActive = source.room != 0 && pedestrianRoomActive(step.room(source.room), kTrafficCarLayerClass, step.layers);
                frame.cell.push_back(point);
                cellPoints_.push_back(index);
            }
    }
    const TrafficSpawnPoint* const cellBase = frame.cell.data();
    frame.place = [this, cellBase, &step](std::size_t car, const TrafficSpawnPoint& point) {
        const std::size_t cellIndex = static_cast<std::size_t>(&point - cellBase);
        const SpawnPoint& source = points_[cellPoints_[cellIndex]];
        TrafficCarPlacement placement;
        placement.car = car;
        placement.position = {source.position[0], source.position[1] + slots_[car].heightOffset, source.position[2]};
        placement.direction = source.direction;
        placement.pathParameter = source.pathParameter;
        placement.room = source.room;
        placement.floorCell = source.floorCell;
        placement.point = cellPoints_[cellIndex];
        placement.route = source.route.value_or(0);
        if (!step.place(placement)) return false;
        manager_.cars()[car].position = placement.position;
        carRooms_[car] = placement.room;
        return true;
    };
    return frame;
}

void TrafficSystem::advanceDrivers(GameRandom& random) {
    std::vector<TrafficCar>& cars = manager_.cars();
    for (std::size_t i = 0; i < cars.size(); ++i) {
        TrafficCar& car = cars[i];
        if (car.disabled) continue;
        if (car.driveState == 1 && car.driveStage == 2) {
            car.driveState = 2;
        } else if (car.driveState == 0 && car.driveStage == 1) {
            car.driveState = 1;
            car.driveStage = 2;
            grace_[i] = static_cast<std::uint8_t>(random.next() % kTrafficGraceRange + kTrafficGraceBase);
            car.driveValue = kTrafficStandInDriveValue;
            continue;
        }
        if (grace_[i] > 0) --grace_[i];
        car.keepWhenFar = grace_[i] > 0;
    }
}

std::vector<TrafficEvent> TrafficSystem::enterScene(const TrafficSceneEnter& enter, const TrafficSystemStep& step, GameRandom& random) {
    spawns_ = {};
    points_.clear();
    TrafficRouteTable& table = manager_.routeTable();
    std::vector<TrafficJunctionFill> fills = trafficJunctionFills(enter.loadedLevels);
    routes_ = setUpTrafficJunctionRoutes(table, trafficJunctionLayouts(fills));
    junctions_ = keepTrafficJunctions(std::move(fills), enter.loadedLevels, routes_.signals);
    if (routes_.junctionsInReach > 0 && enter.pregenFile) {
        spawns_ = assets::parsePregenSpawns(*enter.pregenFile, assets::PregenSpawnKind::Traffic);
        const std::vector<PregenDerivedFields> fields = deriveSpawnFields(spawns_, enter.loadedLevels, enter.rooms);
        const std::vector<int> floorCells = deriveTrafficFloorCells(spawns_, enter.floor);
        for (std::size_t i = 0; i < spawns_.points.size(); ++i) {
            SpawnPoint point;
            point.position = {spawns_.points[i].position[0], spawns_.points[i].position[1], spawns_.points[i].position[2]};
            point.direction = {fields[i].tangent[0], fields[i].tangent[1], fields[i].tangent[2]};
            point.pathParameter = spawns_.points[i].pathPosition;
            point.room = fields[i].room;
            point.floorCell = floorCells[i];
            if (fields[i].pathFound) point.route = trafficPointRoute(table, spawns_.points[i].pathHash);
            points_.push_back(point);
        }
        table.loaded = true;
    }
    applyPendingTrafficRoutes(table);
    syncCars(step);
    TrafficFrame frame = makeFrame(step);
    std::vector<TrafficEvent> events;
    manager_.startRoom(frame, random, &events);
    advanceDrivers(random);
    return events;
}

void TrafficSystem::leaveScene() {
    manager_.unloadRoutes();
    spawns_ = {};
    points_.clear();
}

std::vector<TrafficEvent> TrafficSystem::update(const TrafficSystemStep& step, GameRandom& random) {
    syncCars(step);
    TrafficFrame frame = makeFrame(step);
    std::vector<TrafficEvent> events;
    manager_.update(frame, random, &events);
    advanceDrivers(random);
    return events;
}

std::vector<TrafficJunctionLightEvent> TrafficSystem::updateJunctions(float dt) {
    junctions_.world.routes = manager_.routeTable();
    return runTrafficJunctionUpdates(junctions_.world, dt);
}

}
