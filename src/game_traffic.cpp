#include "game_traffic.hpp"
#include "game_level.hpp"
#include "runtime/pedestrian_system.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>

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

void LevelTraffic::report(const std::vector<runtime::TrafficEvent>& events, const SystemInputs& inputs, assets::PlacedObjectState& state) {
    for (const runtime::TrafficEvent& event : events) {
        const std::string& name = system_.slots()[event.car].instance;
        switch (event.kind) {
        case runtime::TrafficEventKind::Spawn: {
            const runtime::TrafficVec3 position = system_.carPosition(event.car);
            std::cout << "traffic car " << name << ": spawned at point " << system_.pointOfCell(event.point) << " (" << position[0] << ' ' << position[1]
                      << ' ' << position[2] << "), fade " << int(event.fade) << " at update " << inputs.update << '\n';
            break;
        }
        case runtime::TrafficEventKind::Despawn:
            state.disabled.insert(lower(name));
            std::cout << "traffic car " << name << ": removed at update " << inputs.update << '\n';
            break;
        case runtime::TrafficEventKind::Fade: std::cout << "traffic car " << name << ": fade " << int(event.fade) << " at update " << inputs.update << '\n'; break;
        }
    }
}

void LevelTraffic::enterScene(assets::FuseArchive& archive, const assets::Level& level, const std::string& levelName, const runtime::StreamRooms* rooms,
                              const std::map<int, runtime::RoomBox>* boxes, int currentRoom, const runtime::FloorMesh* floor, const SystemInputs& inputs,
                              assets::PlacedObjectState& state) {
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
    report(events, inputs, state);
}

void LevelTraffic::updateSystem(const SystemInputs& inputs, assets::PlacedObjectState& state) {
    if (inputs.enabled) system_.setEnabled(*inputs.enabled);
    if (!inputs.maxVehicles.empty()) {
        for (const float value : inputs.maxVehicles) system_.setMaxVehicles({value});
        std::cout << "traffic: limit " << system_.manager().maxVehicles() << " at update " << inputs.update << '\n';
    }
    report(system_.update(makeStep(inputs, state), random_), inputs, state);
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
}

game_traffic::SystemInputs LevelSession::trafficInputs() {
    game_traffic::SystemInputs inputs;
    inputs.enabled = levelScripts.progressNatives().trafficAndPedestriansEnabled();
    inputs.maxVehicles = levelScripts.progressNatives().takeTrafficMaxVehicles();
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

bool LevelSession::placeTrafficCar(const chase::runtime::TrafficCarPlacement& placement) {
    const std::optional<std::size_t> index = trafficBoarding[placement.car];
    if (!index) return false;
    const std::array<std::array<float, 3>, 4> rows = chase::runtime::pedestrianPlacement(placement.position, placement.direction);
    chase::render::Mat4 matrix{};
    for (std::size_t row = 0; row < 4; ++row)
        for (std::size_t axis = 0; axis < 3; ++axis) matrix[row * 4 + axis] = rows[row][axis];
    matrix[15] = 1.0f;
    placeVehicleAt(*index, matrix);
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
