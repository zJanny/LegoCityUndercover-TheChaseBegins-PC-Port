#include "runtime/camera_reset_probe.hpp"
#include <algorithm>
#include <cmath>

namespace chase::runtime {

namespace {
constexpr float kDeterminantMinimum = 1.0e-5f;  // 0x3727c5ac in 0x17e0f8

void cross(const float a[3], const float b[3], float out[3]) {
    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}

float dot(const float a[3], const float b[3]) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }

struct Segment {
    float from[3], to[3];
    float low[3], high[3];
    explicit Segment(const float f[3], const float t[3]) {
        for (int a = 0; a < 3; ++a) {
            from[a] = f[a];
            to[a] = t[a];
            low[a] = std::min(f[a], t[a]);
            high[a] = std::max(f[a], t[a]);
        }
    }
};

struct Ray {
    float direction[3];
    float length;
};

Ray rayOf(const Segment& segment) {
    Ray ray;
    if (segment.from[0] == segment.to[0] && segment.from[2] == segment.to[2]) {
        const bool up = segment.to[1] > segment.from[1];
        ray.direction[0] = 0.0f;
        ray.direction[1] = up ? 1.0f : -1.0f;
        ray.direction[2] = 0.0f;
        ray.length = up ? segment.to[1] - segment.from[1] : segment.from[1] - segment.to[1];
        return ray;
    }
    float delta[3] = {segment.to[0] - segment.from[0], segment.to[1] - segment.from[1], segment.to[2] - segment.from[2]};
    normaliseVector(delta);
    for (int a = 0; a < 3; ++a) ray.direction[a] = delta[a];
    const float dx = segment.to[0] - segment.from[0], dy = segment.to[1] - segment.from[1], dz = segment.to[2] - segment.from[2];
    ray.length = std::sqrt(dx * dx + dy * dy + dz * dz);
    return ray;
}

// Slab test of a segment against a box in its own space: the distance fraction of the entry point, 0 when the start is inside.
bool segmentMeetsBox(const float from[3], const float to[3], const float centre[3], const float half[3], float& entry) {
    float low = 0.0f, high = 1.0f;
    for (int a = 0; a < 3; ++a) {
        const float start = from[a] - centre[a], delta = to[a] - from[a];
        if (delta == 0.0f) {
            if (start <= -half[a] || start >= half[a]) return false;
            continue;
        }
        float near = (-half[a] - start) / delta, far = (half[a] - start) / delta;
        if (near > far) std::swap(near, far);
        low = std::max(low, near);
        high = std::min(high, far);
        if (low > high) return false;
    }
    entry = low;
    return true;
}

void toBoxSpace(const CollisionWorld::Entry& entry, const float p[3], float out[3]) {
    const float d[3] = {p[0] - entry.matrix[12], p[1] - entry.matrix[13], p[2] - entry.matrix[14]};
    for (int i = 0; i < 3; ++i) out[i] = d[0] * entry.matrix[std::size_t(i) * 4] + d[1] * entry.matrix[std::size_t(i) * 4 + 1] + d[2] * entry.matrix[std::size_t(i) * 4 + 2];
}

struct MeshWalk {
    const assets::CollisionItem& item;
    const Segment& segment;
    const float* direction;
    float& limit;
    bool hit = false;
    float point[3] = {};

    void visit(std::uint32_t index) {
        const assets::CollisionNode& node = item.nodes[index];
        for (int a = 0; a < 3; ++a)
            if (node.centre[a] - node.half[a] > segment.high[a] || node.centre[a] + node.half[a] < segment.low[a]) return;
        if (!node.leaf()) {
            for (std::uint32_t i = 0; i < node.count(); ++i) visit(node.first() + i);
            return;
        }
        for (std::uint32_t i = 0; i < node.count(); ++i) {
            const assets::CollisionTriangle& triangle = item.triangles[node.first() + i];
            if ((std::uint64_t(1) << triangle.surface) & kCameraExcludedSurfaces) continue;
            float v[3][3];
            for (int k = 0; k < 3; ++k)
                for (int a = 0; a < 3; ++a) v[k][a] = item.vertices[std::size_t(triangle.vertex[k]) * 3 + std::size_t(a)];
            const float t = rayTriangleDistance(v[0], v[1], v[2], segment.from, direction, limit);
            if (t < 0.0f) continue;
            hit = true;
            for (int a = 0; a < 3; ++a) point[a] = segment.from[a] + direction[a] * t;
            limit = t;
        }
    }
};
}

float rayTriangleDistance(const float v0[3], const float v1[3], const float v2[3], const float origin[3], const float direction[3], float limit) {
    float e1[3], e2[3], t0[3];
    for (int a = 0; a < 3; ++a) {
        e1[a] = v1[a] - v0[a];
        e2[a] = v2[a] - v0[a];
        t0[a] = origin[a] - v0[a];
    }
    float p[3];
    cross(direction, e2, p);
    const float determinant = dot(e1, p);
    if (determinant < kDeterminantMinimum) return -1.0f;
    const float u = dot(t0, p);
    if (u < 0.0f || u > determinant) return -1.0f;
    float q[3];
    cross(t0, e1, q);
    const float v = dot(direction, q);
    if (v < 0.0f || u + v > determinant) return -1.0f;
    const float t = dot(e2, q) / determinant;
    if (t < 0.0f || t > limit) return -1.0f;
    return t;
}

bool CameraResetProbe::segmentBlocked(const float from[3], const float to[3]) const {
    return nearestHit(from, to).has_value();
}

std::optional<std::array<float, 3>> CameraResetProbe::nearestHit(const float from[3], const float to[3]) const {
    const Segment segment(from, to);
    const Ray ray = rayOf(segment);
    float limit = ray.length;
    std::optional<std::array<float, 3>> result;
    for (const CollisionWorld::Entry& entry : world_.entries()) {
        if (!entry.active || entry.item.nodes.empty() || !kCameraLayer.accepts(entry.mask)) continue;
        if (!excludedOwner_.empty() && entry.owner == excludedOwner_) continue;
        if (entry.box) {
            float start[3], end[3], fraction;
            toBoxSpace(entry, from, start);
            toBoxSpace(entry, to, end);
            if (!segmentMeetsBox(start, end, entry.centre, entry.half, fraction)) continue;
            const float distance = fraction * ray.length;
            if (distance > limit) continue;
            limit = distance;
            std::array<float, 3> point;
            for (int a = 0; a < 3; ++a) point[std::size_t(a)] = from[a] + ray.direction[a] * distance;
            result = point;
            continue;
        }
        MeshWalk walk{entry.item, segment, ray.direction, limit};
        walk.visit(0);
        if (walk.hit) result = std::array<float, 3>{walk.point[0], walk.point[1], walk.point[2]};
    }
    return result;
}

}
