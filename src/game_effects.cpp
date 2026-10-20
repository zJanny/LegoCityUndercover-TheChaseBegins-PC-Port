#include "game_effects.hpp"
#include "runtime/vehicle_damage.hpp"

#include <cstdio>
#include <string>

namespace game_effects {
namespace assets = chase::assets;
namespace runtime = chase::runtime;

void LevelEffects::load(chase::render::Scene& scene, assets::FuseArchive& archive, const assets::ClassLookup& classOf, runtime::GameRandom& random,
                        runtime::DebrisPlaneRing& planes) {
    field_.emplace(random, planes);
    instances_.assign(runtime::kDebrisObjectCount, -1);
    const assets::FuseEntry* entry = archive.find(kDebrisLevel);
    if (!entry) return;
    const assets::Level level = assets::parseLevel(archive.read(*entry));
    const std::uint32_t meshHash = assets::nameHash("meshname");
    for (unsigned slot = 0; slot < runtime::kDebrisObjectCount; ++slot) {
        char name[16];
        std::snprintf(name, sizeof name, "Debris%02u", slot + 1);
        for (const assets::LevelObject& object : level.objects) {
            if (object.instance != name) continue;
            for (const assets::LevelParam& param : object.params) {
                if (param.hash != meshHash || param.type != 1 || param.blocks.empty()) continue;
                const std::string mesh(reinterpret_cast<const char*>(param.blocks[0].data()));
                const std::size_t instance = scene.instances();
                if (!scene.addModel(assets::meshModelPath(mesh))) continue;
                scene.setInstanceVisible(instance, false);
                instances_[slot] = std::ptrdiff_t(instance);
            }
        }
    }
    super_.load(scene, archive, level, classOf, *field_);
}

std::size_t LevelEffects::debrisInstances() const {
    std::size_t count = 0;
    for (const std::ptrdiff_t instance : instances_) count += instance >= 0;
    return count;
}

void LevelEffects::update(float frameDelta, const runtime::DebrisVec3& camera) {
    if (!field_) return;
    field_->update(frameDelta, camera);
    super_.update(frameDelta, *field_);
}

void LevelEffects::apply(chase::render::Scene& scene) const {
    if (!field_) return;
    for (unsigned slot = 0; slot < instances_.size(); ++slot) {
        if (instances_[slot] < 0) continue;
        const std::size_t instance = std::size_t(instances_[slot]);
        const runtime::DebrisObject& object = field_->objects()[slot];
        scene.setInstanceVisible(instance, object.enabled && object.alpha != 0);
        if (!object.enabled) continue;
        scene.setTransform(instance, object.matrix);
        scene.setInstanceTint(instance, {float(object.colour & 0xff) / 255.0f, float((object.colour >> 8) & 0xff) / 255.0f,
                                         float((object.colour >> 16) & 0xff) / 255.0f});
    }
    super_.apply(scene, *field_);
}

void LevelEffects::startSmoke(std::size_t vehicle, runtime::ParticleSystem& particles, unsigned definition, const runtime::ParticleVec3& position) {
    stopSmoke(vehicle, particles, runtime::kVehicleSmokeReplaceFade);
    const std::uint32_t handle = particles.start(definition, {position, std::nullopt, 0});
    if (handle == 0) return;
    smoke_[vehicle] = handle;
    particles.setCallback(handle, [this, vehicle](std::uint32_t removed, int message) {
        const auto found = smoke_.find(vehicle);
        if (message == 2 && found != smoke_.end() && found->second == removed) smoke_.erase(found);
    });
}

void LevelEffects::stopSmoke(std::size_t vehicle, runtime::ParticleSystem& particles, float fade) {
    const auto found = smoke_.find(vehicle);
    if (found == smoke_.end()) return;
    particles.stop(found->second, fade);
    smoke_.erase(found);
}

std::optional<std::uint32_t> LevelEffects::smoke(std::size_t vehicle) const {
    const auto found = smoke_.find(vehicle);
    if (found == smoke_.end()) return std::nullopt;
    return found->second;
}

}
