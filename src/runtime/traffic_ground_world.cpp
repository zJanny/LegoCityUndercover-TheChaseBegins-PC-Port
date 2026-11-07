#include "runtime/traffic_ground_world.hpp"

#include "runtime/camera_reset_probe.hpp"

#include <cmath>
#include <cstring>
#include <stdexcept>

namespace chase::runtime {

namespace {

float dot3(const float a[3], const float b[3]) {
    float sum = a[0] * b[0];
    sum = sum + a[1] * b[1];
    sum = sum + a[2] * b[2];
    return sum;
}

void cross3(const float a[3], const float b[3], float out[3]) {
    float x = a[1] * b[2];
    x = x - a[2] * b[1];
    float y = a[2] * b[0];
    y = y - a[0] * b[2];
    float z = a[0] * b[1];
    z = z - a[1] * b[0];
    out[0] = x;
    out[1] = y;
    out[2] = z;
}

std::int32_t bitsOf(float value) {
    std::int32_t bits;
    static_assert(sizeof bits == sizeof value);
    std::memcpy(&bits, &value, sizeof bits);
    return bits;
}

// 0x270534: the segment against a node box, only x and z of the start and the y range of the segment.
bool segmentMeetsNode(const float from[3], const float to[3], const assets::CollisionNode& node) {
    const float dx = from[0] - node.centre[0];
    const float dy = from[1] - node.centre[1];
    const float dz = from[2] - node.centre[2];
    if (std::fabs(dx) > node.half[0]) return false;
    if (std::fabs(dz) > node.half[2]) return false;
    const float half = node.half[1];
    const float end = to[1] - node.centre[1];
    if (dy < -half) {
        if (-half > end) return false;
    }
    if (dy <= half) return true;
    return half >= end;
}

struct VerticalWalk {
    const assets::CollisionItem& item;
    const float* from;
    const float* to;
    float directionY;
    float length;
    const assets::CollisionTriangle* hitTriangle = nullptr;
    float hitDistance = 0.0f;

    const float* vertex(std::uint32_t index) const { return &item.vertices[std::size_t(index) * 3]; }

    bool triangle(const assets::CollisionTriangle& record) {
        const float* v0 = vertex(record.vertex[0]);
        const float* v1 = vertex(record.vertex[1]);
        const float* v2 = vertex(record.vertex[2]);
        float e1[3], e2[3];
        for (int a = 0; a < 3; ++a) {
            e1[a] = v1[a] - v0[a];
            e2[a] = v2[a] - v0[a];
        }
        float p[3];
        p[0] = directionY * e2[2];
        p[1] = 0.0f;
        p[2] = -(directionY * e2[0]);
        const float determinant = dot3(e1, p);
        if (bitsOf(determinant) < bitsOf(kTrafficGroundDeterminantMinimum)) return false;
        float origin[3];
        for (int a = 0; a < 3; ++a) origin[a] = from[a] - v0[a];
        const float u = dot3(origin, p);
        if (u < 0.0f) return false;
        if (u > determinant) return false;
        float q[3];
        cross3(origin, e1, q);
        const float v = directionY * q[1];
        if (v < 0.0f) return false;
        if (u + v > determinant) return false;
        const float t = dot3(e2, q) / determinant;
        if (t < 0.0f) return false;
        if (t > length) return false;
        hitTriangle = &record;
        hitDistance = t;
        return true;
    }

