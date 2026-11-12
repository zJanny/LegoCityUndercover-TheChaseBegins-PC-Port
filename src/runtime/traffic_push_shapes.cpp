#include "runtime/traffic_push_shapes.hpp"

#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string>

namespace chase::runtime {
namespace {

constexpr std::size_t kWorldCandidateCap = 300;          // 0x208f2c / 0x208fc0: the list 0x56d578 holds 300 entries
constexpr std::uint32_t kCornerCosineBits = 0xbf800000u; // 0x2092d4: a cosine whose bits are below -1.0's bits
static_assert(sizeof(assets::CollisionTriangle) == 32, "0x250e20 compares the 32-byte triangle records");

struct Candidate {
    const CollisionWorld::Entry* entry = nullptr;
    const assets::CollisionTriangle* record = nullptr;   // null for a box entry: the list entry's triangle is null
    std::size_t index = 0;
};

std::uint32_t bitsOf(float value) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof bits);
    return bits;
}

float dot(const float* a, const float* b) {
    float sum = a[0] * b[0];
    sum += a[1] * b[1];
    sum += a[2] * b[2];
    return sum;
}

AiVec3 transformPoint(const AiVec3& p, const AiMatrix& m) {
    AiVec3 out{};
    for (int axis = 0; axis < 3; ++axis) out[axis] = (p[1] * m[1][axis] + p[2] * m[2][axis]) + (m[3][axis] + p[0] * m[0][axis]);
    return out;
}

AiVec3 intoFrame(const AiVec3& p, const AiMatrix& m) {
    const float dx = p[0] - m[3][0];
    const float dy = p[1] - m[3][1];
    const float dz = p[2] - m[3][2];
    AiVec3 out{};
    for (int row = 0; row < 3; ++row) {
        float sum = dx * m[row][0];
        sum += dy * m[row][1];
        sum += dz * m[row][2];
        out[row] = sum;
    }
    return out;
}

AiMatrix multiply(const AiMatrix& a, const AiMatrix& b) {
    AiMatrix out{};
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 3; ++column) {
            float sum = a[row][0] * b[0][column];
            sum += a[row][1] * b[1][column];
            sum += a[row][2] * b[2][column];
            if (row == 3) sum += b[3][column];
            out[row][column] = sum;
        }
    }
    return out;
}

AiVec3 unitMove(const AiVec3& move) {
    float sum = move[0] * move[0];
    sum += move[1] * move[1];
    sum += move[2] * move[2];
    const float length = std::sqrt(sum);
    if (bitsOf(length) <= 0x34000000u) return {0.0f, 0.0f, 1.0f};
    const float inverse = 1.0f / length;
    return {move[0] * inverse, move[1] * inverse, move[2] * inverse};
}

bool nodeOverlaps(const assets::CollisionNode& node, const AiVec3& low, const AiVec3& high) {
    for (int a = 0; a < 3; ++a)
        if (node.centre[a] - node.half[a] > high[a] || node.centre[a] + node.half[a] < low[a]) return false;
    return true;
}

void walkItem(const CollisionWorld::Entry& entry, std::uint32_t index, const AiVec3& low, const AiVec3& high, std::uint32_t excludedSurfaces,
              std::vector<Candidate>& list) {
    const assets::CollisionItem& item = entry.item;
    const assets::CollisionNode& node = item.nodes[index];
    if (!nodeOverlaps(node, low, high)) return;
    if (!node.leaf()) {
        for (std::uint32_t i = 0; i < node.count(); ++i) walkItem(entry, node.first() + i, low, high, excludedSurfaces, list);
        return;
    }
    for (std::uint32_t i = 0; i < node.count() && list.size() < kWorldCandidateCap; ++i) {
        const std::size_t t = node.first() + i;
        const assets::CollisionTriangle& record = item.triangles[t];
        if (record.surface < 32 && ((excludedSurfaces >> record.surface) & 1u) != 0) continue;
        list.push_back({&entry, &record, t});
    }
}

