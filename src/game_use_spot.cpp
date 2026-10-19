#include "game_level.hpp"
#include <cmath>
#include <iostream>

namespace game_level {
namespace runtime = chase::runtime;
namespace render = chase::render;

// STAND-IN SCRIPT-S249: the marker stands at the zone centre in the spot's frame; the ground search of 0x1db800 (a ray 5 m
// down with a 0.5 m step up past objects) is not ported. It is drawn like a particle quad (alpha blend, depth test
// less-or-equal, no depth write); the render state bytes that 0x1b29f0 hands to 0x25511c are not decoded.
void LevelSession::updateUseMarkers(float dt) {
    if (!player || !follow) return;
    const bool driving = boarding && boarding->seatedVehicle();
    std::vector<runtime::UseMarkerCandidate> candidates;
    for (const runtime::UseSpot& spot : useSpots.spots()) {
        runtime::UseMarkerCandidate candidate;
        candidate.instance = spot.instance;
        candidate.matrix = spot.matrix;
        for (int a = 0; a < 3; ++a)
            candidate.ground[std::size_t(a)] = spot.matrix[std::size_t(12 + a)] + spot.zoneCentre[0] * spot.matrix[std::size_t(a)] +
                                               spot.zoneCentre[1] * spot.matrix[std::size_t(4 + a)] + spot.zoneCentre[2] * spot.matrix[std::size_t(8 + a)];
        candidate.usable = !driving && spot.active && !spot.triggerObject.empty() && !objectState.isDisabled(spot.instance);
        candidates.push_back(std::move(candidate));
    }
    const float playerPosition[3] = {player->motion.x, player->vertical.y, player->motion.z};
    const float eye[3] = {follow->eye()[0], follow->eye()[1], follow->eye()[2]};
    std::vector<bool> before;
    for (const runtime::UseMarkerCandidate& candidate : candidates) before.push_back(useMarkers.slotted(candidate.instance));
    useMarkers.update(candidates, eye, playerPosition, dt);
    for (std::size_t i = 0; i < candidates.size(); ++i)
        if (before[i] != useMarkers.slotted(candidates[i].instance))
            std::cout << "use marker: " << candidates[i].instance << (before[i] ? " gone" : " shown") << " at update " << updates << '\n';
}

void LevelSession::drawUseMarkers(const render::Mat4& viewProjection) {
    const std::vector<runtime::UseMarkerQuad> quads = useMarkers.quads();
    if (quads.empty()) return;
    if (!useMarkerRenderer)
        useMarkerRenderer.emplace(shared.archive, std::vector<std::string>(runtime::kUseMarkerTextures.begin(), runtime::kUseMarkerTextures.end()),
                                  runtime::kUseMarkerTextureDirectory);
    std::vector<runtime::ParticleBatch> batches;
    for (const runtime::UseMarkerQuad& quad : quads) {
        runtime::ParticleBatch batch;
        batch.texture = quad.texture;
        const std::uint8_t alpha = std::uint8_t(std::lround(quad.alpha * 255.0f));
        for (const runtime::UseMarkerVertex& vertex : quad.vertices)
            batch.vertices.push_back({{vertex.position[0], vertex.position[1], vertex.position[2]}, {vertex.uv[0], vertex.uv[1]}, {255, 255, 255, alpha}});
        batches.push_back(std::move(batch));
    }
    useMarkerRenderer->draw(viewProjection, batches);
}

}
