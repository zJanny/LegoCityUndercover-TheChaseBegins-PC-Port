#include "game_traffic.hpp"
#include "game_level.hpp"
#include "runtime/pedestrian_system.hpp"

#include "runtime/collision.hpp"
#include "runtime/traffic_ground_world.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>
#include <utility>

namespace game_traffic {

namespace {

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

}

void LevelTraffic::load(const assets::Level& level, const std::function<float(const assets::LevelObject& object)>& heightOffset,
                        assets::PlacedObjectState& state) {
    for (const runtime::TrafficCarSlot& slot : system_.loadLevel(level, heightOffset, random_)) state.disabled.insert(lower(slot.instance));
}

runtime::TrafficSystemStep LevelTraffic::makeStep(const SystemInputs& inputs, assets::PlacedObjectState& state) {
    static const runtime::RoomLayerTable startup = runtime::startupRoomLayers();
    runtime::TrafficSystemStep step;
    step.player = inputs.player;
    step.camera = inputs.camera;
    step.entityEnabled = [this, &state](std::size_t car) { return !state.isDisabled(system_.slots()[car].instance); };
    step.room = [&inputs](int room) { return std::optional<runtime::PedestrianRoom>(runtime::pedestrianRoomObject(room, inputs.roomLoaded(room))); };
    step.layers = startup;
    step.place = [this, &inputs, &state](const runtime::TrafficCarPlacement& placement) {
        const std::string& instance = system_.slots()[placement.car].instance;
        if (!inputs.place(instance, placement)) return false;
        state.disabled.erase(lower(instance));
        return true;
    };
    return step;
}

void LevelTraffic::applyPathRequests(const SystemInputs& inputs) {
    for (const runtime::TrafficPathRequest& request : inputs.pathRequests) {
        system_.enablePath(request.path, request.enable);
        std::cout << "traffic: path " << request.path << (request.enable ? " enabled" : " disabled") << " at update " << inputs.update << '\n';
    }
}

void LevelTraffic::report(const std::vector<runtime::TrafficEvent>& events, std::uint32_t update, assets::PlacedObjectState& state) {
    for (const runtime::TrafficEvent& event : events) {
        const std::string& name = system_.slots()[event.car].instance;
        switch (event.kind) {
        case runtime::TrafficEventKind::Spawn: {
            const runtime::TrafficVec3 position = system_.carPosition(event.car);
            std::cout << "traffic car " << name << ": spawned at point " << system_.pointOfCell(event.point) << " (" << position[0] << ' ' << position[1]
                      << ' ' << position[2] << "), fade " << int(event.fade) << " at update " << update << '\n';
            break;
        }
        case runtime::TrafficEventKind::Despawn:
            state.disabled.insert(lower(name));
            std::cout << "traffic car " << name << ": removed at update " << update << '\n';
            break;
        case runtime::TrafficEventKind::Fade: std::cout << "traffic car " << name << ": fade " << int(event.fade) << " at update " << update << '\n'; break;
        }
    }
}

void LevelTraffic::enterScene(assets::FuseArchive& archive, const assets::Level& level, const std::string& levelName, const runtime::StreamRooms* rooms,
                              const std::map<int, runtime::RoomBox>* boxes, int currentRoom, const runtime::FloorMesh* floor, const SystemInputs& inputs,
                              assets::PlacedObjectState& state) {
    applyPathRequests(inputs);
    runtime::TrafficSceneEnter enter;
    enter.loadedLevels = {&level};
    if (const assets::FuseEntry* entry = archive.find(assets::pregenSpawnPath(assets::PregenSpawnKind::Traffic, levelName))) {
        pregenFile_ = archive.read(*entry);
        enter.pregenFile = &pregenFile_;
    }
    enter.rooms = {rooms, boxes, currentRoom};
    enter.floor = floor;
    const std::vector<runtime::TrafficEvent> events = system_.enterScene(enter, makeStep(inputs, state), random_);
    std::cout << "traffic: " << system_.slots().size() << " cars, limit " << system_.manager().maxVehicles() << ", " << system_.spawnPointCount()
              << " spawn points, " << system_.routeCount() << " routes, " << system_.floorCellsFound() << " floor cells, spawn data "
              << (system_.manager().routesLoaded() ? "loaded" : "not loaded") << '\n';
    report(events, inputs.update, state);
}

void LevelTraffic::updateSystem(const SystemInputs& inputs, assets::PlacedObjectState& state) {
    if (inputs.enabled) system_.setEnabled(*inputs.enabled);
    applyPathRequests(inputs);
    if (!inputs.maxVehicles.empty()) {
        for (const float value : inputs.maxVehicles) system_.setMaxVehicles({value});
        std::cout << "traffic: limit " << system_.manager().maxVehicles() << " at update " << inputs.update << '\n';
    }
    report(system_.update(makeStep(inputs, state), random_), inputs.update, state);
}

std::vector<chase::runtime::TrafficJunctionLightEvent> LevelTraffic::updateJunctions(float dt, std::uint32_t update) {
    static const char* const phases[] = {"red", "yellow", "green"};
    std::vector<runtime::TrafficJunctionLightEvent> events = system_.updateJunctions(dt);
    for (const runtime::TrafficJunctionLightEvent& event : events) {
        if (event.change.phase == event.change.previous) continue;
        std::cout << "traffic junction " << event.junction << ": pair " << event.change.pair << ' '
                  << phases[event.change.phase] << " at update " << update << '\n';
    }
    return events;
}

std::vector<runtime::TrafficCarMove> LevelTraffic::driveCars(const runtime::TrafficDriveStep& step, std::uint32_t update, assets::PlacedObjectState& state) {
    runtime::TrafficDriveResult result = system_.driveCars(step, random_);
    for (const runtime::TrafficDriveEvent& event : result.events) {
        std::cout << "traffic car " << system_.slots()[event.car].instance << ": ";
        switch (event.kind) {
        case runtime::TrafficDriveEventKind::Stop: std::cout << "stopped at junction " << event.signal - 1; break;
        case runtime::TrafficDriveEventKind::Go: std::cout << "goes on at junction " << event.signal - 1; break;
        case runtime::TrafficDriveEventKind::Junction: std::cout << "lane of junction " << event.signal - 1; break;
        case runtime::TrafficDriveEventKind::Road: std::cout << "left the junction lane"; break;
        }
        std::cout << " at update " << update << '\n';
    }
    for (const runtime::TrafficCarStateEvent& entry : result.states) {
        std::cout << "traffic car " << system_.slots()[entry.car].instance << ": ";
        switch (entry.event.kind) {
        case runtime::TrafficStateEventKind::Placed: std::cout << "placed by its driver"; break;
        case runtime::TrafficStateEventKind::Stopping: std::cout << "driver state 6"; break;
        case runtime::TrafficStateEventKind::Destroy: std::cout << "destroyed by its driver"; break;
        case runtime::TrafficStateEventKind::Throw: std::cout << "throw (no throw objects)"; break;
        }
        std::cout << " at update " << update << '\n';
    }
    report(result.traffic, update, state);
    return std::move(result.moves);
}

std::optional<std::size_t> LevelTraffic::carOf(const std::string& lowerInstance) const {
    for (std::size_t i = 0; i < system_.slots().size(); ++i)
        if (lower(system_.slots()[i].instance) == lowerInstance) return i;
    return std::nullopt;
}

}

