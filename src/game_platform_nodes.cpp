#include "game_level.hpp"
#include <iostream>

namespace game_level {
namespace runtime = chase::runtime;

bool LevelSession::objectNodePosition(const std::string& instance, float out[3]) const {
    const std::optional<std::array<float, 3>> translation = objectNodeTranslation(instance);
    if (!translation) return false;
    std::copy(translation->begin(), translation->end(), out);
    return true;
}

void LevelSession::carryPlatformProps() {
    runtime::PlatformSet& platforms = levelScripts.platforms();
    for (const runtime::PlatformArrival& arrival : platforms.takeArrivals())
        std::cout << "platform " << arrival.instance << ": travel ended in state " << arrival.state << " at " << arrival.node[0] << ' ' << arrival.node[1]
                  << ' ' << arrival.node[2] << " after " << arrival.movedUpdates << " node moves at update " << updates << '\n';
    for (std::size_t i = 0; i < switches.objects().size(); ++i) {
        const runtime::PlatformVector offset = platforms.carriedOffset(switches.objects()[i].instance);
        switches.setCarriedOffset(i, offset.data());
    }
    for (const runtime::PlatformArrival& lap : platforms.takeLaps())
        std::cout << "platform " << lap.instance << ": lap ended at " << lap.node[0] << ' ' << lap.node[1] << ' ' << lap.node[2] << " after "
                  << lap.movedUpdates << " node moves at update " << updates << '\n';
    std::set<std::string> moved;
    for (const std::string& name : platforms.takeMoved()) {
        moved.insert(name);
        platformCarried.insert(name);
    }
    for (std::size_t i = 0; i < placedObjects.size(); ++i) {
        const std::string name = game_doors::lowerCase(placedObjects[i].instance);
        if (!platformCarried.count(name)) continue;
        chase::assets::Matrix transform = scene.instanceTransform(placedSceneIndex[i]);
        const runtime::PlatformVector offset = platforms.carriedOffset(name);
        const runtime::PathRotation turn = platforms.carriedRotation(name);
        if (turn != runtime::kIdentityPathRotation) {
            const chase::assets::Matrix placed = chase::assets::placementMatrix(placedObjects[i]);
            const runtime::PathRotation placedRows = {placed[0], placed[1], placed[2], placed[4], placed[5], placed[6], placed[8], placed[9], placed[10]};
            const runtime::PathRotation rows = runtime::multiplyPathRotation(placedRows, turn);
            for (std::size_t row = 0; row < 3; ++row) std::copy_n(rows.begin() + std::ptrdiff_t(row * 3), 3, transform.begin() + std::ptrdiff_t(row * 4));
        }
        for (int axis = 0; axis < 3; ++axis) transform[std::size_t(12 + axis)] = placedObjects[i].position[axis] + offset[axis];
        scene.setTransform(placedSceneIndex[i], transform);
        if (moved.count(name)) collision.moveObject(name, transform);
    }
}

}
