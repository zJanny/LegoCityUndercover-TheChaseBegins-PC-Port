#pragma once
// Which placed objects the original draws: an object that EnableObject (or the level start) disabled has its entity flag
// bit 0 set and its scene node hidden (0x2428c0 calls 0x23f734 with 0), so it is not drawn until EnableObject shows it
// again. Evidence: docs/re/objects.md. No SDL dependency.
#include "assets/assets.hpp"
#include "assets/object_collision.hpp"

#include <vector>

namespace chase::runtime {

// One flag per placement: true when the object is drawn.
std::vector<bool> drawnPlacements(const std::vector<assets::Placement>& placements, const assets::PlacedObjectState& state);

// One flag per placed character: a disabled character is hidden by the same 0x2428c0 as a disabled prop.
std::vector<bool> drawnCharacters(const std::vector<assets::CharacterObject>& characters, const assets::PlacedObjectState& state);

// 0x2428c0 recurses over the class sub-objects (0x1dd4f8) and hides each node: a nested object (wheel, handle, attachment) is drawn only while its owner is.
bool nestedObjectDrawn(bool ownerDrawn);

}
