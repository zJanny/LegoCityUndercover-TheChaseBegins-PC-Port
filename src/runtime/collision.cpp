#include "runtime/collision.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

namespace chase::runtime {
namespace {
using Triangle = CollisionWorld::Triangle;

float dot3(const float a[3], const float b[3]) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }

bool separatedOnAxis(const float axis[3], const float centred[3][3], const float half[3]) {
    float low = dot3(axis, centred[0]), high = low;
    for (int k = 1; k < 3; ++k) {
        const float p = dot3(axis, centred[k]);
        low = std::min(low, p);
        high = std::max(high, p);
    }
    const float radius = std::fabs(axis[0]) * half[0] + std::fabs(axis[1]) * half[1] + std::fabs(axis[2]) * half[2];
    return low > radius || high < -radius;
}

// 0x257ea0: the length is a vsqrt of the sum of squares (unfused), a length at or below 0x34000000 gives (0, 0, 1),
// else the vector is multiplied by the reciprocal of the length.
void normalise(float v[3]) {
    const float length = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    std::uint32_t bits;
    std::memcpy(&bits, &length, sizeof bits);
    if (bits <= 0x34000000u) {
        v[0] = 0;
        v[1] = 0;
        v[2] = 1;
        return;
    }
    const float inverse = 1.0f / length;
    v[0] *= inverse;
    v[1] *= inverse;
    v[2] *= inverse;
}

void flattenAndNormalise(float normal[3]) {
    normal[1] = 0;
    normalise(normal);
}

bool anyOverlap(const std::vector<Triangle>& triangles, const float centre[3], const float half[3]) {
    return std::any_of(triangles.begin(), triangles.end(), [&](const Triangle& t) {
        return boxOverlapsTriangle(centre, half, t.vertex, t.record->normal, t.record->d);
    });
}

// 0x2023e4: halve the box height until the half height is below kBisectionStop, moving up when the upper
// half touches a triangle, else down when the lower half does; nothing in either half returns the box top.
float bisect(const float centre[3], const float half[3], std::vector<Triangle> triangles) {
    const float quarter = half[1] * 0.5f + 1.1920929e-07f;
    if (quarter < kBisectionStop) return centre[1];
    float nextCentre[3] = {centre[0], centre[1], centre[2]};
    const float nextHalf[3] = {half[0], quarter, half[2]};
    nextCentre[1] = centre[1] + quarter;
    if (anyOverlap(triangles, nextCentre, nextHalf)) return bisect(nextCentre, nextHalf, std::move(triangles));
    nextCentre[1] = centre[1] - quarter;
    triangles.erase(std::remove_if(triangles.begin(), triangles.end(), [&](const Triangle& t) {
                        return !boxOverlapsTriangle(nextCentre, nextHalf, t.vertex, t.record->normal, t.record->d);
                    }), triangles.end());
    if (triangles.empty()) return centre[1] + half[1];
    return bisect(nextCentre, nextHalf, std::move(triangles));
}
}

std::size_t CollisionWorld::triangleCount() const {
    std::size_t count = 0;
    for (const Entry& entry : entries_) count += entry.item.triangles.size();
    return count;
}

