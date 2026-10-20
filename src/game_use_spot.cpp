#include "game_level.hpp"
#include "runtime/use_marker_ground.hpp"

#include <cmath>
#include <iostream>
#include <string>

namespace game_level {
namespace runtime = chase::runtime;
namespace render = chase::render;

void LevelSession::loadUseMarkers(chase::assets::FuseArchive& archive) {
    if (const chase::assets::FuseEntry* entry = archive.find("tables/tablescharacters.bxls")) abilityTable.load(chase::assets::parseTable(archive.read(*entry)));
}

// STAND-IN SCRIPT-S249: the marker position is the nearest hit of a segment 5 m down from the zone centre (runtime::useMarkerGround), not the ray
// of 0x1db800 with its object list and step-up; it is drawn like a particle quad (alpha blend, depth test less-or-equal, no depth write); the
// render state bytes that 0x1b29f0 hands to 0x25511c are not decoded.
// STAND-IN SCRIPT-S286: the party list 0x51b248 is not ported; the marker asks only the player character (kPlayerCharacterId) for the ability, and
// the case of 0x292388 where the player entity has kind 159 (character 141) is not modelled.
// STAND-IN SCRIPT-S287: only use spots (165) and detective spots (182) feed the markers; the message 3 answer of the other 34 kinds is not read.
void LevelSession::updateUseMarkers(float dt) {
    if (!player || !follow) return;
    const bool driving = boarding && boarding->seatedVehicle();
    std::vector<runtime::UseMarkerCandidate> candidates;
    const auto add = [&](const std::string& instance, const std::array<float, 16>& matrix, const float zoneCentre[3], bool usable, unsigned kind) {
        runtime::UseMarkerCandidate candidate;
        candidate.instance = instance;
        candidate.matrix = matrix;
        candidate.usable = !driving && usable && !objectState.isDisabled(instance);
        if (candidate.usable && !useMarkers.slotted(instance)) candidate.ground = runtime::useMarkerGround(collision, matrix, zoneCentre);
        const int ability = runtime::useMarkerKindAbility(kind);
        const bool lockedInSave = abilityTable.startsLocked(ability) && !levelScripts.progressNatives().bit(runtime::kAbilityOffset, std::uint32_t(ability));
        candidate.texture = runtime::useMarkerTexture(ability, lockedInSave, abilityTable.characterHas(game_player::kPlayerCharacterId, ability));
        candidates.push_back(std::move(candidate));
    };
    for (const runtime::UseSpot& spot : useSpots.spots())
        add(spot.instance, spot.matrix, spot.zoneCentre, spot.active && !spot.triggerObject.empty(), runtime::kUseSpotKind);
    for (const runtime::DetectiveSpot& spot : detective.spots())
        add(spot.instance, spot.matrix, spot.useZone.centre, spot.usable && runtime::detectiveSpotAnswersUseQuery(spot), runtime::kDetectiveSpotType);
    const float playerPosition[3] = {player->motion.x, player->vertical.y, player->motion.z};
    const float eye[3] = {follow->eye()[0], follow->eye()[1], follow->eye()[2]};
    std::vector<bool> before;
    for (const runtime::UseMarkerCandidate& candidate : candidates) before.push_back(useMarkers.slotted(candidate.instance));
    useMarkers.update(candidates, eye, playerPosition, dt);
    for (std::size_t i = 0; i < candidates.size(); ++i)
        if (before[i] != useMarkers.slotted(candidates[i].instance))
            std::cout << "use marker: " << candidates[i].instance << (before[i] ? " gone" : " shown") << " at update " << updates
                      << (before[i] ? std::string() : std::string(" (") + runtime::kUseMarkerTextures[std::size_t(candidates[i].texture)] + ")") << '\n';
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
