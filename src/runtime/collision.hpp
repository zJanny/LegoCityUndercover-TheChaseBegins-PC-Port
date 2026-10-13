#pragma once
// Static level collision for the player: ground height and wall sliding, replicating the original's
// queries 0x1f9884 (box sweep for the ground, bisection 0x2023e4) and 0x21a73c / 0x208d84 / 0x2092d8
// (oriented box against wall triangles, slide response). Data format and evidence: docs/re/collision.md.
// No SDL dependency.
#include "assets/assets.hpp"
#include <cstdint>
#include <optional>
#include <vector>

namespace chase::runtime {

// Triangles of a level's collision items (CollisionItem of the master model), queried through the
// bounding-box trees stored in the data.
class CollisionWorld {
public:
    void add(const assets::CollisionItem& item) { items_.push_back(item); }
    bool empty() const { return items_.empty(); }
    const std::vector<assets::CollisionItem>& items() const { return items_; }
    std::size_t triangleCount() const;

    struct Triangle {
        const assets::CollisionTriangle* record;
        float vertex[3][3];
    };
    // Calls visit(Triangle) for every triangle whose tree leaf overlaps the box, items in order and
    // the tree depth first. A superset of the triangles that touch the box.
    template <class Visit> void query(const float low[3], const float high[3], Visit&& visit) const {
        for (const assets::CollisionItem& item : items_)
            if (!item.nodes.empty()) walk(item, 0, low, high, visit);
    }

private:
    template <class Visit>
    static void walk(const assets::CollisionItem& item, std::uint32_t index, const float low[3], const float high[3],
                     Visit& visit) {
        const assets::CollisionNode& node = item.nodes[index];
        for (int a = 0; a < 3; ++a)
            if (node.centre[a] - node.half[a] > high[a] || node.centre[a] + node.half[a] < low[a]) return;
        if (!node.leaf()) {
            for (std::uint32_t i = 0; i < node.count(); ++i) walk(item, node.first() + i, low, high, visit);
            return;
        }
        for (std::uint32_t i = 0; i < node.count(); ++i) {
            Triangle triangle;
            triangle.record = &item.triangles[node.first() + i];
            for (int k = 0; k < 3; ++k)
                for (int a = 0; a < 3; ++a) triangle.vertex[k][a] = item.vertices[std::size_t(triangle.record->vertex[k]) * 3 + std::size_t(a)];
            visit(triangle);
        }
    }

    std::vector<assets::CollisionItem> items_;
};

// The player's collision box: entity+0x60 (centre, in node space) and entity+0x6c (half extents), read
// from the player entity of the gameplay checkpoint (docs/re/collision.md). PROVISIONAL for other
// characters and states.
struct PlayerBox {
    float centre[3] = {0.0f, 1.15f, 0.08f};
    float half[3] = {0.6f, 0.85f, 0.52f};
};

// Constants of the ground probe 0x1f9884 / 0x22f134 / 0x20b48c (floats from the game's data at the
// addresses noted in docs/re/collision.md).
constexpr float kFloorNormalMin = 0x1.37af86p-1f;       // *0x376910, 0x3f1bd7c3
constexpr float kFloorNormalMinLoose = 0x1.64fca4p-4f;  // *0x376914, 0x3db27e52
constexpr float kGroundMargin = 0x1.eb851ep-3f;            // *0x37713c, *0x377140: footprint shrink per axis
constexpr float kGroundStepDown = 0.1f;           // literal in 0x22f134
constexpr float kGroundMinimumDistance = 0.05f;   // *0x3771ac, 0x21df6c
constexpr float kGroundKeepTolerance = 0x1.47ae16p-7f;     // 0x3c23d70b in 0x1f9884: smaller changes keep the height
constexpr float kBisectionStop = 0x1.47ae14p-8f;          // 0x3ba3d70a in 0x2023e4
constexpr std::uint32_t kPlayerExcludedSurfaces = 0x80000;  // low exclusion mask passed by 0x20cd30 to 0x21a73c

struct GroundProbe {
    float x = 0, y = 0, z = 0;       // node position after this update's horizontal move
    float forwardX = 0, forwardZ = 1;  // node forward axis (sin h, cos h)
    float rootDelta = 0;             // length of this update's movement before collision (0x20e968 on the root delta)
    float dt = 0;                    // frame step, 0x23602c
    std::optional<float> distance;   // distance below the node to probe; the airborne call of 0x20b48c passes max(|fall speed|, 0.05)
};

// Ground height for the node at (x, y, z): nothing when no floor-like triangle is in reach (the original
// then lets the character fall, which is not replicated), else the new node height. The height only
// changes when the bisected ground differs from y by at least kGroundKeepTolerance.
std::optional<float> groundHeight(const CollisionWorld& world, const GroundProbe& probe, const PlayerBox& box = {});

// Slides the movement (dx, dz) of a node at (x, y, z) heading (forwardX, forwardZ) along wall triangles
// touched by the box at the destination (two passes, a corner removes the movement). Returns the number of wall
// triangles the box overlapped in the first pass.
int slideAlongWalls(const CollisionWorld& world, float x, float y, float z, float forwardX, float forwardZ,
                    float& dx, float& dz, const PlayerBox& box = {});

// Akenine-Moller box/triangle overlap as in 0x20c2b0 (plane, three box axes, nine edge axes).
bool boxOverlapsTriangle(const float centre[3], const float half[3], const float vertex[3][3], const float normal[3], float d);

}
