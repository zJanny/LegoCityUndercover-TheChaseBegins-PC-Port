#pragma once
// Development log of where an attached prop is drawn: the child's position in its parent's frame (the translation of the child's world matrix
// relative to the parent's, docs/re/props-rocking-attach.md) the first time the parent's drawn node has moved more than a metre from where it
// was first seen. A child that follows its parent keeps the local position of the attach; one that stays at its placement does not. No SDL dependency.
#include "assets/assets.hpp"

#include <array>
#include <map>
#include <optional>
#include <set>
#include <string>

namespace chase::runtime {

constexpr float kAttachedDrawnLogDistance = 1.0f;

class AttachedDrawnLog {
public:
    std::optional<std::array<float, 3>> observe(const std::string& child, const assets::Matrix& childWorld, const assets::Matrix& parentWorld);

private:
    std::map<std::string, std::array<float, 3>> firstSeen_;
    std::set<std::string> logged_;
};

}
