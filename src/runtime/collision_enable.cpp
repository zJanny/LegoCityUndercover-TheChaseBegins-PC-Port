#include "runtime/collision_enable.hpp"

namespace chase::runtime {

namespace {
// STAND-IN COLL-S6: a nested object ("parent.child") counts as disabled while any ancestor is, as the drawn nested objects do (the enable and
// disable recursions 0x242cf0 / 0x242b08 / 0x1cfbbc over the descendants were not read to the end).
bool ancestorDisabled(const assets::PlacedObjectState& state, const std::string& owner) {
    for (std::size_t dot = owner.find('.'); dot != std::string::npos; dot = owner.find('.', dot + 1))
        if (state.isDisabled(owner.substr(0, dot))) return true;
    return false;
}
}

void applyObjectCollisionState(CollisionWorld& world, const assets::PlacedObjectState& state) {
    for (CollisionWorld::Entry& entry : world.mutableEntries())
        if (!entry.owner.empty())
            entry.active = !state.isDisabled(entry.owner) && !ancestorDisabled(state, entry.owner) && state.collisionEnabled(entry.owner, entry.collisionDefault);
}

}
