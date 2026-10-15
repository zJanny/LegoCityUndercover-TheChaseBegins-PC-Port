#include "runtime/camera_sweep.hpp"
#include <algorithm>
#include <cmath>

namespace chase::runtime {

namespace {
constexpr std::size_t kMaxRawSegments = 200;
constexpr std::size_t kMaxIntervals = 50;
constexpr std::size_t kMaxTrianglesPerItem = 0x100;
constexpr float kEdgeTolerance = 0.001f;
constexpr float kIntervalMargin = 0.001f;
constexpr float kSmallObjectHalfSize = 1.0f;
constexpr float kSmallObjectDistance = 5.0f;

struct Interval {
    float low, high;
};

float dot3(const float a[3], const float b[3]) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }

void normalise3(float v[3]) {
    const float length = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (length <= 1.1920929e-7f) {
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 1.0f;
        return;
    }
    const float scale = 1.0f / length;
    v[0] *= scale;
    v[1] *= scale;
    v[2] *= scale;
}

void cross3(const float a[3], const float b[3], float out[3]) {
    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}

struct Clip {
    float px, py, nx, ny;
};

Clip makeClip(float apex, float normalX, float normalY) {
    const float length = std::sqrt(normalX * normalX + normalY * normalY);
    if (length <= 1.1920929e-7f) return {0.0f, apex, 0.0f, 1.0f};
    const float scale = 1.0f / length;
    return {0.0f, apex, normalX * scale, normalY * scale};
}

// 0x1eb780: keeps the part of the segment with (p - point) . normal <= 0.
bool clipSegment(CameraSweep::Segment& s, const Clip& line) {
    const float base = line.px * line.nx + line.py * line.ny;
    const float f0 = (s.x0 * line.nx + s.y0 * line.ny) - base;
    const float f1 = (s.x1 * line.nx + s.y1 * line.ny) - base;
    if (f0 <= 0.0f) {
        if (f1 > 0.0f) {
            const float t = f0 / (f0 - f1);
            s.x1 = (1.0f - t) * s.x0 + t * s.x1;
            s.y1 = (1.0f - t) * s.y0 + t * s.y1;
        }
        return true;
    }
    if (f1 > 0.0f) return false;
    const float t = f1 / (f1 - f0);
    s.x0 = (1.0f - t) * s.x1 + t * s.x0;
    s.y0 = (1.0f - t) * s.y1 + t * s.y0;
    return true;
}

// 0x1eb66c: sort by start and merge overlapping intervals.
void mergeIntervals(std::vector<Interval>& intervals) {
    if (intervals.size() < 2) return;
    std::stable_sort(intervals.begin(), intervals.end(), [](const Interval& a, const Interval& b) { return a.low < b.low; });
    std::size_t last = 0;
    for (std::size_t i = 1; i < intervals.size(); ++i) {
        if (intervals[last].high < intervals[i].low) {
            intervals[++last] = intervals[i];
        } else if (intervals[last].high <= intervals[i].high) {
            intervals[last].high = intervals[i].high;
        }
    }
    intervals.resize(last + 1);
}

// Tail of 0x2eafdc / 0x2d4e64: the widest gap between the merged intervals inside [-1, 1].
CameraSweepGap widestGap(const std::vector<Interval>& intervals) {
    CameraSweepGap gap;
    if (intervals.empty()) return gap;
    gap.low = 0.0f;
    gap.high = 0.0f;
    float best = 0.0f;
    float previousEnd = -1.0f;
    const std::size_t count = intervals.size();
    for (std::size_t i = 0; i <= count; ++i) {
        float nextStart = 1.0f;
        if (i < count && intervals[i].low < 1.0f) nextStart = intervals[i].low;
        if (best < nextStart - previousEnd) {
            gap.low = previousEnd;
            gap.high = nextStart;
            best = nextStart - previousEnd;
        }
        previousEnd = -1.0f;
        if (i + 1 <= count && intervals[i].high > -1.0f) previousEnd = intervals[i].high;
    }
    gap.middle = (gap.high + gap.low) * 0.5f;
    if (gap.low == -1.0f && gap.high < 1.0f) gap.middle = gap.high - 1.0f;
    if (gap.low != -1.0f && gap.high == 1.0f) gap.middle = gap.low + 1.0f;
    return gap;
}
}

