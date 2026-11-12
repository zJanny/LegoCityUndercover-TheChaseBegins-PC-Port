#pragma once
// The shape tests behind TrafficPushShapes (docs/re/traffic-car-push.md): the world slide 0x23d434 (0x208d84 without a collider: the query
// with the default descriptor 0x362298, the candidate filter 0x209a98, the response 0x2092d8 and the corner pass), the mesh test 0x287ac4
// (0x208d84 with the mesh entry as the collider) and the oriented test 0x27f174 with the part loop of 0x1f49d8. No SDL dependency.
#include "runtime/collision.hpp"
#include "runtime/traffic_character_gather.hpp"
#include "runtime/traffic_push_world.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace chase::runtime {

// The filter flags of 0x209a98 (0x208d84 adds 0x20 to the caller's flags).
constexpr std::uint32_t kWorldFilterUnitMove = 0x1;      // the facing test uses the normalised move (0x23b998)
constexpr std::uint32_t kWorldFilterKeepFloors = 0x2;    // with 0x4: drop normal y at or below the limit instead of above it
constexpr std::uint32_t kWorldFilterNormalY = 0x4;       // drop normal y above the limit (s0)
constexpr std::uint32_t kWorldFilterFacing = 0x8;        // drop normal . move at or above the facing limit (s1)
constexpr std::uint32_t kWorldFilterSide = 0x10;         // drop a triangle whose plane has the reference point behind it
constexpr std::uint32_t kWorldFilterSideMoving = 0x20;   // the same for an owner with hword +0x14 set
constexpr std::uint32_t kWorldFilterDuplicates = 0x40;   // drop a triangle whose 32-byte record equals an earlier candidate's (0x250e20)
// The modes of 0x2092d8 and 0x208d84.
constexpr std::uint32_t kWorldSlide = 0x1;               // every overlapping triangle slides the move (0x22f928); else the first overlap ends
constexpr std::uint32_t kWorldSlideFlatten = 0x2;        // the slide normal's x and z are normalised flat and the move's y is kept
constexpr std::uint32_t kWorldDropMisses = 0x8;          // a missed entry takes the list's last entry (the ground probe; not ported here)
constexpr std::uint32_t kWorldCornerPass = 0x10;         // 0x208d84: a second query at the slid move; a corner clears x and z

constexpr float kWorldIdentityTolerance = 0x1.a36e2ep-14f;   // 0x2096c4 (1e-4): 0x17e2fc compares the query frame with 0x52fce4
constexpr float kOrientedUprightLimit = 0x1.ccccccp-1f;   // 0x27f650 (0.9), compared as bits with |row 1 y|
constexpr float kOrientedPushOut = 0x1.cac084p-8f;           // 0x27f654 (0.007): the margin a start outside the box is pushed out by
constexpr std::uint32_t kOrientedSlideFilter = 92;           // 0x23d434(mover, move, 92, mode, *0x376910, 0.0)
constexpr std::uint32_t kOrientedSlideInsideMode = 1;        // 0x27f4b0: the mover started inside the box
constexpr std::uint32_t kOrientedSlideOutsideMode = 3;       // 0x27f620: the mover started outside on some axis
constexpr std::uint32_t kMeshPushFilter = 93;                // 0x1f4aa4
constexpr std::uint32_t kMeshPushMode = 3;                   // 0x1f4a94

// What 0x23d434(entity, move, filter, mode, s0, s1, surface mask) runs: 0x208d84 with the entity's box as {+0x60 times its node matrix,
// +0x6c} (the half extents are not rotated), no collider and no frame.
struct WorldBoxSlide {
    AiVec3 centre{};
    AiVec3 half{};
    std::uint32_t filter = 0;
    std::uint32_t mode = 0;
    float normalLimit = 0.0f;              // s0
    float facingLimit = 0.0f;              // s1
    std::uint32_t excludedSurfaces = 0;    // the low word of the surface mask on the stack; the corner pass passes 0
};

// 0x208d84 without a collider: the entries of the default descriptor's layer (kPlayerLayer) around the box at centre + move, filtered and
// answered in order. `move` changes in place. The result is the byte 0x208d84 returns: 0 without candidates, else the response's count.
std::uint8_t slideWorldBox(const CollisionWorld& world, const WorldBoxSlide& slide, AiVec3& move);

// 0x287ac4(mover, move, entry, filter, mode, s0, 0.0): 0x208d84 with `mesh` as the collider and the mover's world box 0x230088 (`moverBox`).
// `move` null is the pure turn call. True when 0x208d84 returns non-zero.
bool meshEntryPush(const CollisionWorld::Entry& mesh, const TrafficWorldBox& moverBox, AiVec3* move, std::uint32_t filter, std::uint32_t mode,
                   float normalLimit);

// 0x27f174(mover, move, box centre, box half, matrix, mover centre, moved centre, slide): the open test of the moved centre in the box's frame
// with the half extents grown by the mover's (+0x6c); with slide the move leaves the box and slides along the world (0x23d434 with the box of
// `moverCentre` and `moverHalf`).
bool orientedBoxPush(const CollisionWorld& world, const AiVec3& moverHalf, AiVec3& move, const AiVec3& boxCentre, const AiVec3& boxHalf,
                     const AiMatrix& matrix, const AiVec3& moverCentre, const AiVec3& movedCentre, bool slide, float normalLimit);

// One part of a pushed entity with flag 0x800: 0x236368(node, i) and the box of the i-th node entry (record +0x14 of the node's model +0xc0:
// box index hword +0x10 of the 24-byte entry at +0xc, boxes of 36 bytes at +0x10 with the centre at +8 and the half extents at +20).
struct TrafficPushPart {
    AiMatrix matrix{};
    TrafficPushBox box;
};

// The world the shape hooks read. Supplied by chase-game in TRAFFIC-56.
struct TrafficPushShapeWorld {
    const CollisionWorld* world = nullptr;
    std::function<const CollisionWorld::Entry*(const TrafficContactEntry& entry)> meshOf;              // entry +0x1c, the collision item
    // Nothing when the record +0xc0 of the owner's node is not of byte +8 = 2 or has no part record (0x1f4d60..0x1f4d74): no test runs.
    std::function<std::optional<std::vector<TrafficPushPart>>(std::uint32_t owner)> parts;
};

// Fills shapes.meshPush, shapes.orientedPush and shapes.slideMove. `shapes` and `world` must outlive the hooks.
// The mesh test and the oriented test change the caller's move (the slide along the mesh, the push out of the box, the world slide) through
// the `move` reference, as the original does through its pointer.
void connectTrafficPushShapes(TrafficPushShapes& shapes, const TrafficPushShapeWorld& world);

}
