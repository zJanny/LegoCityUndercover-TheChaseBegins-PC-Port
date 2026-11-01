#pragma once
// Placed objects hung on a bone of a character by AttachToBone / AttachToBoneRot (docs/re/script-natives-animation.md, entry "Bone attaches
// of characters"). AttachToBoneRot 0x1bf718 builds the rotation matrix of 0x237c48 from its three angles in degrees and hands it to 0x20f33c;
// that makes the child's local matrix the argument matrix with the bone's bind translation added (0x23d39c on the translation rows) and links
// the node under the parent's node with a bone record (0x242814, 0x2023bc). The record's handler 0x109c2c / 0x10f800 gives the node's world
// every frame as local * skin matrix of the bone * world of the parent's node. AttachToBone without a child bone takes 0x22acdc (0x2ca88c),
// the rule of the hand items (STAND-IN SCRIPT-S317: the bone's whole bind matrix as the local matrix). No SDL dependency.
#include "assets/assets.hpp"
#include "runtime/natives_animation.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <string>

namespace chase::runtime {

// The matrix 0x1bf718 passes on: rotation of 0x237c48 from the angles (degrees, 0.0174532942 each), no translation.
assets::Matrix boneAttachArgument(const float rotationDegrees[3]);
// 0x20f33c: the argument matrix with the bone's bind translation (row 3 of the joint's bind world matrix) added.
assets::Matrix boneAttachLocal(const assets::Matrix& argument, const assets::Matrix& jointBind);
// 0x109c2c with a parent node: local * skin matrix of the bone * world of the parent node.
assets::Matrix boneAttachedWorld(const assets::Matrix& local, const assets::Matrix& jointSkin, const assets::Matrix& parentWorld);

class BoneAttachments {
public:
    struct Link {
        std::string parent, joint;
        assets::Matrix local{};
        std::uint32_t sequence = 0;
    };
    // `jointBind` is the bind world matrix of `attachment.parentBone` in the parent's skeleton. Returns false when this call was already taken
    // (same child, same call number). Kinds other than Bone and BoneRotated detach the child.
    bool attach(const std::string& lowerChild, const Attachment& attachment, const assets::Matrix& jointBind);
    void detach(const std::string& lowerChild) { links_.erase(lowerChild); }
    const Link* find(const std::string& lowerChild) const;
    const std::map<std::string, Link>& links() const { return links_; }

private:
    std::map<std::string, Link> links_;
};

}