namespace {
void transformPoint(const assets::Matrix& m, const float in[3], float out[3]) {
    for (int a = 0; a < 3; ++a) out[a] = in[0] * m[std::size_t(a)] + in[1] * m[4 + std::size_t(a)] + in[2] * m[8 + std::size_t(a)] + m[12 + std::size_t(a)];
}

void rotateVector(const assets::Matrix& m, const float in[3], float out[3]) {
    for (int a = 0; a < 3; ++a) out[a] = in[0] * m[std::size_t(a)] + in[1] * m[4 + std::size_t(a)] + in[2] * m[8 + std::size_t(a)];
}

// 0x21ba50: the 8 corners (bit 0 x, bit 1 y, bit 2 z; set means plus) and the 12 triangles of an oriented box with the
// vertex order DAT_00376930 and the face normals DAT_00376954.
constexpr std::uint8_t kBoxTriangles[12][3] = {{0, 2, 1}, {3, 1, 2}, {4, 5, 6}, {7, 6, 5}, {0, 4, 2}, {6, 2, 4},
                                               {1, 3, 5}, {7, 5, 3}, {0, 1, 4}, {5, 4, 1}, {2, 6, 3}, {7, 3, 6}};
constexpr float kBoxNormals[12][3] = {{0, 0, -1}, {0, 0, -1}, {0, 0, 1}, {0, 0, 1}, {-1, 0, 0}, {-1, 0, 0},
                                      {1, 0, 0},  {1, 0, 0},  {0, -1, 0}, {0, -1, 0}, {0, 1, 0}, {0, 1, 0}};

assets::CollisionItem boxItem(const assets::Matrix& matrix, const float centre[3], const float half[3]) {
    assets::CollisionItem item;
    item.name = "Box";
    item.vertices.resize(24);
    for (int corner = 0; corner < 8; ++corner) {
        const float local[3] = {centre[0] + ((corner & 1) ? half[0] : -half[0]), centre[1] + ((corner & 2) ? half[1] : -half[1]),
                                centre[2] + ((corner & 4) ? half[2] : -half[2])};
        transformPoint(matrix, local, &item.vertices[std::size_t(corner) * 3]);
    }
    item.triangles.resize(12);
    for (std::size_t t = 0; t < 12; ++t) {
        assets::CollisionTriangle& triangle = item.triangles[t];
        for (int k = 0; k < 3; ++k) triangle.vertex[k] = kBoxTriangles[t][k];
        rotateVector(matrix, kBoxNormals[t], triangle.normal);
        const float* first = &item.vertices[std::size_t(triangle.vertex[0]) * 3];
        triangle.d = dot3(first, triangle.normal);
    }
    assets::CollisionNode root;
    float low[3], high[3];
    for (int a = 0; a < 3; ++a) {
        low[a] = high[a] = item.vertices[std::size_t(a)];
        for (int corner = 1; corner < 8; ++corner) {
            low[a] = std::min(low[a], item.vertices[std::size_t(corner) * 3 + std::size_t(a)]);
            high[a] = std::max(high[a], item.vertices[std::size_t(corner) * 3 + std::size_t(a)]);
        }
        root.centre[a] = (low[a] + high[a]) * 0.5f;
        root.half[a] = (high[a] - low[a]) * 0.5f;
    }
    root.info = (0u << 5) | (12u << 1) | 1u;
    item.nodes.push_back(root);
    return item;
}

assets::CollisionItem movedItem(const assets::CollisionItem& local, const assets::Matrix& matrix) {
    assets::CollisionItem item = local;
    for (std::size_t v = 0; v + 2 < item.vertices.size(); v += 3) {
        const float in[3] = {local.vertices[v], local.vertices[v + 1], local.vertices[v + 2]};
        transformPoint(matrix, in, &item.vertices[v]);
    }
    for (assets::CollisionTriangle& triangle : item.triangles) {
        const float in[3] = {triangle.normal[0], triangle.normal[1], triangle.normal[2]};
        rotateVector(matrix, in, triangle.normal);
        triangle.d = dot3(&item.vertices[std::size_t(triangle.vertex[0]) * 3], triangle.normal);
    }
    for (assets::CollisionNode& node : item.nodes) {
        const float centre[3] = {node.centre[0], node.centre[1], node.centre[2]};
        const float half[3] = {node.half[0], node.half[1], node.half[2]};
        transformPoint(matrix, centre, node.centre);
        for (int a = 0; a < 3; ++a)
            node.half[a] = std::fabs(matrix[std::size_t(a)]) * half[0] + std::fabs(matrix[4 + std::size_t(a)]) * half[1] +
                           std::fabs(matrix[8 + std::size_t(a)]) * half[2];
    }
    return item;
}
}

void CollisionWorld::addObject(const assets::ObjectCollision& object) {
    Entry entry;
    entry.mask = object.mask;
    if (object.mesh) {
        entry.item = movedItem(*object.mesh, object.matrix);
    } else {
        entry.item = boxItem(object.matrix, object.centre, object.half);
        entry.box = true;
        entry.matrix = object.matrix;
        std::copy(object.centre, object.centre + 3, entry.centre);
        std::copy(object.half, object.half + 3, entry.half);
    }
    entries_.push_back(std::move(entry));
}

