#include "runtime/collision_enable.hpp"

namespace chase::runtime {

void applyObjectCollisionState(CollisionWorld& world, const assets::PlacedObjectState& state) {
    for (CollisionWorld::Entry& entry : world.mutableEntries())
        if (!entry.owner.empty()) entry.active = !state.isDisabled(entry.owner) && state.collisionEnabled(entry.owner, entry.collisionDefault);
}

}
