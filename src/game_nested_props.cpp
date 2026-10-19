#include "game_level.hpp"
#include "runtime/nested_props.hpp"
#include "runtime/object_visibility.hpp"

#include <iostream>
#include <map>

namespace game_level {
using namespace chase;

void LevelSession::placeNestedProps(assets::FuseArchive& archive) {
    std::map<std::string, std::vector<assets::Placement>> byClass;
    for (std::size_t i = 0; i < placedObjects.size(); ++i) {
        auto known = byClass.find(placedObjects[i].className);
        if (known == byClass.end()) {
            const assets::Level* classFile = classOf(placedObjects[i].className);
            known = byClass.emplace(placedObjects[i].className, classFile ? runtime::nestedProps(archive, *classFile, classOf) : std::vector<assets::Placement>{}).first;
        }
        const assets::Matrix parent = assets::placementMatrix(placedObjects[i]);
        for (const assets::Placement& child : known->second) {
            const std::size_t instance = scene.instances();
            if (!scene.addModel(assets::meshModelPath(child.mesh), assets::multiply(assets::placementMatrix(child), parent))) continue;
            nestedPropSceneIndex.push_back({i, instance});
        }
    }
    std::cout << "nested objects drawn: " << nestedPropSceneIndex.size() << '\n';
}

void LevelSession::showNestedProps(const std::vector<bool>& drawn) {
    for (const auto& [placedIndex, instance] : nestedPropSceneIndex)
        scene.setInstanceVisible(instance, runtime::nestedObjectDrawn(drawn[placedIndex]) && roomShown(placedRooms[placedIndex]));
}

}