bool CameraSweep::slice(SliceShape& shape, const float triangle[3][3]) const {
    if (shape.raw.size() >= kMaxRawSegments) return false;
    float x[3], d[3], y[3];
    for (int k = 0; k < 3; ++k) {
        float q[3] = {triangle[k][0] - origin_[0], triangle[k][1] - origin_[1], triangle[k][2] - origin_[2]};
        x[k] = dot3(q, shape.lateral);
        d[k] = dot3(q, shape.plane);
        y[k] = dot3(q, depth_);
    }
    const bool anyPositive = d[0] > 0.0f || d[1] > 0.0f || d[2] > 0.0f;
    const bool anyNonPositive = d[0] <= 0.0f || d[1] <= 0.0f || d[2] <= 0.0f;
    if (!anyPositive || !anyNonPositive) return false;
    float points[3][2];
    int count = 0;
    for (int e = 0; e < 3; ++e) {
        const int a = e, b = (e + 1) % 3;
        if ((d[a] > 0.0f && d[b] <= 0.0f) || (d[a] <= 0.0f && d[b] > 0.0f)) {
            const float t = d[b] / (d[b] - d[a]);
            points[count][0] = x[a] * t + (1.0f - t) * x[b];
            points[count][1] = y[a] * t + (1.0f - t) * y[b];
            ++count;
        }
    }
    if (count < 2) return false;
    shape.raw.push_back({points[0][0], points[0][1], points[1][0], points[1][1]});
    return true;
}

float CameraSweep::slabDepth(const float point[3]) const {
    const float q[3] = {point[0] - origin_[0], point[1] - origin_[1], point[2] - origin_[2]};
    return dot3(q, depth_);
}

// 0x2eb5d4: an oriented box is cut by the slab plane into one segment per face the plane crosses (from the lowest to the
// highest lateral position of the face's crossed edges). An edge counts when its end points lie strictly on both sides.
void CameraSweep::sliceBox(SliceShape& shape, const float matrix[16], const float centre[3], const float half[3]) const {
    constexpr int kEdges[12][2] = {{0, 1}, {0, 2}, {0, 4}, {1, 3}, {1, 5}, {2, 3}, {2, 6}, {3, 7}, {4, 5}, {4, 6}, {5, 7}, {6, 7}};
    constexpr int kFaces[6][4] = {{1, 2, 6, 9}, {3, 4, 7, 10}, {0, 2, 4, 8}, {5, 6, 7, 11}, {0, 1, 3, 5}, {8, 9, 10, 11}};
    float x[8], d[8], y[8];
    for (int corner = 0; corner < 8; ++corner) {
        const float local[3] = {centre[0] + ((corner & 1) ? half[0] : -half[0]), centre[1] + ((corner & 2) ? half[1] : -half[1]),
                                centre[2] + ((corner & 4) ? half[2] : -half[2])};
        float world[3];
        for (int a = 0; a < 3; ++a) world[a] = local[0] * matrix[a] + local[1] * matrix[4 + a] + local[2] * matrix[8 + a] + matrix[12 + a];
        const float q[3] = {world[0] - origin_[0], world[1] - origin_[1], world[2] - origin_[2]};
        x[corner] = dot3(q, shape.lateral);
        d[corner] = dot3(q, shape.plane);
        y[corner] = dot3(q, depth_);
    }
    bool crossed[12];
    float cutX[12], cutY[12];
    bool any = false;
    for (int e = 0; e < 12; ++e) {
        const int a = kEdges[e][0], b = kEdges[e][1];
        crossed[e] = (d[a] > 0.0f && d[b] < 0.0f) || (d[a] < 0.0f && d[b] > 0.0f);
        if (!crossed[e]) continue;
        any = true;
        const float t = d[b] / (d[b] - d[a]);
        cutX[e] = x[a] * t + (1.0f - t) * x[b];
        cutY[e] = y[a] * t + (1.0f - t) * y[b];
    }
    if (!any) return;
    for (const auto& face : kFaces) {
        bool found = false;
        float lowX = 0, highX = 0, lowY = 0, highY = 0;
        for (const int e : face) {
            if (!crossed[e]) continue;
            if (!found) {
                lowX = highX = cutX[e];
                lowY = highY = cutY[e];
            } else {
                if (cutX[e] < lowX) {
                    lowX = cutX[e];
                    lowY = cutY[e];
                }
                if (highX < cutX[e]) {
                    highX = cutX[e];
                    highY = cutY[e];
                }
            }
            found = true;
        }
        if (!found) continue;
        if (shape.raw.size() >= kMaxRawSegments) break;
        shape.raw.push_back({lowX, lowY, highX, highY});
    }
}