void collectEntry(const CollisionWorld::Entry& entry, const AiVec3& low, const AiVec3& high, std::uint32_t excludedSurfaces,
                  std::vector<Candidate>& list) {
    if (entry.item.nodes.empty() || list.size() >= kWorldCandidateCap) return;
    if (entry.box) {
        if (nodeOverlaps(entry.item.nodes[0], low, high)) list.push_back({&entry, nullptr, 0});
        return;
    }
    walkItem(entry, 0, low, high, excludedSurfaces, list);
}

const assets::CollisionTriangle& storedRecord(const Candidate& c) {
    const std::vector<assets::CollisionTriangle>& triangles = c.entry->localMesh ? c.entry->localMesh->triangles : c.entry->item.triangles;
    return triangles[c.index];
}

const void* recordSource(const Candidate& c) {
    if (c.entry->localMesh) return c.entry->localMesh.get();
    return &c.entry->item;
}

bool sameRecord(const Candidate& a, const Candidate& b) {
    return recordSource(a) == recordSource(b) && std::memcmp(&storedRecord(a), &storedRecord(b), sizeof(assets::CollisionTriangle)) == 0;
}

void filterCandidates(std::vector<Candidate>& list, const AiVec3& reference, const AiVec3& move, std::uint32_t flags, float normalLimit,
                      float facingLimit) {
    AiVec3 facing = move;
    if ((flags & kWorldFilterUnitMove) != 0) facing = unitMove(move);
    for (std::size_t i = 0; i < list.size();) {
        const Candidate& c = list[i];
        bool drop = false;
        if (c.record) {
            if ((flags & kWorldFilterDuplicates) != 0)
                for (std::size_t j = 0; j < i && !drop; ++j) drop = list[j].record && sameRecord(c, list[j]);
            const float* normal = c.record->normal;
            if (!drop && (flags & kWorldFilterNormalY) != 0)
                drop = (flags & kWorldFilterKeepFloors) != 0 ? normal[1] <= normalLimit : normal[1] > normalLimit;
            if (!drop && (flags & kWorldFilterFacing) != 0) drop = dot(normal, facing.data()) >= facingLimit;
            // STAND-IN LEVOBJ-S81: hword +0x14 of the owner is taken as set for the placed objects (mask other than the static level's).
            const bool sideTest = (flags & kWorldFilterSide) != 0 || ((flags & kWorldFilterSideMoving) != 0 && c.entry->mask != kStaticMask);
            if (!drop && sideTest) drop = !(c.record->d <= dot(reference.data(), normal));
        }
        if (drop) list.erase(list.begin() + static_cast<std::ptrdiff_t>(i));
        else ++i;
    }
}

bool triangleOverlaps(const float vertex[3][3], const float normal[3], float d, const AiVec3& centre, const AiVec3& half,
                      const AiVec3& translation, bool identity) {
    if (identity) return boxOverlapsTriangle(centre.data(), half.data(), vertex, normal, d);
    float moved[3][3];
    for (int k = 0; k < 3; ++k)
        for (int a = 0; a < 3; ++a) moved[k][a] = vertex[k][a] - translation[a];
    return boxOverlapsTriangle(centre.data(), half.data(), moved, normal, dot(normal, moved[0]));
}

void slideAlong(const float triangleNormal[3], AiVec3& move, bool flatten) {
    float normal[3] = {triangleNormal[0], triangleNormal[1], triangleNormal[2]};
    if (flatten) {
        const float y = normal[1];
        if (bitsOf(std::fabs(y)) < 0x3f800000u) {
            normal[1] = 0.0f;
            normaliseVector(normal);
            normal[1] = y;
        }
    }
    AiVec3 slid = move;
    const float along = dot(normal, slid.data());
    if (along < 0.0f)
        for (int a = 0; a < 3; ++a) slid[a] = slid[a] - normal[a] * along;
    move[0] = slid[0];
    move[2] = slid[2];
    if (!flatten) move[1] = slid[1];
}

