#pragma once
// The placed props/prop_trafficlight objects drawn node by node so that the parts red, yellow and green follow the junction's light state
// (docs/re/traffic-junction.md). Logic lives in runtime/traffic_light_parts.
#include "assets/assets.hpp"
#include "render/scene.hpp"
#include "runtime/traffic_light_parts.hpp"
#include <array>
#include <cstddef>
#include <vector>

namespace game_traffic_lights {

class TrafficLightLooks {
public:
    // The junction objects of `level` get the part flags of their set-up; each light found among `placed` is drawn node by node.
    void load(chase::render::Scene& scene, chase::assets::FuseArchive& archive, const chase::assets::Level& level,
              const std::vector<chase::assets::Placement>& placed, const std::vector<std::size_t>& placedScene);
    // The 0x20b19c changes of the cycle 0x1d8924 of junction `junction` (the level's junction objects in order).
    void applyChange(std::size_t junction, const chase::runtime::TrafficLightChange& change);
    void apply(chase::render::Scene& scene, const std::vector<bool>& shownPlaced) const;
    std::size_t lights() const { return lights_.size(); }
    std::size_t junctions() const { return states_.junctions(); }

private:
    struct Light {
        std::size_t junction, pair, member, placedIndex, sceneInstance;
        std::vector<std::ptrdiff_t> nodeInstance;
        std::array<std::ptrdiff_t, chase::runtime::kTrafficLightPartCount> partNode;
    };
    chase::runtime::TrafficLightPartStates states_;
    std::vector<Light> lights_;
};

}
