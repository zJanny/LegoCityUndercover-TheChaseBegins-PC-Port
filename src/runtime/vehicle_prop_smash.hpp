#pragma once
// The smash of destructible props by a moving vehicle (docs/re/vehicles-physics-props.md, entry 2026-10-16T10:5x). Every update the vehicle's
// update 0x16ce9c calls 0x275c00 when its rigid body is dynamic (rigid flags +0xcc bit 1 clear) and has a non-zero velocity: the world grid
// 0x578dfc is queried (0x237f30, mode 1) with the world box of the vehicle node's bounds (node+0xa0 / +0xac), each entry of mask 4 or 6 whose box
// overlaps passes 0x2c02c8 (the entity box +0x60 / +0x6c of the vehicle against the target's), and a target without physics body that is
// no character gets message 0 with damage 100 (the vehicle as sender) from 0x275e74. The wobbly prop's handler 0x2a3390 subtracts the damage from its
// health halfword (record +0x10, the `WobbleHealth` parameter) and for a health at or below 0 destroys the entity (0x23cd80). No SDL dependency.
#include "assets/assets.hpp"
#include "assets/object_collision.hpp"
#include "runtime/prop_destroy.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

struct SmashBox {
    std::array<float, 3> centre{};
    std::array<float, 3> half{};
};

// 0x22ec3c: the world box (centre and half extent) of a box in node space under a node matrix (row vectors, translation last). A matrix whose
// first and sixth element exceed 0.99900 (0x3f7fbe77) keeps the half extent as it is.
SmashBox worldSmashBox(const assets::Matrix& node, const SmashBox& local);
// 0x2335e4: per axis |centre difference| <= sum of the half extents.
bool smashBoxesOverlap(const SmashBox& a, const SmashBox& b);

struct SmashingVehicle {
    assets::Matrix node{};       // the vehicle node's world matrix (0x250810)
    SmashBox nodeBounds;         // node+0xa0 / +0xac: the union of the model's mesh boxes (modelBounds; the oracle's trooper node equals it)
    SmashBox entityBox;          // entity+0x60 / +0x6c: the collision box (modelCollisionBox)
    float velocity[3] = {0, 0, 0};  // rigid body +0x13c
    bool dynamicBody = true;     // rigid flags +0xcc bit 1 clear
};

struct SmashableProp {
    assets::Matrix node{};       // the prop node's world matrix
    SmashBox entityBox;          // entity+0x60 / +0x6c
    std::optional<SmashBox> meshRoot;  // grid entry kind 1 (entity+0x78): the shape is the root node of the collision item, in the model's space
    std::int16_t health = 0;     // WobbleHealth (record +0x10 of 0x2a3390, +0x110 of 0x1bc38c); 1 for the classes that any damage destroys
};

constexpr std::int16_t kVehicleSmashDamage = 100;

// The vehicle pass runs (0x16ce9c: dynamic body and a velocity of non-zero length; the states of a kinematic body are not ported).
bool vehicleSmashPassRuns(const SmashingVehicle& vehicle);
// The grid query of 0x275c00 followed by the filter 0x2c02c8: the vehicle's node bounds overlap the prop's grid shape under its node matrix (the entity
// box of a box entry, the collision item's root node of a mesh entry: 0x237f30 mode 1), and the vehicle's world entity box, moved into the prop's frame (0x237eb4, inverse rotation), overlaps the prop's entity box.
bool vehicleSmashesProp(const SmashingVehicle& vehicle, const SmashableProp& prop);
// The wobbly handler 0x2a3390 for message 0 with damage 100: the new health and whether the entity is destroyed.
struct SmashOutcome {
    std::int16_t health = 0;
    bool destroyed = false;
};
SmashOutcome applySmashDamage(std::int16_t health);


// The props of a level whose message 0 handler destroys them at health 0 or on any damage: leprops/leprop_wobblydestructible (type 119, 0x2a3390) and
// props/prop_trafficlight (type 204, 0x1bc38c) with their WobbleHealth, props/prop_respawnabledestructible (type 214, 0x2b3fcc) and
// leprops/leprop_simpledestructible (type 85) through the base handler 0x1f8948 (damage not 0). STAND-IN VEH-S105: an object with other classes is
// not smashed (the hit sequences of type 67, 0x15e408, are not ported); a wobbly prop with a fall-apart clip is destroyed at once instead of
// playing it. The destroyed entity is reported by name and the caller treats it as disabled (the original deletes it, 0x23cd80).
struct SmashedProp {
    std::string instance;
    SmashableProp prop;
    PropDestroyEffects effects;  // message 1 of the destroyed entity (0x1f8948, 0x2aa5c8)
};

class VehiclePropSmasher {
public:
    void load(const assets::Level& level, const assets::ClassLookup& classOf, const std::vector<assets::ObjectCollision>& placed);
    // Runs the query for one vehicle; returns the instances destroyed by it. Props whose entity is disabled or without collision flag 0x200 are
    // not found by the query layer.
    std::vector<SmashedProp> smash(const SmashingVehicle& vehicle, const assets::PlacedObjectState& state);
    std::size_t propCount() const;
    // Adds the objects that scripts destroy and whose kind has ported message 1 effects (destroyEffectsPorted) but that have no collision entry
    // (props/prop_prop, prop_platform, ability_rocksmash, ...): the node matrix is the placement's, the entity box is the model's bounds
    // (`boundsOf`, 0x242eb0). STAND-IN SCRIPT-S251: the entity box (+0x60 / +0x6c) of these classes is set by constructors that are not read.
    using BoundsOf = std::function<std::optional<SmashBox>(const assets::Placement&)>;
    void loadScriptDestroyed(const assets::Level& level, const assets::ClassLookup& classOf, const std::vector<assets::Placement>& placements, const BoundsOf& boundsOf);
    // The node, entity box and message 1 effects of a prop whose destroy message has ported effects (destroyEffectsPorted: the smashable classes,
    // props/ability_dynamitedestructible, which only scripts and the player's dynamite destroy, and the kinds 64, 67, 71, 132, 204 that scripts
    // destroy); null for any other instance and for objects without a collision entry (no entity box: STAND-IN SCRIPT-S251).
    std::optional<SmashedProp> destroyData(const std::string& lowerInstance) const;

private:
    struct Entry {
        std::string instance;
        bool collisionDefault = true;
        bool smashable = true;
        SmashableProp prop;
        PropDestroyEffects effects;
        bool destroyed = false;
    };
    std::vector<Entry> props_;
};

}
