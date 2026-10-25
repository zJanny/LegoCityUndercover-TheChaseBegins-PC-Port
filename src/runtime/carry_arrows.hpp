#pragma once
// The arrows of the carry system (docs/re/carrying.md): the tail of the carry object update 0x16d420 and the carry target update 0x19cf5c put
// the entity into the carry group (type 2, key: the carry-it waypoint) and the carry arrow list 2, or take it out, and the use hint 0x1cba8c
// hides the arrow of the carry object or target the A press would use (0x2b9c28). No SDL dependency.
#include "assets/object_collision.hpp"
#include "runtime/carry.hpp"
#include "runtime/script_runner.hpp"
#include <string>
#include <vector>

namespace chase::runtime {

struct CarryArrowRequest {
    std::string instance;  // lower case
    bool enable = false;
};

struct CarryArrowUpdate {
    std::vector<CarryArrowRequest> requests;
    std::string hide;  // lower case, empty for none
};

// One update. A carry object whose use zone is usable (UseStartActive, record +4 bit 4) leaves the group and the list while it is carried and
// joins them while it is free, unless it is a FreeplayObject; a usable zone of a carried object that is not free and an unusable zone change
// nothing. A carry target joins while the player carries its UseObject (ctrl+0x13c) and leaves otherwise.
// STAND-IN LEVOBJ-S99: only enabled objects and targets make requests (whether a disabled entity's update runs was not read; 0x2428c0 calls
// 0x22d5f0 on entity +0x40), and the hide is asked for the object or target of CarrySet::useAt while `hintActive` (the gate of the use hint's
// caller is not read; chase-game passes the A press gate).
CarryArrowUpdate carryArrows(const CarrySet& set, const assets::PlacedObjectState& state, const float player[3], bool hintActive);

void applyCarryArrows(const CarryArrowUpdate& update, ScriptRunner& scripts);

}
