#include "game_level.hpp"

#include "runtime/entity_update_list.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>

namespace game_level {
namespace runtime = chase::runtime;

void LevelSession::loadTrafficBodies() {
    std::vector<runtime::TrafficCarBodies::Car> cars(traffic.carCount());
    std::vector<runtime::AiMatrix> nodes(traffic.carCount());
    trafficBodyWorld.node = [this](std::size_t car) {
        runtime::AiMatrix node{};
        const std::optional<std::size_t> index = trafficBoarding[car];
        if (!index) return node;
        const chase::render::Mat4& matrix = boardingVehicles[*index].matrix;
        for (std::size_t row = 0; row < 4; ++row)
            for (std::size_t axis = 0; axis < 3; ++axis) node[row][axis] = matrix[row * 4 + axis];
        return node;
    };
    for (std::size_t car = 0; car < cars.size(); ++car) {
        nodes[car] = trafficBodyWorld.node(car);
        const std::optional<std::size_t> index = trafficBoarding[car];
        if (!index) continue;
        const runtime::VehicleRecord& record = levelScripts.vehicles().records()[boardingRecords[*index]];
        cars[car].centreOfMass = record.centreOfMass;
        for (const runtime::VehicleWheel& wheel : record.wheels) {
            runtime::TrafficBodyWheel spun;
            spun.radius = wheel.suspension.radius;
            spun.mountX = wheel.suspension.mount[0];
            spun.mountY = wheel.suspension.mount[1];
            cars[car].wheels.push_back(spun);
        }
    }
    trafficBodies.load(cars, nodes, runtime::kVehicleBodyGravity);
}

const runtime::FloorMesh* LevelSession::trafficBodyFloor() {
    const chase::assets::CollisionItem* floor = floorItem();
    if (!floor) {
        trafficFloorMesh.reset();
        return nullptr;
    }
    trafficFloorMesh.emplace(*floor);
    return &*trafficFloorMesh;
}

void LevelSession::stepTrafficBodies(int worldSteps) {
    if (trafficBodies.size() == 0) return;
    static const runtime::RoomLayerTable layers = runtime::startupRoomLayers();
    trafficBodyWorld.floor = trafficBodyFloor();
    const auto disabled = [this](std::size_t car) {
        const std::optional<std::size_t> index = trafficBoarding[car];
        return !index || objectState.isDisabled(boardingVehicles[*index].instance);
    };
    for (std::size_t car = 0; car < trafficBodies.size(); ++car) {
        if (!trafficBoarding[car]) continue;
        runtime::UpdateListEntity entity;
        entity.disabled = disabled(car);
        // STAND-IN TRAFFIC-S29: layer class 0 and room type 0 for every car, and the room test run every update instead of at the
        // entity's enable and disable.
        const bool listed = runtime::entityBelongsOnUpdateList(entity, std::uint8_t(0), layers);
        runtime::TrafficBodyState& body = trafficBodies.body(car);
        if (listed == ((body.world & runtime::kTrafficBodyWanted) != 0)) continue;
        runtime::setTrafficBodyInWorld(body, listed);
        std::cout << "traffic car " << traffic.instance(car) << ": body " << (listed ? "joins" : "leaves") << " the physics world at update " << updatesForLog << '\n';
    }
    trafficBodies.beforeWorldStep(trafficBodyWorld, disabled);
    for (int step = 0; step < worldSteps; ++step)
        for (std::size_t car = 0; car < trafficBodies.size(); ++car) {
            runtime::TrafficBodyState& body = trafficBodies.body(car);
            if ((body.rigidFlags & runtime::kRigidFlagOutOfWorld) != 0) continue;
            runtime::stepTrafficBody(body, runtime::kVehicleWorldStep, trafficBodyWorld.floor);
        }
}

}
