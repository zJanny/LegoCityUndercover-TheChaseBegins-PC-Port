#include "game_level.hpp"

namespace game_level {
namespace runtime = chase::runtime;

void LevelSession::loadScriptDestroyProps(chase::assets::FuseArchive& archive) {
    propSmasher.loadScriptDestroyed(level, classOf, placedObjects, [&archive](const chase::assets::Placement& placement) -> std::optional<runtime::SmashBox> {
        float centre[3], half[3];
        if (!chase::assets::modelBounds(archive, chase::assets::meshModelPath(placement.mesh), centre, half)) return std::nullopt;
        return runtime::SmashBox{{centre[0], centre[1], centre[2]}, {half[0], half[1], half[2]}};
    });
}

void LevelSession::propDestroyMessage(const std::string& lowerInstance) {
    const auto data = propSmasher.destroyData(lowerInstance);
    if (!data) return;
    propDestroyedEffects(*data);
    const auto kind = levelScripts.objectNatives().kindOf(lowerInstance);
    const bool soundPlayedByTheHit = !kind || runtime::destroyEffectSet(*kind) == runtime::DestroyEffectSet::BaseProp;
    if (!soundPlayedByTheHit) playPropSound(data->effects.sound, runtime::propEffectCentre(data->prop.entityBox.centre, data->prop.node));
}

}
