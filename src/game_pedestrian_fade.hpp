#pragma once
// The node fade of the pedestrians in chase-game (runtime/pedestrian_fade.hpp): the mesh node each slot hangs on, the start after a
// spawn, the per-frame step and the alpha the character instances are drawn with.
#include "assets/assets.hpp"
#include "assets/object_collision.hpp"
#include "render/character.hpp"
#include "render/scene.hpp"
#include "runtime/pedestrian_fade.hpp"
#include "runtime/pedestrian_system.hpp"

#include <optional>
#include <string>
#include <vector>

namespace game_pedestrian_fade {

namespace assets = chase::assets;
namespace render = chase::render;
namespace runtime = chase::runtime;

// STAND-IN PED-S11: the node bounds are the model's mesh box union (assets::modelBounds) and the node radius +0x9c is the length of its
// half extents (the source of the loader's 40-byte record word is not read).
runtime::PedestrianNodeBounds nodeBoundsOf(assets::FuseArchive& archive, const std::string& bodyModelPath);

class SlotFades {
public:
    // The level load: slot i's pedestrian node is a mesh node with `bounds[i]`.
    void load(runtime::PedestrianSystem& system, const std::vector<runtime::PedestrianNodeBounds>& bounds);
    // After a spawn: 0x275434 when the manager found the mesh node type.
    void spawned(const runtime::PedestrianSystem& system, std::size_t slot);
    // 0x10eb70 for every live pedestrian, at its node position after the update.
    void step(const runtime::PedestrianSystem& system, const std::vector<bool>& live, const runtime::PedestrianVec3& fadePoint);
    std::optional<runtime::PedestrianDrawTest> drawTest(const runtime::PedestrianSystem& system, std::size_t slot,
                                                        const runtime::PedestrianVec3& fadePoint) const;
    const runtime::PedestrianFades& fades() const { return fades_; }

private:
    runtime::PedestrianFades fades_;
    runtime::DistanceFadeTable table_;
};

// The opacity of the node's alpha byte on the body, head and hat instances.
void applyOpacity(render::Scene& scene, const render::CharacterInstance& instance, float opacity);

}