bool boxOverlapsTriangle(const float centre[3], const float half[3], const float vertex[3][3], const float normal[3], float d) {
    const float planeRadius = std::fabs(normal[0]) * half[0] + std::fabs(normal[1]) * half[1] + std::fabs(normal[2]) * half[2];
    if (std::fabs(dot3(normal, centre) - d) > planeRadius) return false;
    float centred[3][3];
    for (int k = 0; k < 3; ++k)
        for (int a = 0; a < 3; ++a) centred[k][a] = vertex[k][a] - centre[a];
    for (int a = 0; a < 3; ++a) {
        const float low = std::min({centred[0][a], centred[1][a], centred[2][a]});
        const float high = std::max({centred[0][a], centred[1][a], centred[2][a]});
        if (low > half[a] || high < -half[a]) return false;
    }
    for (int e = 0; e < 3; ++e) {
        float edge[3];
        for (int a = 0; a < 3; ++a) edge[a] = centred[(e + 1) % 3][a] - centred[e][a];
        normalise(edge);
        for (int a = 0; a < 3; ++a) {
            float unit[3] = {0, 0, 0};
            unit[a] = 1;
            const float axis[3] = {unit[1] * edge[2] - unit[2] * edge[1], unit[2] * edge[0] - unit[0] * edge[2],
                                   unit[0] * edge[1] - unit[1] * edge[0]};
            if (axis[0] == 0 && axis[1] == 0 && axis[2] == 0) continue;
            if (separatedOnAxis(axis, centred, half)) return false;
        }
    }
    return true;
}

std::optional<float> groundHeight(const CollisionWorld& world, const GroundProbe& probe, const PlayerBox& box) {
    const float slopeScale = std::sqrt(1.0f - kFloorNormalMin * kFloorNormalMin);
    // 0x22f134 for a character of type 27: probe start above the node, raised by the previous movement
    // when the step is longer than 1/30 s.
    float lift = box.centre[1] - box.half[1];
    const float stretch = std::max(probe.dt * 30.0f - 1.0f, 0.0f);
    lift += probe.rootDelta * slopeScale * stretch;
    const float startY = probe.y + lift;
    // 0x20cd30: distance below the node = movement * sin(max slope) - 0.1, at least 0x21df6c (0.05).
    const float distance = probe.distance ? *probe.distance : std::max(probe.rootDelta * slopeScale - 0.1f, kGroundMinimumDistance);
    const float endY = probe.y - kGroundStepDown - distance;

    // 0x1f9884: the footprint shrinks by the margins, becomes the world box of the rotated box, clamped
    // to the longer side.
    const float extentX = box.half[0] - kGroundMargin, extentZ = box.half[2] - kGroundMargin;
    const float reach = std::max(extentX, extentZ);
    const float cosine = probe.forwardZ, sine = probe.forwardX;
    float footX = std::fabs(cosine) * extentX + std::fabs(sine) * extentZ;
    float footZ = std::fabs(sine) * extentX + std::fabs(cosine) * extentZ;
    footX = reach <= footX ? reach : footX;
    footZ = reach <= footZ ? reach : footZ;
    const float halfHeight = (startY - endY) * 0.5f;
    const float centre[3] = {probe.x + probe.forwardX * box.centre[2], startY - halfHeight, probe.z + probe.forwardZ * box.centre[2]};
    const float half[3] = {footX, halfHeight, footZ};

    std::vector<Triangle> candidates;
    const float low[3] = {centre[0] - half[0], centre[1] - half[1], centre[2] - half[2]};
    const float high[3] = {centre[0] + half[0], centre[1] + half[1], centre[2] + half[2]};
    world.query(low, high, kPlayerLayer, [&](const Triangle& t) {
        if (boxOverlapsTriangle(centre, half, t.vertex, t.record->normal, t.record->d)) candidates.push_back(t);
    });
    // 0x2ce21c: only floor-like triangles, with the looser limit when none is steep enough to be a wall-free floor.
    auto keep = [&](float limit) {
        std::vector<Triangle> floors;
        for (const Triangle& t : candidates)
            if (limit < t.record->normal[1]) floors.push_back(t);
        return floors;
    };
    std::vector<Triangle> floors = keep(kFloorNormalMin);
    if (floors.empty()) floors = keep(kFloorNormalMinLoose);
    if (floors.empty()) return std::nullopt;
    const float ground = bisect(centre, half, std::move(floors));
    return std::fabs(probe.y - ground) < kGroundKeepTolerance ? probe.y : ground;
}