namespace game_level {

void LevelSession::loadTraffic() {
    traffic.load(
        level,
        [this](const chase::assets::LevelObject& object) {
            std::string name = object.instance;
            std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return char(std::tolower(c)); });
            const chase::runtime::VehicleRecord* record = levelScripts.vehicles().find(name);
            return record ? record->trafficHeightOffRoad - record->centreOfMass[1] : 0.0f;
        },
        objectState);
    trafficBoarding.assign(traffic.carCount(), std::nullopt);
    trafficFadeShown.assign(traffic.carCount(), 255);
    for (std::size_t car = 0; car < traffic.carCount(); ++car) {
        std::string name = traffic.instance(car);
        std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return char(std::tolower(c)); });
        for (std::size_t i = 0; i < boardingVehicles.size(); ++i)
            if (boardingVehicles[i].instance == name) trafficBoarding[car] = i;
    }
    loadTrafficBodies();
    loadTrafficContact();
    setupTrafficCarLights();
}

game_traffic::SystemInputs LevelSession::trafficInputs() {
    game_traffic::SystemInputs inputs;
    inputs.enabled = levelScripts.progressNatives().trafficAndPedestriansEnabled();
    inputs.maxVehicles = levelScripts.progressNatives().takeTrafficMaxVehicles();
    inputs.pathRequests = levelScripts.progressNatives().takeTrafficPathRequests();
    inputs.player = {player->position()[0], player->position()[1], player->position()[2]};
    inputs.camera = {follow->eye()[0], follow->eye()[1], follow->eye()[2]};
    inputs.roomLoaded = [this](int room) { return roomShown(room); };
    inputs.update = updatesForLog;
    inputs.place = [this](const std::string&, const chase::runtime::TrafficCarPlacement& placement) { return placeTrafficCar(placement); };
    return inputs;
}

