#include "runtime/attach_children.hpp"

#include <algorithm>

namespace chase::runtime {

bool AttachedChildren::attach(const std::string& child, const std::string& parent, std::uint32_t sequence, const assets::Matrix& childWorld,
                              const assets::Matrix& parentWorld) {
    const auto taken = sequences_.find(child);
    const NodeAttachments::Link* link = nodes_.find(child);
    if (link && taken != sequences_.end() && taken->second == sequence && link->parent == parent) return false;
    nodes_.attach(child, parent, childWorld, parentWorld);
    sequences_[child] = sequence;
    return true;
}

void AttachedChildren::detach(const std::string& child) {
    nodes_.detach(child);
    sequences_.erase(child);
}

std::vector<std::pair<std::string, assets::Matrix>> AttachedChildren::follow(const WorldOf& worldOf) {
    std::vector<std::pair<std::size_t, std::string>> order;
    for (const auto& [child, link] : nodes_.links()) {
        std::size_t depth = 0;
        std::string parent = link.parent;
        while (depth <= nodes_.links().size()) {
            const NodeAttachments::Link* up = nodes_.find(parent);
            if (!up) break;
            parent = up->parent;
            ++depth;
        }
        order.emplace_back(depth, child);
    }
    std::stable_sort(order.begin(), order.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    std::map<std::string, assets::Matrix> written;
    const auto current = [&](const std::string& name) -> std::optional<assets::Matrix> {
        const auto done = written.find(name);
        return done != written.end() ? std::optional<assets::Matrix>(done->second) : worldOf(name);
    };
    std::vector<std::pair<std::string, assets::Matrix>> changed;
    for (const auto& [depth, child] : order) {
        const NodeAttachments::Link* link = nodes_.find(child);
        const std::optional<assets::Matrix> parentWorld = current(link->parent);
        const std::optional<assets::Matrix> world = current(child);
        if (!parentWorld || !world) continue;
        const std::optional<assets::Matrix> followed = nodes_.follow(child, *world, *parentWorld);
        if (!followed) continue;
        written[child] = *followed;
        if (*followed != *world) changed.emplace_back(child, *followed);
    }
    return changed;
}

}
