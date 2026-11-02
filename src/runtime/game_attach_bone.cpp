#include "game_level.hpp"

#include <iostream>

namespace game_level {
namespace runtime = chase::runtime;

void LevelSession::followBoneAttached() {
    for (const auto& [instance, attachment] : levelScripts.animation().attachments()) {
        const bool onBone = attachment.kind == runtime::AttachKind::Bone || attachment.kind == runtime::AttachKind::BoneRotated;
        if (!onBone) {
            boneAttachments.detach(instance);
            continue;
        }
        const std::optional<chase::assets::Matrix> bind = levelCharacters->jointBind(attachment.parent, attachment.parentBone);
        if (!bind) boneAttachments.detach(instance);
        if (!bind || !placedNodeWorld(instance)) continue;
        if (boneAttachments.attach(instance, attachment, *bind))
            std::cout << "prop " << instance << ": attached to bone " << attachment.parentBone << " of " << attachment.parent << " at update " << updates << '\n';
    }
    for (const auto& [instance, link] : boneAttachments.links()) {
        const auto frame = levelCharacters->jointFrame(link.parent, link.joint);
        if (!frame) continue;
        const std::optional<std::size_t> index = placedSceneByInstance.at(instance);
        scene.setTransform(*index, runtime::boneAttachedWorld(link.local, frame->skin, frame->world));
    }
}

}
