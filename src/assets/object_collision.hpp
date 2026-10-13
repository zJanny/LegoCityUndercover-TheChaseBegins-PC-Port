#pragma once
// Collision of the objects a level places (props, lamp posts, signs, crates): which objects register collision
// with the world, the shape each one registers and its node matrix. Rules and evidence: docs/re/collision.md.
// No SDL dependency.
#include "assets/assets.hpp"
#include <memory>

namespace chase::assets {

// One collision entry the original registers in its world grid for a placed object (0x30e0d8 with 0x23aff8).
struct ObjectCollision {
    std::string instance;
    std::string className;
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
// switches the box off.
std::vector<ObjectCollision> objectCollisions(FuseArchive& archive, const Level& level);

}
