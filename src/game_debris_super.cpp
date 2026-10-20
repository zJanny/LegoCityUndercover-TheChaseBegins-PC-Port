#include "game_debris_super.hpp"
#include "game_level.hpp"

#include <cstdio>
#include <iostream>
#include <string>

namespace game_debris_super {
namespace assets = chase::assets;
namespace runtime = chase::runtime;

void SuperDebrisObjects::load(chase::render::Scene& scene, assets::FuseArchive& archive, const assets::Level& level, const assets::ClassLookup& classOf,
                              runtime::DebrisField& field) {
    instances_ = filled();
    tint_ = runtime::SuperBrickTint{};
    clock_ = 0;
    const assets::DrawableObjects drawable = assets::drawableObjects(archive, level, classOf);
    std::array<std::optional<std::uint16_t>, runtime::kSuperDebrisObjectCount> values{};
    for (unsigned i = 0; i < runtime::kSuperDebrisObjectCount; ++i) {
        char name[16];
        std::snprintf(name, sizeof name, "SBDebris%02u", i + 1);
        for (const assets::SuperBrickObject& brick : drawable.superBricks) {
            if (brick.instance != name) continue;
            values[i] = brick.value;
            const std::size_t instance = scene.instances();
            if (!scene.addModel(assets::meshModelPath(drawable.objects[brick.placedIndex].mesh))) break;
            scene.setInstanceVisible(instance, false);
            instances_[i] = std::ptrdiff_t(instance);
            break;
        }
    }
    field.loadSuperObjects(values);
}

void SuperDebrisObjects::update(float frameDelta, runtime::DebrisField& field) {
    clock_ += frameDelta;
    const std::array<std::uint8_t, 3>& rainbow = tint_.bytes();
    field.animateSuper(clock_, std::uint32_t(rainbow[0]) | std::uint32_t(rainbow[1]) << 8 | std::uint32_t(rainbow[2]) << 16 | 0xff000000u);
    tint_.update(frameDelta);
}

void SuperDebrisObjects::apply(chase::render::Scene& scene, const runtime::DebrisField& field) const {
    for (unsigned i = 0; i < instances_.size(); ++i) {
        if (instances_[i] < 0) continue;
        const std::size_t instance = std::size_t(instances_[i]);
        const runtime::DebrisObject& object = field.superObjects()[i];
        scene.setInstanceVisible(instance, object.enabled && object.alpha != 0);
        if (!object.enabled) continue;
        scene.setTransform(instance, object.matrix);
        scene.setInstanceTint(instance, {float(object.colour & 0xff) / 255.0f, float((object.colour >> 8) & 0xff) / 255.0f,
                                         float((object.colour >> 16) & 0xff) / 255.0f});
    }
}

std::size_t SuperDebrisObjects::instances() const {
    std::size_t count = 0;
    for (const std::ptrdiff_t instance : instances_) count += instance >= 0;
    return count;
}

std::optional<runtime::DebrisSuperBlock> prepareSuperBlock(runtime::DebrisField& field, const std::optional<runtime::DebrisSuperBlock>& block,
                                                           const std::array<float, 16>& node, bool enabled) {
    if (!block || !enabled) return std::nullopt;
    runtime::DebrisSuperBlock out = *block;
    out.node = node;
    field.listFreeSuperObjects();
    return out;
}

}

namespace game_level {
namespace runtime = chase::runtime;

std::optional<runtime::DebrisSuperBlock> LevelSession::destroyedSuperBlock(const std::optional<runtime::DebrisSuperBlock>& block, const std::array<float, 16>& node) {
    runtime::DebrisField* field = effects.debris();
    if (!field) return std::nullopt;
    return game_debris_super::prepareSuperBlock(*field, block, node, levelScripts.progressNatives().superBricksEnabled().value_or(true));
}

std::optional<runtime::DebrisSuperBlock> LevelSession::contactSuperBlock(std::size_t vehicleIndex, int value) {
    runtime::DebrisField* field = effects.debris();
    if (!field) return std::nullopt;
    runtime::DebrisSuperBlock block;
    block.value = std::uint8_t(value);
    block.count = std::uint8_t(runtime::kDebrisDefaultCount);
    return game_debris_super::prepareSuperBlock(*field, block, boardingVehicles[vehicleIndex].matrix, true);
}

}
