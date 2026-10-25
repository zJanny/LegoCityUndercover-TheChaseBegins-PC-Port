#include "game_level.hpp"
#include <iostream>

namespace game_level {
namespace runtime = chase::runtime;

void LevelSession::updatePropRocking() {
    for (const runtime::PropRocking& prop : levelScripts.propRocking().props()) {
        const auto logged = propRockingLogged.find(prop.instance);
        if (logged == propRockingLogged.end() || logged->second != prop.rocking) {
            if (logged != propRockingLogged.end()) std::cout << "prop " << prop.instance << ": rocking " << (prop.rocking ? "on" : "off") << " at update " << updates << '\n';
            propRockingLogged[prop.instance] = prop.rocking;
        }
        if (!prop.rocking || objectState.isDisabled(prop.instance)) continue;
        auto scene = propRockingScene.find(prop.instance);
        if (scene == propRockingScene.end()) {
            std::optional<std::size_t> found;
            for (std::size_t i = 0; i < placedObjects.size() && !found; ++i)
                if (game_doors::lowerCase(placedObjects[i].instance) == prop.instance) found = placedSceneIndex[i];
            scene = propRockingScene.emplace(prop.instance, found).first;
        }
        if (scene->second) this->scene.setTransform(*scene->second, runtime::rockingMatrix(prop, updates * runtime::kRockingClockUnitsPerUpdateStandIn));
    }
}

}