void CameraSweep::prepare(const CollisionWorld& world, const float camera[3], const float look[3], float fullDistance,
                          const CameraSweepSettings& settings) {
    fullDistance_ = fullDistance;
    apex_ = fullDistance + settings.reach;
    float toLook[3] = {look[0] - camera[0], look[1] - camera[1], look[2] - camera[2]};
    const float distance = std::sqrt(toLook[0] * toLook[0] + toLook[1] * toLook[1] + toLook[2] * toLook[2]);
    if (!(distance > 1.0e-6f)) {
        shapes_[Side] = SliceShape();
        shapes_[Up] = SliceShape();
        return;
    }
    const float inverse = 1.0f / distance;
    for (int i = 0; i < 3; ++i) depth_[i] = toLook[i] * inverse;
    const float worldUp[3] = {0.0f, 1.0f, 0.0f};
    float right[3], up[3];
    cross3(depth_, worldUp, right);
    normalise3(right);
    cross3(right, depth_, up);
    normalise3(up);
    farLimit_ = fullDistance - 0.25f;
    nearLimit_ = std::min(fullDistance - (std::min(distance, 30.0f) - 0.1f), farLimit_ - 0.01f);
    for (int i = 0; i < 3; ++i) origin_[i] = look[i] + depth_[i] * (settings.reach - apex_);

    for (int s = 0; s < 2; ++s) {
        SliceShape& shape = shapes_[s];
        shape = SliceShape();
        const float* lateral = s == Side ? right : up;
        const float* plane = s == Side ? up : right;
        for (int i = 0; i < 3; ++i) {
            shape.lateral[i] = lateral[i];
            shape.plane[i] = plane[i];
        }
        shape.size = s == Side ? settings.sideHalfWidth : settings.upHalfWidth;
    }

    float low[3], high[3];
    bool first = true;
    for (int s = 0; s < 2; ++s) {
        const SliceShape& shape = shapes_[s];
        float points[3][3];
        for (int i = 0; i < 3; ++i) {
            points[0][i] = origin_[i] - shape.size * shape.lateral[i];
            points[1][i] = origin_[i] + shape.size * shape.lateral[i];
            points[2][i] = origin_[i] + apex_ * depth_[i];
        }
        for (int i = 0; i < 3; ++i) {
            const float lo = std::min({points[0][i], points[1][i], points[2][i]});
            const float hi = std::max({points[0][i], points[1][i], points[2][i]});
            low[i] = first ? lo : std::min(low[i], lo);
            high[i] = first ? hi : std::max(high[i], hi);
        }
        first = false;
    }

    const float nearDepth = settings.previousNear >= 0.0f ? settings.previousNear : nearLimit_;
    for (const CollisionWorld::Entry& entry : world.entries()) {
        const assets::CollisionItem& item = entry.item;
        if (!entry.active || item.nodes.empty() || !kCameraLayer.accepts(entry.mask)) continue;
        if (!settings.excludedOwner.empty() && entry.owner == settings.excludedOwner) continue;
        if (entry.box) {
            const assets::CollisionNode& root = item.nodes[0];
            bool overlaps = true;
            for (int a = 0; a < 3; ++a)
                overlaps = overlaps && root.centre[a] - root.half[a] <= high[a] && root.centre[a] + root.half[a] >= low[a];
            if (!overlaps) continue;
            if (entry.half[0] < kSmallObjectHalfSize && entry.half[2] < kSmallObjectHalfSize) {
                float centre[3];
                for (int a = 0; a < 3; ++a)
                    centre[a] = entry.centre[0] * entry.matrix[std::size_t(a)] + entry.centre[1] * entry.matrix[4 + std::size_t(a)] +
                                entry.centre[2] * entry.matrix[8 + std::size_t(a)] + entry.matrix[12 + std::size_t(a)];
                if (nearDepth + kSmallObjectDistance < slabDepth(centre)) continue;
            }
            sliceBox(shapes_[Side], entry.matrix.data(), entry.centre, entry.half);
            sliceBox(shapes_[Up], entry.matrix.data(), entry.centre, entry.half);
            continue;
        }
        std::vector<const assets::CollisionTriangle*> hits;
        struct Walker {
            const assets::CollisionItem& item;
            const float* low;
            const float* high;
            std::vector<const assets::CollisionTriangle*>& hits;
            void walk(std::uint32_t index) const {
                if (hits.size() >= kMaxTrianglesPerItem) return;
                const assets::CollisionNode& node = item.nodes[index];
                for (int a = 0; a < 3; ++a)
                    if (node.centre[a] - node.half[a] > high[a] || node.centre[a] + node.half[a] < low[a]) return;
                if (!node.leaf()) {
                    for (std::uint32_t i = 0; i < node.count(); ++i) walk(node.first() + i);
                    return;
                }
                for (std::uint32_t i = 0; i < node.count(); ++i) {
                    if (hits.size() >= kMaxTrianglesPerItem) return;
                    const assets::CollisionTriangle& triangle = item.triangles[node.first() + i];
                    if ((std::uint64_t(1) << triangle.surface) & kCameraExcludedSurfaces) continue;
                    hits.push_back(&triangle);
                }
            }
        } walker{item, low, high, hits};
        walker.walk(0);
        if (hits.empty()) continue;

        const std::size_t rawBefore = shapes_[Side].raw.size();
        const std::size_t otherBefore = shapes_[Up].raw.size();
        float minX = 3.4028235e+38f, maxX = -3.4028235e+38f, minY = minX, maxY = maxX;
        for (const assets::CollisionTriangle* triangle : hits) {
            float vertices[3][3];
            for (int k = 0; k < 3; ++k)
                for (int a = 0; a < 3; ++a) vertices[k][a] = item.vertices[std::size_t(triangle->vertex[k]) * 3 + std::size_t(a)];
            if (slice(shapes_[Side], vertices)) {
                const Segment& s = shapes_[Side].raw.back();
                minX = std::min({minX, s.x0, s.x1});
                maxX = std::max({maxX, s.x0, s.x1});
                minY = std::min({minY, s.y0, s.y1});
                maxY = std::max({maxY, s.y0, s.y1});
            }
            slice(shapes_[Up], vertices);
        }
        if (shapes_[Side].raw.size() != rawBefore && (maxX - minX) * 0.5f < kSmallObjectHalfSize &&
            (maxY - minY) * 0.5f < kSmallObjectHalfSize && nearDepth + kSmallObjectDistance < (minY + maxY) * 0.5f) {
            shapes_[Side].raw.resize(rawBefore);
            shapes_[Up].raw.resize(otherBefore);
        }
    }
    for (int s = 0; s < 2; ++s) process(shapes_[s]);
}

