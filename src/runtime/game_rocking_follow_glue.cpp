#include "game_level.hpp"

#include <cmath>
#include <iostream>

namespace game_level {
namespace runtime = chase::runtime;

namespace {
constexpr float kAttachedPositionEpsilon = 1e-5f;
}

std::optional<std::array<float, 3>> LevelSession::objectNodeTranslation(const std::string& instance) const {
    float carried[3];
    if (carry.position(instance, carried)) return std::array<float, 3>{carried[0], carried[1], carried[2]};
    const std::optional<chase::assets::Placement> start = chase::assets::objectStart(level, instance);
    if (!start) return std::nullopt;
    const std::array<float, 3> offset = levelScripts.platforms().carriedOffset(game_doors::lowerCase(instance));
    return std::array<float, 3>{start->position[0] + offset[0], start->position[1] + offset[1], start->position[2] + offset[2]};
}

std::optional<chase::assets::Matrix> LevelSession::placedNodeWorld(const std::string& instance) {
    auto found = placedSceneByInstance.find(instance);
    if (found == placedSceneByInstance.end()) {
        std::optional<std::size_t> index;
        for (std::size_t i = 0; i < placedObjects.size() && !index; ++i)
            if (game_doors::lowerCase(placedObjects[i].instance) == instance) index = placedSceneIndex[i];
        found = placedSceneByInstance.emplace(instance, index).first;
    }
    if (!found->second) return std::nullopt;
    return scene.instanceTransform(*found->second);
}

void LevelSession::attachLevelCharacters() {
    for (const auto& [instance, character] : levelCharacters->characters()) {
        const runtime::Attachment* attachment = levelScripts.animation().attachment(instance);
        const runtime::NodeAttachments::Link* link = characterAttachments.find(instance);
        if (!attachment || attachment->kind != runtime::AttachKind::Object) {
            if (link) characterAttachments.detach(instance);
            continue;
        }
        std::uint32_t& taken = characterAttachCalls[instance];
        if (link && link->parent == attachment->parent && taken == attachment->sequence) continue;
        const std::optional<chase::assets::Matrix> world = levelCharacters->world(instance);
        const std::optional<chase::assets::Matrix> parentWorld = placedNodeWorld(attachment->parent);
        if (!world || !parentWorld) continue;
        characterAttachments.attach(instance, attachment->parent, *world, *parentWorld);
        taken = attachment->sequence;
        std::cout << "character " << instance << ": attached to " << attachment->parent << " at update " << updates << '\n';
    }
}

void LevelSession::followAttachedCharacters() {
    runtime::AiCore& ai = levelScripts.ai();
    for (const auto& [instance, link] : characterAttachments.links()) {
        const std::optional<chase::assets::Matrix> parentWorld = placedNodeWorld(link.parent);
        const std::optional<chase::assets::Matrix> world = levelCharacters->world(instance);
        if (!parentWorld || !world) continue;
        const std::optional<chase::assets::Matrix> followed = characterAttachments.follow(instance, *world, *parentWorld);
        if (!followed) continue;
        levelCharacters->place(instance, *followed);
        const float moved = std::fabs((*followed)[12] - (*world)[12]) + std::fabs((*followed)[13] - (*world)[13]) + std::fabs((*followed)[14] - (*world)[14]);
        if (moved > kAttachedPositionEpsilon && !ai.movedLastUpdate(instance)) ai.setPosition(instance, followed->data() + 12);
    }
}

void LevelSession::snapAiMovedCharacters(float dt) {
    runtime::AiCore& ai = levelScripts.ai();
    for (const auto& [instance, character] : levelCharacters->characters()) {
        if (!ai.movedLastUpdate(instance) || characterAttachments.find(instance)) continue;
        const runtime::AiCharacter* moved = ai.character(instance);
        const std::optional<chase::assets::Matrix> world = levelCharacters->world(instance);
        if ((moved && moved->superJump) || !world) continue;
        // STAND-IN SCRIPT-S306: the ground step of the character move 0x20cd30 is the player's ground probe; no fall without ground.
        runtime::GroundProbe probe;
        probe.x = (*world)[12];
        probe.y = (*world)[13];
        probe.z = (*world)[14];
        probe.dt = dt;
        const std::optional<float> height = runtime::groundHeight(collision, probe);
        if (!height || *height == probe.y) continue;
        chase::assets::Matrix snapped = *world;
        snapped[13] = *height;
        levelCharacters->place(instance, snapped);
        ai.setPosition(instance, snapped.data() + 12);
    }
}

}
