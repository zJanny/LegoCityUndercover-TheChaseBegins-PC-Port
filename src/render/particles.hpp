#pragma once
// Particle quads of chase-game (draw 0x2722cc): the batches built by runtime::ParticleSystem::buildDraw, drawn after the opaque
// scene with the blend of the definition (byte +0x177), depth test less-or-equal and no depth write (the draw's state bytes in
// docs/re/particles.md). Textures load from particles/<name>.btga when first used.
#include "assets/assets.hpp"
#include "render/scene.hpp"
#include "runtime/particles.hpp"
#include <memory>
#include <string>
#include <vector>

namespace chase::render {

class ParticleRendererImpl;

class ParticleRenderer {
public:
    // Textures load from <directory><name>.btga.
    ParticleRenderer(assets::FuseArchive& archive, std::vector<std::string> textureNames, std::string directory = "particles/");
    ~ParticleRenderer();
    ParticleRenderer(const ParticleRenderer&) = delete;
    ParticleRenderer& operator=(const ParticleRenderer&) = delete;

    // `viewProjection` takes game-space world coordinates, as the other scene draws do.
    void draw(const Mat4& viewProjection, const std::vector<runtime::ParticleBatch>& batches);
    unsigned missingTextures() const;

private:
    std::unique_ptr<ParticleRendererImpl> impl_;
};

}