void triangleVertices(const assets::CollisionItem& item, const assets::CollisionTriangle& record, float vertex[3][3]) {
    for (int k = 0; k < 3; ++k)
        for (int a = 0; a < 3; ++a) vertex[k][a] = item.vertices[std::size_t(record.vertex[k]) * 3 + std::size_t(a)];
}

unsigned respond(const std::vector<Candidate>& list, const AiVec3& centre, const AiVec3& half, const AiVec3& translation, AiVec3* move,
                 std::uint32_t mode) {
    if ((mode & kWorldDropMisses) != 0) throw std::logic_error("traffic push shapes: 0x2092d8 with mode 8 is not ported here");
    const bool identity = std::fabs(translation[0]) <= kWorldIdentityTolerance && std::fabs(translation[1]) <= kWorldIdentityTolerance &&
                          std::fabs(translation[2]) <= kWorldIdentityTolerance;
    const bool slides = (mode & kWorldSlide) != 0 && move;
    const bool flatten = (mode & kWorldSlideFlatten) != 0;
    unsigned count = 0;
    for (const Candidate& c : list) {
        const assets::CollisionItem& item = c.entry->item;
        bool hit = false;
        if (c.record) {
            float vertex[3][3];
            triangleVertices(item, *c.record, vertex);
            hit = triangleOverlaps(vertex, c.record->normal, c.record->d, centre, half, translation, identity);
            if (hit && slides) slideAlong(c.record->normal, *move, flatten);
        } else {
            for (const assets::CollisionTriangle& record : item.triangles) {
                float vertex[3][3];
                triangleVertices(item, record, vertex);
                if (!triangleOverlaps(vertex, record.normal, record.d, centre, half, translation, identity)) continue;
                hit = true;
                if (slides) slideAlong(record.normal, *move, flatten);
            }
        }
        if (!hit) continue;
        ++count;
        if ((mode & kWorldSlide) == 0) break;
    }
    return count;
}

bool cornerBetween(const std::vector<Candidate>& second, const std::vector<Candidate>& first) {
    for (const Candidate& after : second) {
        if (!after.record) continue;
        for (const Candidate& before : first) {
            if (!before.record) continue;
            const float cosine = dot(after.record->normal, before.record->normal);
            if (cosine < 0.0f && bitsOf(cosine) < kCornerCosineBits) return true;
        }
    }
    return false;
}

using Collect = std::function<void(const AiVec3& low, const AiVec3& high, std::uint32_t excludedSurfaces, std::vector<Candidate>& list)>;

std::uint8_t listResult(std::size_t count) { return count == 0 ? 1 : static_cast<std::uint8_t>(count & 0xffu); }

std::uint8_t queryAndRespond(const Collect& collect, const AiVec3& centre, const AiVec3& half, AiVec3* move, std::uint32_t filter,
                             std::uint32_t mode, float normalLimit, float facingLimit, std::uint32_t excludedSurfaces,
                             std::vector<Candidate>& list) {
    const AiVec3 translation = move ? *move : AiVec3{0.0f, 0.0f, 0.0f};
    const AiVec3 destination = {centre[0] + translation[0], centre[1] + translation[1], centre[2] + translation[2]};
    const AiVec3 low = {destination[0] - half[0], destination[1] - half[1], destination[2] - half[2]};
    const AiVec3 high = {destination[0] + half[0], destination[1] + half[1], destination[2] + half[2]};
    list.clear();
    collect(low, high, excludedSurfaces, list);
    if (!list.empty() && filter != 0) filterCandidates(list, centre, translation, filter | kWorldFilterSideMoving, normalLimit, facingLimit);
    if (list.empty()) return 0;
    const std::uint8_t result = static_cast<std::uint8_t>(respond(list, centre, half, translation, move, mode) & 0xffu);
    if (!move || result == 0 || (mode & kWorldCornerPass) == 0) return result;
    const std::vector<Candidate> first = list;
    std::vector<Candidate> second;
    if (queryAndRespond(collect, centre, half, move, filter, mode & ~kWorldCornerPass, normalLimit, facingLimit, 0, second) == 0) {
        list = first;
        return listResult(list.size());
    }
    if (cornerBetween(second, first)) {
        (*move)[0] = 0.0f;
        (*move)[2] = 0.0f;
    }
    list = second;
    return listResult(list.size());
}

