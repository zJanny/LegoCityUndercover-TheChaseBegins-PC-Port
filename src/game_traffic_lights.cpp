#include "game_traffic_lights.hpp"
#include "game_doors.hpp"
#include "runtime/prop_nodes.hpp"
#include <iostream>

namespace game_traffic_lights {
using namespace chase;

void TrafficLightLooks::load(render::Scene& scene, assets::FuseArchive& archive, const assets::Level& level,
                             const std::vector<assets::Placement>& placed, const std::vector<std::size_t>& placedScene) {
    for (const runtime::TrafficJunctionFill& fill : runtime::trafficJunctionFills({&level})) {
        const std::size_t junction = states_.junctions();
        states_.add(fill);
        for (std::size_t pair = 0; pair < fill.lightInstances.size(); ++pair)
            for (std::size_t member = 0; member < 2; ++member) {
                const std::string name = game_doors::lowerCase(fill.lightInstances[pair][member]);
                if (name.empty()) continue;
                std::size_t index = 0;
                while (index < placed.size() && game_doors::lowerCase(placed[index].instance) != name) ++index;
                if (index == placed.size()) continue;
                const std::string modelPath = assets::meshModelPath(placed[index].mesh);
                const assets::FuseEntry* entry = archive.find(modelPath);
                if (entry == nullptr) continue;
                const assets::Model model = assets::parseModel(archive.read(*entry));
                const runtime::PropNodes nodes = runtime::PropNodes::of(model);
                const runtime::NodeMatrix placement = assets::placementMatrix(placed[index]);
                Light light{junction, pair, member, index, placedScene[index], scene.addModelNodes(modelPath, placement), {}};
                if (light.nodeInstance.empty()) continue;
                for (std::size_t node = 0; node < light.nodeInstance.size(); ++node)
                    if (light.nodeInstance[node] >= 0) scene.setTransform(std::size_t(light.nodeInstance[node]), nodes.world(node, placement));
                for (std::size_t part = 0; part < light.partNode.size(); ++part)
                    light.partNode[part] = runtime::trafficLightPartNode(model.nodeNames, std::uint8_t(part));
                lights_.push_back(std::move(light));
            }
    }
    std::cout << "traffic lights: " << lights_.size() << " in " << states_.junctions() << " junctions\n";
}

void TrafficLightLooks::applyChange(std::size_t junction, const runtime::TrafficLightChange& change) {
    states_.applyChange(junction, change);
}

void TrafficLightLooks::apply(render::Scene& scene, const std::vector<bool>& shownPlaced) const {
    for (const Light& light : lights_) {
        const bool shown = light.placedIndex < shownPlaced.size() && shownPlaced[light.placedIndex];
        scene.setInstanceVisible(light.sceneInstance, false);
        for (std::size_t node = 0; node < light.nodeInstance.size(); ++node) {
            if (light.nodeInstance[node] < 0) continue;
            bool visible = shown;
            for (std::size_t part = 0; part < light.partNode.size(); ++part)
                if (light.partNode[part] == std::ptrdiff_t(node) && !states_.shown(light.junction, light.pair, light.member, std::uint8_t(part))) visible = false;
            scene.setInstanceVisible(std::size_t(light.nodeInstance[node]), visible);
        }
    }
}

}