    bool visit(std::uint32_t index) {
        const assets::CollisionNode& node = item.nodes[index];
        if (!segmentMeetsNode(from, to, node)) return false;
        if (node.leaf()) {
            for (std::uint32_t i = 0; i < node.count(); ++i)
                if (triangle(item.triangles[node.first() + i])) return true;
            return false;
        }
        for (std::uint32_t i = 0; i < node.count(); ++i)
            if (visit(node.first() + i)) return true;
        return false;
    }
};

TrafficGroundMesh meshOf(const assets::CollisionItem& item) {
    TrafficGroundMesh mesh;
    const std::vector<std::uint8_t>& table = item.colourBlock.empty() ? item.flagBlock : item.colourBlock;
    std::uint32_t flag = 0;
    for (std::size_t i = 0; i < 4 && i < item.flagBlock.size(); ++i) flag |= std::uint32_t(item.flagBlock[i]) << (8 * i);
    mesh.hasColours = flag != 0;
    if (mesh.hasColours) {
        for (std::size_t i = 0; i + 4 <= table.size(); i += 4)
            mesh.colours.push_back(std::uint32_t(table[i]) | std::uint32_t(table[i + 1]) << 8 | std::uint32_t(table[i + 2]) << 16 | std::uint32_t(table[i + 3]) << 24);
    }
    return mesh;
}

void toBoxSpace(const CollisionWorld::Entry& entry, const float p[3], float out[3]) {
    const float d[3] = {p[0] - entry.matrix[12], p[1] - entry.matrix[13], p[2] - entry.matrix[14]};
    for (int i = 0; i < 3; ++i) out[i] = d[0] * entry.matrix[std::size_t(i) * 4] + d[1] * entry.matrix[std::size_t(i) * 4 + 1] + d[2] * entry.matrix[std::size_t(i) * 4 + 2];
}

}

std::optional<TrafficGroundHit> castTrafficGround(const CollisionWorld& world, const TrafficGroundSegment& segment) {
    if (segment.from[0] != segment.to[0] || segment.from[2] != segment.to[2])
        throw std::invalid_argument("castTrafficGround: STAND-IN TRAFFIC-S23, the segment is not vertical");
    const float directionY = segment.to[1] > segment.from[1] ? 1.0f : -1.0f;
    const float length = directionY > 0.0f ? segment.to[1] - segment.from[1] : segment.from[1] - segment.to[1];
    for (const CollisionWorld::Entry& entry : world.entries()) {
        if (!entry.active || entry.item.nodes.empty() || !kTrafficGroundLayer.accepts(entry.mask)) continue;
        if (entry.box) {
            float start[3], end[3], fraction;
            toBoxSpace(entry, segment.from.data(), start);
            toBoxSpace(entry, segment.to.data(), end);
            if (segmentMeetsBox(start, end, entry.centre, entry.half, fraction)) return std::nullopt;
            continue;
        }
        VerticalWalk walk{entry.item, segment.from.data(), segment.to.data(), directionY, length};
        if (!walk.visit(0)) continue;
        const assets::CollisionTriangle& record = *walk.hitTriangle;
        TrafficGroundHit hit;
        for (int a = 0; a < 3; ++a) hit.triangle.normal[std::size_t(a)] = record.normal[a];
        for (std::size_t k = 0; k < 3; ++k)
            for (std::size_t a = 0; a < 3; ++a) hit.triangle.corners[k][a] = entry.item.vertices[std::size_t(record.vertex[k]) * 3 + a];
        for (std::size_t k = 0; k < 3; ++k) hit.triangle.colourIndex[k] = record.unknown[k];
        hit.mesh = meshOf(entry.item);
        hit.point = segment.from;
        hit.point[1] = hit.point[1] + directionY * walk.hitDistance;
        return hit;
    }
    return std::nullopt;
}

void applyTrafficColourMessage(TrafficNodeTint& node, std::uint32_t colour) {
    node.bytes = {static_cast<std::uint8_t>(colour & 0xffu), static_cast<std::uint8_t>((colour >> 8) & 0xffu), static_cast<std::uint8_t>((colour >> 16) & 0xffu)};
}

TrafficGroundWorld trafficGroundWorld(const CollisionWorld& world, std::function<void(std::uint32_t colour)> sendColour) {
    TrafficGroundWorld result;
    result.cast = [&world](const TrafficGroundSegment& segment) { return castTrafficGround(world, segment); };
    result.sendColour = std::move(sendColour);
    return result;
}

}
