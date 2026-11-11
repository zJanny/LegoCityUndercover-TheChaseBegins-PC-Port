#include "game_level.hpp"
#include "runtime/node_view_cull.hpp"

#include <algorithm>
#include <cmath>

namespace game_level {

namespace {

chase::runtime::ViewRows rowsOf(const chase::assets::Matrix& matrix) {
    chase::runtime::ViewRows rows{};
    for (std::size_t row = 0; row < 4; ++row)
        for (std::size_t axis = 0; axis < 3; ++axis) rows[row][axis] = matrix[row * 4 + axis];
    return rows;
}

std::array<float, 3> worldOf(const chase::assets::Matrix& matrix, const std::array<float, 3>& local) {
    std::array<float, 3> out{};
    for (std::size_t axis = 0; axis < 3; ++axis)
        out[axis] = local[0] * matrix[axis] + local[1] * matrix[4 + axis] + local[2] * matrix[8 + axis] + matrix[12 + axis];
    return out;
}

}

void LevelSession::setupTrafficCarLights() {
    trafficCarLights.load(traffic.carCount());
    trafficGlowLocal.assign(traffic.carCount(), {});
    trafficCullBounds.assign(traffic.carCount(), {});
    for (std::size_t car = 0; car < traffic.carCount(); ++car) {
        if (!trafficBoarding[car]) continue;
        const std::size_t placedIndex = vehicleDrives[*trafficBoarding[car]].placedIndex;
        const std::string& path = scene.instanceModel(placedSceneIndex[placedIndex]);
        const chase::assets::FuseEntry* entry = shared.archive.find(path);
        if (!entry) continue;
        const chase::assets::Model model = chase::assets::parseModel(shared.archive.read(*entry));
        const chase::runtime::VehicleRecord& record = levelScripts.vehicles().records()[boardingRecords[*trafficBoarding[car]]];
        chase::runtime::TrafficBrakeLightClass values;
        values.models = record.brakeLightModels;
        values.locators = record.brakeLightLocators;
        values.particle = record.brakeLightParticle;
        const chase::runtime::TrafficBrakeLights lights = chase::runtime::makeTrafficBrakeLights(values, model.nodeNames);
        trafficCarLights.setup(car, lights);
        for (std::size_t slot = 0; slot < lights.glowParts.size(); ++slot) {
            const std::int32_t part = lights.glowParts[slot];
            if (part < 0 || std::size_t(part) >= model.nodeMatrices.size()) continue;
            const auto& matrix = model.nodeMatrices[std::size_t(part)];
            trafficGlowLocal[car][slot] = {matrix[12], matrix[13], matrix[14]};
        }
        float centre[3] = {0, 0, 0}, half[3] = {0, 0, 0};
        if (chase::assets::modelBounds(shared.archive, path, centre, half))
            trafficCullBounds[car] = {{centre[0], centre[1], centre[2]}, std::sqrt(half[0] * half[0] + half[1] * half[1] + half[2] * half[2])};
    }
}

void LevelSession::applyTrafficCarLights() {
    trafficLightsDisabled.resize(trafficBoarding.size(), false);
    for (std::size_t car = 0; car < trafficBoarding.size(); ++car) {
        if (!trafficBoarding[car]) continue;
        const bool disabled = objectState.isDisabled(boardingVehicles[*trafficBoarding[car]].instance);
        if (disabled && !trafficLightsDisabled[car]) trafficCarLights.disable(car);
        trafficLightsDisabled[car] = disabled;
    }
    chase::runtime::TrafficCarLights::Changes changes = trafficCarLights.takeChanges();
    for (const chase::runtime::TrafficCarPartChange& change : changes.parts) {
        if (!trafficBoarding[change.car]) continue;
        const std::size_t instance = placedSceneIndex[vehicleDrives[*trafficBoarding[change.car]].placedIndex];
        scene.setInstanceNodeShown(instance, change.part.part, change.part.shown, change.part.alpha);
    }
    chase::runtime::ParticleSystem& system = particles.system();
    for (const chase::runtime::TrafficCarGlowRequest& request : changes.glows) {
        if (!request.start) {
            system.stop(request.handle, 0.0f);
            continue;
        }
        if (!trafficBoarding[request.car]) continue;
        const chase::assets::Matrix& matrix = boardingVehicles[*trafficBoarding[request.car]].matrix;
        const std::array<float, 3> position = worldOf(matrix, trafficGlowLocal[request.car][request.slot]);
        chase::runtime::ParticleStart start;
        start.position = {position[0], position[1], position[2]};
        const std::uint32_t handle = system.start(request.definition, start);
        trafficCarLights.glowStarted(request.car, request.slot, handle);
        if (handle != 0)
            system.setCallback(handle, [this, car = request.car](std::uint32_t ended, int message) { trafficCarLights.glowEvent(car, ended, message); });
    }
    for (std::size_t car = 0; car < trafficBoarding.size(); ++car) {
        if (!trafficBoarding[car]) continue;
        const chase::runtime::TrafficBrakeLights& lights = trafficCarLights.lights(car);
        for (std::size_t slot = 0; slot < lights.glows.size(); ++slot) {
            if (lights.glows[slot] == 0) continue;
            const std::array<float, 3> position = worldOf(boardingVehicles[*trafficBoarding[car]].matrix, trafficGlowLocal[car][slot]);
            system.setPosition(lights.glows[slot], {position[0], position[1], position[2]});
        }
    }
}

void LevelSession::cullTrafficCars(const chase::render::Mat4& view, float fieldOfView, float aspect, float nearZ, float farZ) {
    const chase::render::Mat4 world = chase::render::rigidInverse(view);
    chase::runtime::ViewCamera camera;
    camera.frame = {{{-view[0], -view[4], -view[8]}, {view[1], view[5], view[9]}, {-view[2], -view[6], -view[10]}, {world[12], world[13], world[14]}}};
    chase::runtime::viewScales(fieldOfView, aspect, camera.scaleX, camera.scaleY);
    camera.nearZ = nearZ;
    camera.farZ = farZ;
    const chase::runtime::ViewPlanes planes = chase::runtime::viewPlanes(camera);
    for (std::size_t car = 0; car < trafficBoarding.size(); ++car) {
        if (!trafficBoarding[car] || objectState.isDisabled(boardingVehicles[*trafficBoarding[car]].instance)) continue;
        chase::runtime::ViewNode node;
        node.matrix = rowsOf(boardingVehicles[*trafficBoarding[car]].matrix);
        node.centre = trafficCullBounds[car].centre;
        node.radius = trafficCullBounds[car].radius;
        node.fade = std::uint8_t(traffic.fade(car));
        trafficCarLights.setNodeCulled(car, chase::runtime::viewNodeCulled(camera, planes, node));
    }
}

}
