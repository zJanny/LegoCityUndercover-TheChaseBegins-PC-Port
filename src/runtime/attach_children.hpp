#pragma once
// Placed objects under a placed parent node (docs/re/props-rocking.md, entry "Attached children take the parent's rotation"): AttachObject 0x1bf68c
// links the child's node under the parent's with 0x245008 each time it is called, so a repeated call takes the local matrix again from the
// child's world at that moment; every update the child's world is its local matrix times the parent's world, parents before their children.
// No SDL dependency.
#include "runtime/node_attach.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace chase::runtime {

class AttachedChildren {
public:
    using WorldOf = std::function<std::optional<assets::Matrix>(const std::string&)>;
    // `sequence` numbers the AttachObject calls. Returns false when this call was already taken (same child, same call).
    bool attach(const std::string& lowerChild, const std::string& lowerParent, std::uint32_t sequence, const assets::Matrix& childWorld,
                const assets::Matrix& parentWorld);
    void detach(const std::string& lowerChild);
    bool has(const std::string& lowerChild) const { return nodes_.find(lowerChild) != nullptr; }
    bool translate(const std::string& lowerChild, const float delta[3], const assets::Matrix& parentWorld) { return nodes_.translate(lowerChild, delta, parentWorld); }
    const NodeAttachments::Link* find(const std::string& lowerChild) const { return nodes_.find(lowerChild); }
    const std::map<std::string, NodeAttachments::Link>& links() const { return nodes_.links(); }
    // The new world of every attached child whose world changed, parents first. `worldOf` gives the current world of a node (a child's world
    // written by another writer since the last follow is kept relative to the parent first, as 0x24c118 does). A child without a parent world
    // keeps its place.
    std::vector<std::pair<std::string, assets::Matrix>> follow(const WorldOf& worldOf);

private:
    NodeAttachments nodes_;
    std::map<std::string, std::uint32_t> sequences_;
};

}
