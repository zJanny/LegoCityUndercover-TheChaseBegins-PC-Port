#include "runtime/bone_attach.hpp"

namespace chase::runtime {

assets::Matrix boneAttachArgument(const float rotationDegrees[3]) {
    assets::Placement rotation;
    for (int axis = 0; axis < 3; ++axis) rotation.orientation[axis] = rotationDegrees[axis];
    return assets::placementMatrix(rotation);
}

assets::Matrix boneAttachLocal(const assets::Matrix& argument, const assets::Matrix& jointBind) {
    assets::Matrix local = argument;
    for (int axis = 12; axis < 15; ++axis) local[std::size_t(axis)] += jointBind[std::size_t(axis)];
    return local;
}

assets::Matrix boneAttachedWorld(const assets::Matrix& local, const assets::Matrix& jointSkin, const assets::Matrix& parentWorld) {
    return assets::multiply(assets::multiply(local, jointSkin), parentWorld);
}

bool BoneAttachments::attach(const std::string& child, const Attachment& attachment, const assets::Matrix& jointBind) {
    if (attachment.kind != AttachKind::Bone && attachment.kind != AttachKind::BoneRotated) {
        detach(child);
        return false;
    }
    const auto taken = links_.find(child);
    if (taken != links_.end() && taken->second.sequence == attachment.sequence && taken->second.parent == attachment.parent) return false;
    Link link;
    link.parent = attachment.parent;
    link.joint = attachment.parentBone;
    link.sequence = attachment.sequence;
    link.local = attachment.kind == AttachKind::BoneRotated ? boneAttachLocal(boneAttachArgument(attachment.rotationDegrees), jointBind) : jointBind;
    links_[child] = link;
    return true;
}

const BoneAttachments::Link* BoneAttachments::find(const std::string& child) const {
    const auto found = links_.find(child);
    return found == links_.end() ? nullptr : &found->second;
}

}