void CameraSweep::process(SliceShape& shape) const {
    shape.pieces.clear();
    std::vector<Interval> intervals;
    const Clip nearClip{0.0f, nearLimit_, 0.0f, -1.0f};
    const Clip farClip{0.0f, farLimit_, 0.0f, 1.0f};
    const Clip left = makeClip(apex_, -apex_, shape.size);
    const Clip right = makeClip(apex_, apex_, shape.size);
    for (Segment& raw : shape.raw) {
        if (raw.x1 < raw.x0) {
            std::swap(raw.x0, raw.x1);
            std::swap(raw.y0, raw.y1);
        }
        Segment s = raw;
        if (!clipSegment(s, farClip) || !clipSegment(s, left) || !clipSegment(s, right)) continue;
        shape.pieces.push_back(s);
        if (!clipSegment(s, nearClip)) continue;
        float t0 = (s.x0 * apex_) / ((apex_ - s.y0) * shape.size);
        float t1 = (s.x1 * apex_) / ((apex_ - s.y1) * shape.size);
        float low = t0, high = t1;
        if (t1 < t0) {
            low = t1;
            high = t0;
        }
        if (std::fabs(s.y0 - nearLimit_) < kEdgeTolerance && low < 0.0f) low = -1.0f;
        if (std::fabs(s.y1 - nearLimit_) < kEdgeTolerance && 0.0f < high) high = 1.0f;
        if (std::fabs(s.y0 - farLimit_) < kEdgeTolerance && low < 0.0f) low = -1.0f;
        if (std::fabs(s.y1 - farLimit_) < kEdgeTolerance && 0.0f < high) high = 1.0f;
        intervals.push_back({low - kIntervalMargin, high + kIntervalMargin});
        if (intervals.size() == kMaxIntervals) {
            mergeIntervals(intervals);
            if (intervals.size() == kMaxIntervals) break;
        }
    }
    mergeIntervals(intervals);
    shape.gap = widestGap(intervals);
}