bool insideOpen(float value, float half) { return value > -half && value < half; }

AiVec3 scaled(const AiVec3& row, float factor) { return {row[0] * factor, row[1] * factor, row[2] * factor}; }

void addTo(AiVec3& move, const AiVec3& delta) {
    for (int a = 0; a < 3; ++a) move[a] = move[a] + delta[a];
}

template <typename Function>
const Function& need(const Function& function, const char* name) {
    if (!function) throw std::logic_error(std::string("traffic push shapes: missing hook ") + name);
    return function;
}

}

std::uint8_t slideWorldBox(const CollisionWorld& world, const WorldBoxSlide& slide, AiVec3& move) {
    const Collect collect = [&world](const AiVec3& low, const AiVec3& high, std::uint32_t excludedSurfaces, std::vector<Candidate>& list) {
        for (const CollisionWorld::Entry& entry : world.entries())
            if (entry.active && kPlayerLayer.accepts(entry.mask)) collectEntry(entry, low, high, excludedSurfaces, list);
    };
    std::vector<Candidate> list;
    return queryAndRespond(collect, slide.centre, slide.half, &move, slide.filter, slide.mode, slide.normalLimit, slide.facingLimit,
                           slide.excludedSurfaces, list);
}

bool meshEntryPush(const CollisionWorld::Entry& mesh, const TrafficWorldBox& moverBox, AiVec3* move, std::uint32_t filter, std::uint32_t mode,
                   float normalLimit) {
    const Collect collect = [&mesh](const AiVec3& low, const AiVec3& high, std::uint32_t, std::vector<Candidate>& list) {
        collectEntry(mesh, low, high, 0, list);
    };
    std::vector<Candidate> list;
    return queryAndRespond(collect, moverBox.centre, moverBox.half, move, filter, mode, normalLimit, 0.0f, 0, list) != 0;
}

bool orientedBoxPush(const CollisionWorld& world, const AiVec3& moverHalf, AiVec3& move, const AiVec3& boxCentre, const AiVec3& boxHalf,
                     const AiMatrix& matrix, const AiVec3& moverCentre, const AiVec3& movedCentre, bool slide, float normalLimit) {
    AiVec3 q = intoFrame(movedCentre, matrix);
    for (int a = 0; a < 3; ++a) q[a] = q[a] - boxCentre[a];
    AiVec3 reach{};
    for (int a = 0; a < 3; ++a) reach[a] = boxHalf[a] + moverHalf[a];
    for (int a = 0; a < 3; ++a)
        if (!insideOpen(q[a], reach[a])) return false;
    if (!slide) return true;

    AiVec3 start = intoFrame(moverCentre, matrix);
    for (int a = 0; a < 3; ++a) start[a] = start[a] - boxCentre[a];
    bool startedInside = true;
    for (int a = 0; a < 3; ++a) startedInside = startedInside && start[a] > -reach[a] && start[a] < reach[a];

    WorldBoxSlide worldSlide;
    worldSlide.centre = moverCentre;
    worldSlide.half = moverHalf;
    worldSlide.filter = kOrientedSlideFilter;
    worldSlide.normalLimit = normalLimit;
    if (startedInside) {
        const float depthX = reach[0] - std::fabs(q[0]);
        const float depthZ = reach[2] - std::fabs(q[2]);
        const bool upright = bitsOf(std::fabs(matrix[1][1])) > bitsOf(kOrientedUprightLimit);
        int axis = 0;
        if (depthX < depthZ) axis = (!upright && !(depthX < reach[1] - std::fabs(q[1]))) ? 1 : 0;
        else axis = (!upright && depthZ >= reach[1] - std::fabs(q[1])) ? 1 : 2;
        const float out = q[axis] < 0.0f ? -reach[axis] - q[axis] : reach[axis] - q[axis];
        const AiVec3 push = scaled(matrix[axis], out);
        const float pushX = std::fabs(push[0]);
        const float pushY = std::fabs(push[1]);
        if (pushY < pushX || std::fabs(push[2]) > pushY) addTo(move, push);
        worldSlide.mode = kOrientedSlideInsideMode;
    } else {
        for (int a = 0; a < 3; ++a) {
            if (!(start[a] < reach[a])) addTo(move, scaled(matrix[a], (reach[a] - q[a]) + kOrientedPushOut));
            if (!(start[a] > -reach[a])) addTo(move, scaled(matrix[a], (-reach[a] - q[a]) - kOrientedPushOut));
        }
        worldSlide.mode = kOrientedSlideOutsideMode;
    }
    slideWorldBox(world, worldSlide, move);
    return true;
}