void LevelSession::enterTrafficScene() {
    if (!player || !follow) return;
    const chase::assets::CollisionItem* floor = floorItem();
    std::optional<chase::runtime::FloorMesh> mesh;
    if (floor) mesh.emplace(*floor);
    traffic.enterScene(shared.archive, level, levelName, roomStreamer ? &roomStreamer->rooms() : nullptr, roomStreamer ? &roomStreamer->boxes() : nullptr,
                       roomStreamer ? roomStreamer->currentRoom() : 0, mesh ? &*mesh : nullptr, trafficInputs(), objectState);
}

void LevelSession::updateTrafficSystem() {
    if (!player || !follow) return;
    traffic.updateSystem(trafficInputs(), objectState);
}

void LevelSession::updateTrafficJunctions(float dt) {
    if (!player || !follow) return;
    for (const chase::runtime::TrafficJunctionLightEvent& event : traffic.updateJunctions(dt, updatesForLog))
        trafficLights.applyChange(event.junction, event.change);
}

void LevelSession::driveTrafficCars(float dt) {
    if (!player || !follow) return;
    const auto rows = [](const chase::render::Mat4& matrix) {
        chase::runtime::AiMatrix node{};
        for (std::size_t row = 0; row < 4; ++row)
            for (std::size_t axis = 0; axis < 3; ++axis) node[row][axis] = matrix[row * 4 + axis];
        return node;
    };
    chase::runtime::TrafficDriveStep step;
    step.dt = dt;
    step.clock = chase::runtime::gameClockAfter(updatesForLog);
    step.lights = &trafficCarLights;
    std::vector<std::optional<std::size_t>> carOfVehicle(boardingVehicles.size());
    for (std::size_t car = 0; car < trafficBoarding.size(); ++car)
        if (trafficBoarding[car]) carOfVehicle[*trafficBoarding[car]] = car;
    for (std::size_t i = 0; i < boardingVehicles.size(); ++i) {
        const chase::runtime::BoardingVehicle& vehicle = boardingVehicles[i];
        if (objectState.isDisabled(vehicle.instance)) continue;
        step.obstacles.push_back({{rows(vehicle.matrix), {vehicle.boxHalf[0], vehicle.boxHalf[1], vehicle.boxHalf[2]}}, carOfVehicle[i]});
    }
    const float heading = chase::runtime::headingRadians(player->motion.facing);
    const chase::runtime::PlayerBox box;
    step.obstacles.push_back({{chase::runtime::pedestrianPlacement({player->position()[0], player->position()[1], player->position()[2]}, {std::sin(heading), 0.0f, std::cos(heading)}),
                               {box.half[0], box.half[1], box.half[2]}},
                              std::nullopt});
    step.ground = chase::runtime::trafficGroundWorld(collision, [](std::uint32_t) {});
    step.chaseMarkers = [this](std::size_t car, bool on) {
        if (const std::optional<std::size_t> index = trafficBoarding[car]) levelScripts.setChaseMarkers(boardingVehicles[*index].instance, on);
    };
    step.body = [this, &rows](std::size_t car) -> std::optional<chase::runtime::TrafficCarBody> {
        const std::optional<std::size_t> index = trafficBoarding[car];
        if (!index) return std::nullopt;
        const chase::runtime::BoardingVehicle& vehicle = boardingVehicles[*index];
        chase::runtime::TrafficCarBody body;
        body.node = rows(vehicle.matrix);
        body.centre = {vehicle.boxCentre[0], vehicle.boxCentre[1], vehicle.boxCentre[2]};
        body.halfExtents = {vehicle.boxHalf[0], vehicle.boxHalf[1], vehicle.boxHalf[2]};
        body.radius = std::sqrt(body.halfExtents[0] * body.halfExtents[0] + body.halfExtents[1] * body.halfExtents[1] + body.halfExtents[2] * body.halfExtents[2]);
        body.gearSpeed = levelScripts.vehicles().records()[boardingRecords[*index]].topSpeed;
        readTrafficBody(car, body);
        return body;
    };
    step.bodies = trafficBodies.hooks(trafficBodyWorld);
    step.now = trafficClock();
    feedTrafficCharacters(step);
    syncTrafficDriverFields(true);
    const std::vector<chase::runtime::TrafficCarMove> moves = traffic.driveCars(step, updatesForLog, objectState);
    syncTrafficDriverFields(false);
    reportTrafficPushes();
    for (const chase::runtime::TrafficCarMove& move : moves) {
        const std::optional<std::size_t> index = trafficBoarding[move.car];
        if (!index) continue;
        chase::render::Mat4 matrix{};
        for (std::size_t row = 0; row < 4; ++row)
            for (std::size_t axis = 0; axis < 3; ++axis) matrix[row * 4 + axis] = move.node[row][axis];
        matrix[15] = 1.0f;
        placeVehicleAt(*index, matrix);
    }
    applyTrafficCarLights();
}