CameraSweepGap CameraSweep::sideGapAt(float distance) const {
    const SliceShape& shape = shapes_[Side];
    const float nearLimit = fullDistance_ - (distance - 0.1f);
    const Clip nearClip{0.0f, nearLimit, 0.0f, -1.0f};
    std::vector<Interval> intervals;
    for (const Segment& piece : shape.pieces) {
        Segment s = piece;
        if (!clipSegment(s, nearClip)) continue;
        float low = (s.x0 * apex_) / ((apex_ - s.y0) * shape.size);
        float high = (s.x1 * apex_) / ((apex_ - s.y1) * shape.size);
        if (std::fabs(s.y0 - nearLimit) < kEdgeTolerance && low < 0.0f) low = -1.0f;
        if (std::fabs(s.y1 - nearLimit) < kEdgeTolerance && 0.0f < high) high = 1.0f;
        if (std::fabs(s.y0 - farLimit_) < kEdgeTolerance && low < 0.0f) low = -1.0f;
        if (std::fabs(s.y1 - farLimit_) < kEdgeTolerance && 0.0f < high) high = 1.0f;
        intervals.push_back({low - kIntervalMargin, high + kIntervalMargin});
        if (intervals.size() == kMaxIntervals) {
            mergeIntervals(intervals);
            if (intervals.size() == kMaxIntervals) break;
        }
    }
    mergeIntervals(intervals);
    return widestGap(intervals);
}

float CameraSweep::deepestObstacle(Shape which, float scale) const {
    const SliceShape& shape = shapes_[which];
    const float half = shape.size * scale;
    const Clip left = makeClip(apex_, -apex_, half);
    const Clip right = makeClip(apex_, apex_, half);
    float deepest = 0.0f;
    for (const Segment& piece : shape.pieces) {
        Segment s = piece;
        if (!clipSegment(s, left) || !clipSegment(s, right)) continue;
        deepest = std::max(deepest, s.y0);
        deepest = std::max(deepest, s.y1);
    }
    return deepest;
}

}
