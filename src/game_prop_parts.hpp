#pragma once
// Moving parts of placed props in the scene: the bricks of build-its (docs/re/buildit.md) and the Badge child of the waypoints
// (docs/re/objective-hud.md). Logic lives in runtime/buildit_bricks, runtime/mesh_bouncer and runtime/prop_nodes.
#include "assets/assets.hpp"
#include "render/scene.hpp"
#include "runtime/buildit_bricks.hpp"
#include "runtime/buildit_set.hpp"
#include "runtime/mesh_bouncer.hpp"
#include <cstddef>
#include <vector>

namespace game_prop_parts {

class PropParts {
public:
    // The waypoint at `waypointIndex` (same order as the scene's waypoint instances) gets its class's child objects.
    void addWaypointChildren(chase::render::Scene& scene, const chase::assets::ClassLookup& classOf, std::size_t waypointIndex,
                             const chase::assets::Placement& waypoint);
    // Build-its: the parts model is drawn node by node. `placed` and `placedScene` are the level's placed objects and their scene instances.
    void loadBuildits(chase::render::Scene& scene, chase::assets::FuseArchive& archive, const chase::runtime::BuilditSet& buildits,
                      const std::vector<chase::assets::Placement>& placed, const std::vector<std::size_t>& placedScene);
    void step(float dt);
    // `shown[i]` tells whether placed object i is drawn (enabled and in a shown room), `waypointShown[w]` the same for waypoint w.
    void apply(chase::render::Scene& scene, const chase::runtime::BuilditSet& buildits, const std::vector<chase::assets::Placement>& placed,
               const std::vector<std::size_t>& placedScene, const std::vector<bool>& shown, const std::vector<bool>& waypointShown) const;

private:
    struct Child {
        std::size_t waypoint;
        std::size_t instance;
        chase::runtime::NodeMatrix parent;
        chase::runtime::NodeMatrix local;
        bool bouncer;
        chase::runtime::MeshBouncer motion;
    };
    struct Parts {
        std::size_t buildit, placedIndex;
        std::ptrdiff_t finalIndex;
        chase::runtime::BuilditBricks bricks;
        std::vector<std::ptrdiff_t> nodeInstance;
    };
    std::vector<Child> children_;
    std::vector<Parts> parts_;
};

}