void connectTrafficPushShapes(TrafficPushShapes& shapes, const TrafficPushShapeWorld& world) {
    shapes.meshPush = [&shapes, &world](std::uint32_t mover, const AiVec3& move, const TrafficContactEntry& entry) {
        const CollisionWorld::Entry* mesh = need(world.meshOf, "meshOf")(entry);
        if (!mesh) throw std::logic_error("traffic push shapes: a mesh entry without its collision item");
        const TrafficPushBox box = need(shapes.box, "box")(mover);
        const TrafficWorldBox moverBox = trafficShapeWorldBox(need(shapes.nodeMatrix, "nodeMatrix")(mover), box.centre, box.half);
        AiVec3 changed = move;
        return meshEntryPush(*mesh, moverBox, &changed, kMeshPushFilter, kMeshPushMode, shapes.floorNormalMin);
    };
    shapes.orientedPush = [&shapes, &world](std::uint32_t mover, const AiVec3& move, const TrafficContactEntry& entry, const AiMatrix& matrix,
                                            const TrafficPushBox& box, const AiVec3& moverCentre, const AiVec3& movedCentre, bool slide) {
        if (!world.world) throw std::logic_error("traffic push shapes: no collision world");
        const AiVec3 moverHalf = need(shapes.box, "box")(mover).half;
        AiVec3 changed = move;
        if ((need(shapes.flags, "flags")(entry.owner) & kEntityFlagPushParts) == 0)
            return orientedBoxPush(*world.world, moverHalf, changed, box.centre, box.half, matrix, moverCentre, movedCentre, slide,
                                   shapes.floorNormalMin);
        const std::optional<std::vector<TrafficPushPart>> parts = need(world.parts, "parts")(entry.owner);
        if (!parts) return false;
        for (const TrafficPushPart& part : *parts)
            if (orientedBoxPush(*world.world, moverHalf, changed, part.box.centre, part.box.half, multiply(part.matrix, matrix), moverCentre,
                                movedCentre, slide, shapes.floorNormalMin))
                return true;
        return false;
    };
    shapes.slideMove = [&shapes, &world](std::uint32_t entity, AiVec3& move, std::uint32_t flags, float normalLimit) {
        if (!world.world) throw std::logic_error("traffic push shapes: no collision world");
        const TrafficPushBox box = need(shapes.box, "box")(entity);
        WorldBoxSlide slide;
        slide.centre = transformPoint(box.centre, need(shapes.nodeMatrix, "nodeMatrix")(entity));
        slide.half = box.half;
        slide.filter = flags;
        slide.mode = kPushSlideMode;
        slide.normalLimit = normalLimit;
        slideWorldBox(*world.world, slide, move);
    };
}

}
