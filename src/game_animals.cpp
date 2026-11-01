#include "game_level.hpp"
#include <iostream>

namespace game_level {
namespace runtime = chase::runtime;

void LevelSession::updatePropAnimations(float dt) {
    for (const runtime::AnimationPlay& start : catRescue.update(dt)) propAnimations.play(start);
    propAnimations.update(dt);
    for (const runtime::PropPose& pose : propAnimations.poses()) {
        bool heldByCutscene = false;
        for (std::size_t i = 0; i < placedObjects.size(); ++i)
            if (game_doors::lowerCase(placedObjects[i].instance) == pose.instance) heldByCutscene = cutsceneHost->holdsObject(placedObjects[i].objectIndex);
        if (heldByCutscene) {
            if (heldPropsLogged.insert(pose.instance).second) std::cout << "prop " << pose.instance << ": its own animation is not shown while a cutscene holds it\n";
            continue;
        }
        scene.pose(pose.key, pose.skin);
        if (!pose.delta) continue;
        for (std::size_t i = 0; i < placedObjects.size(); ++i)
            if (game_doors::lowerCase(placedObjects[i].instance) == pose.instance)
                scene.setTransform(placedSceneIndex[i], runtime::applyRootDelta(scene.instanceTransform(placedSceneIndex[i]), *pose.delta));
    }
}

bool LevelSession::placeProp(const std::string& instance, const chase::assets::Placement& target) {
    for (std::size_t i = 0; i < placedObjects.size(); ++i) {
        if (game_doors::lowerCase(placedObjects[i].instance) != instance) continue;
        const chase::assets::Matrix node = placementMatrix(target);
        propAnimations.place(instance, node);
        scene.setTransform(placedSceneIndex[i], node);
        placedRooms[i] = runtime::roomNumber(target.room);
        collision.moveObject(instance, node);
        return true;
    }
    return false;
}

}
