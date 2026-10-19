#pragma once
// Zipline ropes of chase-game (draw 0x297b7c): textured triangle strips from runtime::ziplineRopeDraws, drawn after the opaque scene. A rope at full
// alpha is drawn without blending and writes depth; a faded rope blends. Textures load from the archive path of the RopeTexture name with its
// extension turned into .btga. Render states are stand-ins (TRAV-S28).
#include "assets/assets.hpp"
#include "render/scene.hpp"
#include "runtime/zipline_rope.hpp"
#include <memory>
#include <vector>

namespace chase::render {

class RopeRendererImpl;

class RopeRenderer {
public:
    explicit RopeRenderer(assets::FuseArchive& archive);
    ~RopeRenderer();
    RopeRenderer(const RopeRenderer&) = delete;
    RopeRenderer& operator=(const RopeRenderer&) = delete;

    void draw(const Mat4& viewProjection, const std::vector<runtime::RopeDraw>& draws);
    unsigned missingTextures() const;

private:
    std::unique_ptr<RopeRendererImpl> impl_;
};

}
