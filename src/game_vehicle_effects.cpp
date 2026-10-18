#include "game_level.hpp"
#include "runtime/vehicle_wheels.hpp"

#include <algorithm>
#include <iostream>

namespace game_level {
namespace runtime = chase::runtime;

runtime::DebrisLineTest LevelSession::debrisLineStandIn() const {
    return [this](const runtime::DebrisVec3& from, const runtime::DebrisVec3& to) -> std::optional<runtime::DebrisLineHit> {
        const std::optional<runtime::WheelHit> hit = runtime::castWheelRay(collision, from, to, [this](const runtime::CollisionWorld::Entry& entry) {
            return std::any_of(boardingVehicles.begin(), boardingVehicles.end(), [&](const runtime::BoardingVehicle& vehicle) { return vehicle.instance == entry.owner; });
        });
        if (!hit) return std::nullopt;
        return runtime::DebrisLineHit{hit->point, hit->normal};
    };
}

void LevelSession::vehicleBrickBurst(std::size_t vehicleIndex, const runtime::Vector3& point) {
    runtime::DebrisField* debris = effects.debris();
    if (!debris) return;
    const runtime::VehicleRecord& record = levelScripts.vehicles().records()[boardingRecords[vehicleIndex]];
    runtime::DebrisBurst burst;
    burst.position = point;
    burst.colour = record.debrisColour;
    burst.secondColour = record.debrisColour2;
    debris->burst(burst, debrisLineStandIn(), debrisLineStandIn());
}

void LevelSession::vehicleBreakBurst(std::size_t vehicleIndex) {
    runtime::DebrisField* debris = effects.debris();
    if (!debris) return;
    const runtime::VehicleRecord& record = levelScripts.vehicles().records()[boardingRecords[vehicleIndex]];
    const runtime::BoardingVehicle& vehicle = boardingVehicles[vehicleIndex];
    runtime::PropDestroyEffects parameters;
    parameters.debrisAmount = record.debrisAmount;
    parameters.colour = record.debrisColour;
    parameters.secondColour = record.debrisColour2;
    parameters.ratio = record.debrisRatio;
    const runtime::DebrisVec3 bottom = runtime::propDebrisPosition({vehicle.boxCentre[0], vehicle.boxCentre[1], vehicle.boxCentre[2]},
                                                                   {vehicle.boxHalf[0], vehicle.boxHalf[1], vehicle.boxHalf[2]}, vehicle.matrix);
    debris->burst(runtime::propDebrisBurst(parameters, bottom), debrisLineStandIn(), debrisLineStandIn());
}

void LevelSession::vehicleDamageEffects(std::size_t vehicleIndex, const runtime::DamageOutcome& outcome) {
    const runtime::BoardingVehicle& vehicle = boardingVehicles[vehicleIndex];
    if (outcome.smokeEffect != 0) {
        const runtime::ParticleVec3 position =
            runtime::vehicleSmokePosition({vehicle.boxCentre[0], vehicle.boxCentre[1], vehicle.boxCentre[2]}, vehicle.boxHalf[2], vehicle.matrix);
        effects.startSmoke(vehicleIndex, particles.system(), unsigned(outcome.smokeEffect), position);
    }
    if (outcome.destroyed) effects.stopSmoke(vehicleIndex, particles.system(), runtime::kVehicleSmokeDestroyFade);
}

void LevelSession::stepVehicleFlashes() {
    for (std::size_t i = 0; i < vehicleDrives.size() && i < boardingRecords.size(); ++i) {
        runtime::VehicleRecord& record = *levelScripts.vehicles().mutableRecord(boardingRecords[i]);
        const runtime::VehicleFlashStep step = runtime::stepVehicleFlash(record);
        const std::size_t placedIndex = vehicleDrives[i].placedIndex;
        std::vector<std::size_t> instances = {placedSceneIndex[placedIndex]};
        for (const auto& [placed, instance] : wheelSceneIndex)
            if (placed == placedIndex) instances.push_back(instance);
        for (const std::size_t instance : instances) {
            if (step.colour)
                scene.setInstanceTint(instance, {float(*step.colour & 0xff) / 255.0f, float((*step.colour >> 8) & 0xff) / 255.0f,
                                                 float((*step.colour >> 16) & 0xff) / 255.0f});
            scene.setInstanceWhiteMaterials(instance, record.flashWhite);
        }
    }
}

void LevelSession::propDestroyedEffects(const runtime::SmashedProp& prop) {
    const runtime::PropDestroyEffects& e = prop.effects;
    const runtime::DebrisVec3 centre = runtime::propEffectCentre(prop.prop.entityBox.centre, prop.prop.node);
    if (e.explosionParticle > 0 && std::size_t(e.explosionParticle) < particles.system().defs().defs.size())
        particles.system().start(unsigned(e.explosionParticle), {centre, std::nullopt, 0});
    if (e.spawnDebris && effects.debris())
        effects.debris()->burst(runtime::propDebrisBurst(e, runtime::propDebrisPosition(prop.prop.entityBox.centre, prop.prop.entityBox.half, prop.prop.node)),
                                debrisLineStandIn(), debrisLineStandIn());
    std::cout << "prop " << prop.instance << ": explosion particle " << e.explosionParticle << ", " << (e.spawnDebris ? e.debrisAmount : 0u)
              << " bricks (stand-in VEH-S102: no screen shake, flash light, sound, TriggerObject or Respawn)\n";
}

}
