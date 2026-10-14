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

}
