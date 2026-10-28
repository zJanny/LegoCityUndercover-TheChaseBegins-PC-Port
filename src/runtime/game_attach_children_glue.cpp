#include "game_level.hpp"

#include <iostream>

namespace game_level {
namespace runtime = chase::runtime;

namespace {
runtime::PlatformVector ownOffset(const runtime::PlatformSet& platforms, const std::string& child, const std::string& parent) {
    const runtime::PlatformVector all = platforms.carriedOffset(child);
    const runtime::PlatformVector inherited = platforms.carriedOffset(parent);
    return {all[0] - inherited[0], all[1] - inherited[1], all[2] - inherited[2]};
}
}

void LevelSession::attachPlacedChildren() {
    for (const auto& [instance, attachment] : levelScripts.animation().attachments()) {
        if (levelCharacters->world(instance)) continue;
        if (attachment.kind != runtime::AttachKind::Object) {
            propAttachments.detach(instance);
            continue;
        }
        const std::optional<chase::assets::Matrix> world = placedNodeWorld(instance);
        const std::optional<chase::assets::Matrix> parentWorld = placedNodeWorld(attachment.parent);
        if (!world || !parentWorld) continue;
        if (propAttachments.attach(instance, attachment.parent, attachment.sequence, *world, *parentWorld)) {
            propOwnOffsets[instance] = ownOffset(levelScripts.platforms(), instance, attachment.parent);
            std::cout << "prop " << instance << ": attached to " << attachment.parent << " at update " << updates << '\n';
        }
    }
}

void LevelSession::followPlacedChildren() {
    attachPlacedChildren();
    const auto sceneIndexOf = [this](const std::string& instance) -> std::optional<std::size_t> {
        for (std::size_t i = 0; i < placedObjects.size(); ++i)
            if (game_doors::lowerCase(placedObjects[i].instance) == instance) return placedSceneIndex[i];
        return std::nullopt;
    };
    struct Shift {
        std::string instance;
        runtime::PlatformVector delta;
        chase::assets::Matrix parentWorld;
    };
    std::vector<Shift> shifts;
    for (const auto& [instance, link] : propAttachments.links()) {
        const runtime::PlatformVector own = ownOffset(levelScripts.platforms(), instance, link.parent);
        runtime::PlatformVector& last = propOwnOffsets[instance];
        const runtime::PlatformVector delta{own[0] - last[0], own[1] - last[1], own[2] - last[2]};
        last = own;
        if (delta[0] == 0.0f && delta[1] == 0.0f && delta[2] == 0.0f) continue;
        if (const std::optional<chase::assets::Matrix> parentWorld = placedNodeWorld(link.parent)) shifts.push_back({instance, delta, *parentWorld});
    }
    for (const Shift& shift : shifts) propAttachments.translate(shift.instance, shift.delta.data(), shift.parentWorld);
    const auto worldOf = [this](const std::string& instance) { return placedNodeWorld(instance); };
    for (const auto& [instance, world] : propAttachments.follow(worldOf)) {
        const std::optional<std::size_t> index = sceneIndexOf(instance);
        if (!index) continue;
        scene.setTransform(*index, world);
        collision.moveObject(instance, world);
    }
}

}
