#include "runtime/node_attach.hpp"

namespace chase::runtime {

assets::Matrix relativeToParent(const assets::Matrix& world, const assets::Matrix& parentWorld) {
    assets::Matrix inverse{};
    for (int row = 0; row < 3; ++row)
        for (int column = 0; column < 3; ++column) inverse[column * 4 + row] = parentWorld[row * 4 + column];
    for (int column = 0; column < 3; ++column) {
        float value = -(inverse[column] * parentWorld[12]);
        value -= inverse[4 + column] * parentWorld[13];
        value -= inverse[8 + column] * parentWorld[14];
        inverse[12 + column] = value;
    }
    inverse[15] = 1.0f;
    return assets::multiply(world, inverse);
}

assets::Matrix childWorld(const assets::Matrix& local, const assets::Matrix& parentWorld) { return assets::multiply(local, parentWorld); }

void NodeAttachments::attach(const std::string& child, const std::string& parent, const assets::Matrix& world, const assets::Matrix& parentWorld) {
    links_[child] = Link{parent, relativeToParent(world, parentWorld), world};
}

void NodeAttachments::detach(const std::string& child) { links_.erase(child); }

bool NodeAttachments::setWorld(const std::string& child, const assets::Matrix& world, const assets::Matrix& parentWorld) {
    const auto found = links_.find(child);
    if (found == links_.end()) return false;
    found->second.local = relativeToParent(world, parentWorld);
    found->second.world = world;
    return true;
}

std::optional<assets::Matrix> NodeAttachments::follow(const std::string& child, const assets::Matrix& current, const assets::Matrix& parentWorld) {
    const auto found = links_.find(child);
    if (found == links_.end()) return std::nullopt;
    Link& link = found->second;
    if (current != link.world) link.local = relativeToParent(current, parentWorld);
    link.world = childWorld(link.local, parentWorld);
    return link.world;
}

bool NodeAttachments::translate(const std::string& child, const float delta[3], const assets::Matrix& parentWorld) {
    const auto found = links_.find(child);
    if (found == links_.end()) return false;
    for (int column = 0; column < 3; ++column)
        for (int row = 0; row < 3; ++row) found->second.local[12 + column] += delta[row] * parentWorld[column * 4 + row];
    return true;
}

const NodeAttachments::Link* NodeAttachments::find(const std::string& child) const {
    const auto found = links_.find(child);
    return found == links_.end() ? nullptr : &found->second;
}

}
