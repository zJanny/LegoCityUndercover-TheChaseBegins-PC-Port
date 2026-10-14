#include "game_doors.hpp"
#include <algorithm>
#include <cctype>

namespace game_doors {
using namespace chase::assets;
using namespace chase::render;
namespace runtime = chase::runtime;

std::string lowerCase(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

std::vector<DoorObject> loadDoors(Scene& scene, FuseArchive& archive, const Level& level, const ClassLookup& classOf, const std::vector<ObjectCollision>& objects,
                                  const std::map<std::string, std::pair<std::size_t, Mat4>>& instances) {
    std::vector<DoorObject> doors;
    for (const ObjectCollision& object : objects) {
        if (!isDoorClass(object.className)) continue;
        const auto instance = instances.find(object.instance);
        if (instance == instances.end() || object.model.empty()) continue;
        const LevelObject* source = nullptr;
        for (const LevelObject& candidate : level.objects)
            if (candidate.instance == object.instance) source = &candidate;
        const FuseEntry* modelEntry = archive.find(object.model);
        if (!source || !modelEntry) continue;
        const Model model = parseModel(archive.read(*modelEntry));
        DoorObject door;
        door.instance = object.instance;
        door.soundOwner = std::uint16_t(doors.size() + 1);
        door.params = runtime::doorParams(*source, *classOf(source->className), model, object.centre, object.half);
        door.sceneInstance[0] = instance->second.first;
        door.placement = instance->second.second;
        for (std::size_t n = 0; n < model.nodeMatrices.size() && n < 2; ++n)
            if (model.nodeMatrices[n][15] != 0.0f) door.rest[n] = model.nodeMatrices[n];
        if (door.params.twoLeaves && model.nodeMesh.size() > 1 && model.nodeMesh[0] >= 0 && model.nodeMesh[1] >= 0 &&
            scene.setInstanceMesh(door.sceneInstance[0], object.model, model.nodeMesh[0])) {
            door.sceneInstance[1] = scene.instances();
            door.separateLeaves = scene.addModel(object.model, door.placement, {}, object.model + "#" + std::to_string(model.nodeMesh[1]), model.nodeMesh[1]);
        }
        for (std::size_t n = 0; n < model.nodePivot.size() && n < 2; ++n) std::copy_n(model.nodePivot[n].data(), 3, door.pivot[n]);
        door.position = {door.placement[12], door.placement[13], door.placement[14]};
        doors.push_back(std::move(door));
    }
    return doors;
}

void poseDoor(Scene& scene, const DoorObject& door) {
    const runtime::DoorPose pose = runtime::doorPose(door.state, door.params, door.pivot);
    for (int leaf = 0; leaf < (door.separateLeaves ? 2 : 1); ++leaf)
        scene.setTransform(door.sceneInstance[leaf], chase::render::multiply(door.placement, chase::render::multiply(pose.node[leaf], door.rest[leaf])));
}

}