namespace {
struct WallPass {
    int overlaps = 0;                          // triangles the oriented box overlapped (0x2092d8 result)
    std::vector<std::array<float, 3>> normals; // candidates: floor-excluded, facing the movement, inside the world box
};

// One run of 0x208d84 with flags 0x6c / 3: candidates are the triangles touching the world box of the oriented
// box at the destination, steeper than a floor and facing the movement; every candidate the oriented box overlaps
// slides the movement, in order.
WallPass wallPass(const CollisionWorld& world, const PlayerBox& box, const float node[3], const float right[3],
                  const float up[3], const float forward[3], float movement[3]) {
    const float destination[3] = {node[0] + movement[0], node[1] + movement[1], node[2] + movement[2]};
    float centre[3], reach[3];
    for (int a = 0; a < 3; ++a) {
        centre[a] = destination[a] + box.centre[0] * right[a] + box.centre[1] * up[a] + box.centre[2] * forward[a];
        reach[a] = std::fabs(right[a]) * box.half[0] + std::fabs(up[a]) * box.half[1] + std::fabs(forward[a]) * box.half[2];
    }
    const float low[3] = {centre[0] - reach[0], centre[1] - reach[1], centre[2] - reach[2]};
    const float high[3] = {centre[0] + reach[0], centre[1] + reach[1], centre[2] + reach[2]};
    const float requested[3] = {movement[0], movement[1], movement[2]};
    WallPass pass;
    world.query(low, high, kPlayerLayer, [&](const Triangle& t) {
        const float* normal = t.record->normal;
        if (normal[1] > kFloorNormalMin) return;
        if (t.record->surface < 32 && ((kPlayerExcludedSurfaces >> t.record->surface) & 1)) return;
        if (dot3(normal, requested) >= 0) return;
        bool touches = true;
        for (int a = 0; a < 3; ++a) {
            const float lo = std::min({t.vertex[0][a], t.vertex[1][a], t.vertex[2][a]});
            const float hi = std::max({t.vertex[0][a], t.vertex[1][a], t.vertex[2][a]});
            touches = touches && lo <= high[a] && hi >= low[a];
        }
        if (!touches) return;
        pass.normals.push_back({normal[0], normal[1], normal[2]});
        float local[3][3];
        for (int k = 0; k < 3; ++k) {
            float relative[3];
            for (int a = 0; a < 3; ++a) relative[a] = t.vertex[k][a] - destination[a];
            local[k][0] = dot3(right, relative);
            local[k][1] = dot3(up, relative);
            local[k][2] = dot3(forward, relative);
        }
        const float localNormal[3] = {dot3(right, normal), dot3(up, normal), dot3(forward, normal)};
        const float localD = t.record->d - dot3(normal, destination);
        if (!boxOverlapsTriangle(box.centre, box.half, local, localNormal, localD)) return;
        ++pass.overlaps;
        float slide[3] = {normal[0], normal[1], normal[2]};
        if (std::fabs(slide[1]) < 1.0f) flattenAndNormalise(slide);
        else slide[1] = 0;
        const float along = slide[0] * movement[0] + slide[2] * movement[2];
        if (along < 0) {
            movement[0] -= slide[0] * along;
            movement[2] -= slide[2] * along;
        }
    });
    return pass;
}
}

int slideAlongWalls(const CollisionWorld& world, float x, float y, float z, float forwardX, float forwardZ,
                    float& dx, float& dz, const PlayerBox& box) {
    // Node axes as rows: right (c, 0, -s), up (0, 1, 0), forward (s, 0, c).
    const float sine = forwardX, cosine = forwardZ;
    const float right[3] = {cosine, 0, -sine}, up[3] = {0, 1, 0}, forward[3] = {sine, 0, cosine};
    const float node[3] = {x, y, z};
    float movement[3] = {dx, 0, dz};
    const WallPass first = wallPass(world, box, node, right, up, forward, movement);
    if (first.overlaps > 0) {
        // 0x208d84 with flag 0x10 (the player: 0x245b00 is set): a second pass at the slid destination. If it still
        // touches a wall and one of its walls meets a wall of the first pass at an angle with a negative cosine
        // above -1, the character is in a corner and the movement is removed.
        const WallPass second = wallPass(world, box, node, right, up, forward, movement);
        if (second.overlaps > 0) {
            bool corner = false;
            for (const auto& after : second.normals)
                for (const auto& before : first.normals) {
                    const float cosineBetween = after[0] * before[0] + after[1] * before[1] + after[2] * before[2];
                    corner = corner || (cosineBetween < 0 && cosineBetween > -1.0f);
                }
            if (corner) {
                movement[0] = 0;
                movement[2] = 0;
            }
        }
    }
    dx = movement[0];
    dz = movement[2];
    return first.overlaps;
}

}
