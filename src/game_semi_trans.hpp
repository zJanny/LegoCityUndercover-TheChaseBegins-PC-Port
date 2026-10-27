#pragma once
// The near-camera semi-transparent lists in chase-game (runtime/semi_trans_near_camera.hpp): the scene instances an
// AddToSemiTransNearCameraList entry names, their alpha after each update and the alpha they are drawn with.
#include "assets/assets.hpp"
#include "render/scene.hpp"
#include "runtime/natives_effects.hpp"
#include "runtime/semi_trans_near_camera.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace game_semi_trans {

namespace runtime = chase::runtime;

// The camera matrix rows and the player position 0x2e5b14 passes, from the follow camera.
runtime::NearCameraView followView(const float eye[3], float yaw, float pitch, const float player[3]);

class NearCameraFades {
public:
    // 0x2cb734 for the entries the scripts added since the last call, then 0x1ddde4 over every entry.
    // STAND-IN RENDER-S72: every scene instance of the model (or of `<model>_atlas_<level>`) is an entry (the original walks the entities of
    // the level's rooms); the entry name (a sub-node) is ignored; the entry box is the model bounds 0x242eb0, where 0x1ddde4 reads the node
    // box +0xa0 / +0xac.
    void update(chase::assets::FuseArchive& archive, const chase::render::Scene& scene, const std::string& levelName,
                const std::vector<runtime::SemiTransNearCameraEntry>& entries, const runtime::NearCameraView& view);
    // STAND-IN RENDER-S72: the entry alpha caps the instance alpha the other fades set (both write the node alpha; their order is not read).
    void apply(chase::render::Scene& scene);

private:
    struct Tracked {
        std::size_t instance;
        runtime::Vector3 centre, half;
        std::int16_t alpha = runtime::kSemiTransOpaque;
        float base = 1.0f, written = -1.0f;
    };
    std::size_t seenEntries_ = 0;
    std::vector<Tracked> tracked_;
};

}