bool LevelSession::placeTrafficCar(const chase::runtime::TrafficCarPlacement& placement) {
    const std::optional<std::size_t> index = trafficBoarding[placement.car];
    if (!index) return false;
    const std::array<std::array<float, 3>, 4> rows = chase::runtime::pedestrianPlacement(placement.position, placement.direction);
    chase::render::Mat4 matrix{};
    for (std::size_t row = 0; row < 4; ++row)
        for (std::size_t axis = 0; axis < 3; ++axis) matrix[row * 4 + axis] = rows[row][axis];
    matrix[15] = 1.0f;
    placeVehicleAt(*index, matrix);
    trafficBodies.spawn(placement.car, placement.floorCell, rows, trafficBodyFloor());
    VehicleDrive& drive = vehicleDrives[*index];
    drive.floorTracker = {};
    drive.history = chase::runtime::VehicleMatrixHistory(matrix);
    chase::runtime::VehicleRecord* record = levelScripts.vehicles().mutableRecord(boardingRecords[*index]);
    record->state = record->requestedState = 1;
    record->statePending = false;
    return true;
}

void LevelSession::applyTrafficFades() {
    for (std::size_t car = 0; car < trafficBoarding.size(); ++car) {
        if (!trafficBoarding[car]) continue;
        const int fade = traffic.fade(car);
        if (fade == trafficFadeShown[car]) continue;
        trafficFadeShown[car] = fade;
        const std::size_t placedIndex = vehicleDrives[*trafficBoarding[car]].placedIndex;
        const float multiplier = float(fade) / 255.0f;
        scene.setInstanceFade(placedSceneIndex[placedIndex], multiplier);
        for (const auto& [owner, wheelInstance] : wheelSceneIndex)
            if (owner == placedIndex) scene.setInstanceFade(wheelInstance, multiplier);
    }
}

}
