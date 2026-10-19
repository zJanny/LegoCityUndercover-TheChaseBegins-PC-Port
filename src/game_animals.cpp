#include "game_level.hpp"

namespace game_level {
namespace runtime = chase::runtime;

void LevelSession::updatePropAnimations(float dt) {
    propAnimations.update(dt);
    for (const runtime::PropPose& pose : propAnimations.poses()) {
        scene.pose(pose.key, pose.skin);
        if (!pose.moved) continue;
        for (std::size_t i = 0; i < placedObjects.size(); ++i)
            if (game_doors::lowerCase(placedObjects[i].instance) == pose.instance) scene.setTransform(placedSceneIndex[i], pose.node);
    }
}

}
