#pragma once
// Collision of the objects a level places (props, lamp posts, signs, crates): which objects register collision
// with the world, the shape each one registers and its node matrix. Rules and evidence: docs/re/collision.md.
// No SDL dependency.
#include "assets/assets.hpp"
#include <map>
#include <memory>
#include <set>
#include <string>

namespace chase::assets {

// State of the placed objects at the start of a level, by lower-case instance name: entity flag bit 0 (disabled, set by
// EnableObject with 0 and not registered with the queries) and flag 0x200 (collision, EnableCollision) overrides.
// `destroyed` holds the entities DestroyProp / KillObject and the destroy of a prop 0x23cd80 reached (flag 0x10 of +0xc set):
// isDisabled counts them (SCRIPT-304). EnableObject clears only `disabled` (0x242cf0 does not test the destroyed bit), so a destroyed entity that
// the destroy also hid is drawn again after EnableObject, still without collision; isHidden is the draw test.
struct PlacedObjectState {
    std::set<std::string> disabled;
    std::set<std::string> destroyed;
    std::map<std::string, bool> collisionOverride;

    bool isDisabled(const std::string& instance) const;
    bool isHidden(const std::string& instance) const;
    bool collisionEnabled(const std::string& instance, bool initial) const;
};

// The model bounds 0x242eb0 (union of the mesh boxes of the nodes that are not NoBounds); false when the model has none.
bool modelBounds(FuseArchive& archive, const std::string& modelPath, float centre[3], float half[3]);
// Bounding box of the root node of the model's first collision item. It is the entity box (+0x60 / +0x6c) of vehicles: the oracle values of the
// police trooper and the worker equal it to four decimals (docs/re/vehicles-boarding.md, entry 2026-10-15T08:15:49). False when the model has no collision item.
bool modelCollisionBox(FuseArchive& archive, const std::string& modelPath, float centre[3], float half[3]);

// One collision entry the original registers in its world grid for a placed object (0x30e0d8 with 0x23aff8).
struct ObjectCollision {
    std::string instance;
    std::string className;
    bool collisionDefault = true;                 // the object's BoundCollision parameter: entity flag 0x200 before any EnableCollision
    std::string model;                            // meshModelPath of the object's model, empty without one
    std::uint32_t mask = 6;                       // entry mask word: 6 for objects with the dynamic flag 0x100, which every prop has
    Matrix matrix{};                              // node world matrix of the object (row vectors, translation last)
    std::shared_ptr<const CollisionItem> mesh;    // first collision item of the model (entity+0x78), null for a box
    float centre[3] = {0, 0, 0};                  // box in node space (entity+0x60), used when mesh is null
    float half[3] = {0, 0, 0};                    // box half extents (entity+0x6c)
};

// Objects of the level that register collision, for the classes whose constructor rule is recovered:
//  - every class with a "CollisionBound" record in its class file's section +0x24 (centre and half extents),
//    when the class is a prop ("props/", "leprops/");
//  - the prop classes "leprops/leprop_*", "props/prop_respawnabledestructible" and "props/prop_door" and "leprops/leprop_door" (whose automatic box
//    is narrowed by 0.24 on x and z): the first collision item of the
//    model when the object's "MeshCollision" parameter is set, else a box (the model's collision box record when
//    MeshCollision is set and the model has no item, else the union of the model's mesh bounds);
//  - "dummy/dummycollision": a box with the half extents of the instance's "CollisionBound" parameter.
// Characters, vehicles and classes with other constructors are not included. A "BoundCollision" parameter of 0
// clears the collision flag 0x200 of the object unless `state` sets it again; disabled objects are left out.
std::vector<ObjectCollision> objectCollisions(FuseArchive& archive, const Level& level, const PlacedObjectState& state);

// Every object the rules above cover, whatever its enable state. The original lists an entity in its world grid once (0x2ba3f4
// returns when it is already listed) and the query layers test the owner's flags (bit 0 disabled, 0x200 collision) when they
// run, so EnableObject and EnableCollision during play only change those flags (docs/re/collision.md).
std::vector<ObjectCollision> objectCollisionCandidates(FuseArchive& archive, const Level& level);

}
