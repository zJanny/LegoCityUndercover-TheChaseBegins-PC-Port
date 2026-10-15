#pragma once
// Which placed objects the collision queries accept while playing. The original lists an entity in its world grid once and the query
// layers test the owner's flags when they run (bit 0 set by EnableObject 0, flag 0x200 set and cleared by EnableCollision), so a change
// of the script state only changes those flags. Evidence: docs/re/collision.md. No SDL dependency.
#include "assets/object_collision.hpp"
#include "runtime/collision.hpp"

namespace chase::runtime {

void applyObjectCollisionState(CollisionWorld& world, const assets::PlacedObjectState& state);

}
