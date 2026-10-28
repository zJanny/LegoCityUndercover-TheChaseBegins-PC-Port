#pragma once
// Scene nodes under a parent node (docs/re/props-rocking.md, entry "AttachObject carries the girder guy"): AttachObject 0x1bf68c links the child's
// node under the parent's with 0x245008, which keeps the child's world matrix by storing it relative to the parent's (0x258bc4); 0x24c118 sets
// a node's world matrix the same way when it has a parent and marks the children, whose world is their local matrix times the parent's world.
// Row vectors, translation in the last row (assets::Matrix). No SDL dependency.
#include "assets/assets.hpp"

#include <map>
#include <optional>
#include <string>

namespace chase::runtime {

// 0x258bc4: `world` times the rigid inverse of `parentWorld` (transposed rotation rows, translation -t R^T).
assets::Matrix relativeToParent(const assets::Matrix& world, const assets::Matrix& parentWorld);
// The world matrix of a child node: its local matrix times the parent's world.
assets::Matrix childWorld(const assets::Matrix& local, const assets::Matrix& parentWorld);

class NodeAttachments {
public:
    struct Link {
        std::string parent;
        assets::Matrix local{};
        assets::Matrix world{};  // the child's world after the last attach or follow
    };
    // 0x245008 for a child that has its world matrix `world` under a parent at `parentWorld`.
    void attach(const std::string& lowerChild, const std::string& lowerParent, const assets::Matrix& world, const assets::Matrix& parentWorld);
    void detach(const std::string& lowerChild);
    // 0x24c118 on an attached child: its new world matrix, kept relative to the parent. False when the child has no parent.
    bool setWorld(const std::string& lowerChild, const assets::Matrix& world, const assets::Matrix& parentWorld);
    // The child's world under the parent's current world. A `current` world other than the one of the last attach or follow was written by
    // something else (the character move, PositionObject) and is kept relative to the parent first, as 0x24c118 does. Nothing for a child
    // without a parent.
    std::optional<assets::Matrix> follow(const std::string& lowerChild, const assets::Matrix& current, const assets::Matrix& parentWorld);
    // Moves the child by `delta` in world space (a platform node move that writes the child's own node): the local translation changes by delta turned into the parent's frame.
    bool translate(const std::string& lowerChild, const float delta[3], const assets::Matrix& parentWorld);
    const Link* find(const std::string& lowerChild) const;
    const std::map<std::string, Link>& links() const { return links_; }

private:
    std::map<std::string, Link> links_;
};

}
