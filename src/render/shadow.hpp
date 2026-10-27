#pragma once
// The shadow passes (runtime/shadow.hpp, docs/re/rendering-shadows.md). STAND-IN RENDER-S67: the PICA shadow framebuffer and the
// shadow texture lookup of unit 0 are replaced by a 512 x 512 depth texture of the caster parts and a linear depth comparison, and the
// two combiner stages 0x12ecd4 appends run as a second draw of the receiving parts after the scene that multiplies the colour already
// drawn (so it also scales fog and the blended draws over them). Plain types only.
#include "render/scene.hpp"
#include <array>
#include <cstdint>
#include <memory>

namespace chase::render {

class ShadowPassImpl;

class ShadowPass {
public:
    ShadowPass();
    ~ShadowPass();
    ShadowPass(const ShadowPass&) = delete;
    ShadowPass& operator=(const ShadowPass&) = delete;

    // Renders the depth of the scene's caster parts as seen by `shadowViewProjection` (runtime::shadowViewProjection).
    void render(const Scene& scene, const Mat4& shadowViewProjection);
    // Multiplies the colour of the receiving parts by the shadow factor of the constant (runtime::shadowConstant) where the last
    // rendered texture says they are in shadow. Call after Scene::draw with the same view projection.
    void apply(const Scene& scene, const Mat4& viewProjection, const std::array<std::uint8_t, 3>& constant);

private:
    std::unique_ptr<ShadowPassImpl> impl_;
};

}
