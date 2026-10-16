#include "runtime/floor_walk.hpp"

#include <cmath>

namespace chase::runtime {

const assets::CollisionTriangle& FloorMesh::triangle(int index) const {
    const assets::FloorCell& entry = cell(index);
    return entry.itemTriangle ? item_->triangles[entry.triangle] : item_->floor->extraTriangles[entry.triangle];
}

FloorPoint FloorMesh::vertex(int index, int corner) const {
    const assets::FloorCell& entry = cell(index);
    const std::uint32_t v = triangle(index).vertex[corner];
    const std::vector<float>& vertices = entry.itemTriangle ? item_->vertices : item_->floor->extraVertices;
    return {vertices[std::size_t(v) * 3], vertices[std::size_t(v) * 3 + 1], vertices[std::size_t(v) * 3 + 2]};
}

int floorNeighbour(std::uint16_t raw) {
    const int value = std::int16_t(raw);
    return value > -4 ? value : int(raw);
}

namespace {
float planeSide(const assets::CollisionTriangle& triangle, const FloorPoint& p) {
    float sum = triangle.normal[0] * p[0];
    sum += triangle.normal[1] * p[1];
    sum += triangle.normal[2] * p[2];
    return sum - triangle.d;
}

int leavingEdge(const FloorMesh& mesh, int cell, const FloorPoint& from, float dx, float dz) {
    const auto side = [&](const FloorPoint& v) { return (v[2] - from[2]) * dx - dz * (v[0] - from[0]); };
    if (side(mesh.vertex(cell, 0)) > 0.0f) return side(mesh.vertex(cell, 1)) <= 0.0f ? 0 : 1;
    return side(mesh.vertex(cell, 2)) > 0.0f ? 2 : 1;
}
}

std::optional<FloorRayHit> floorRay(const FloorMesh& mesh, int cell, const FloorPoint& start, const std::array<float, 3>& direction, float length) {
    if (cell < 0 || planeSide(mesh.triangle(cell), start) < 0.0f) return std::nullopt;
    const FloorPoint end = {start[0] + direction[0] * length, start[1] + direction[1] * length, start[2] + direction[2] * length};
    for (int steps = 29; steps > 0; --steps) {
        const assets::CollisionTriangle& triangle = mesh.triangle(cell);
        const int edge = leavingEdge(mesh, cell, start, direction[0], direction[2]);
        const FloorPoint a = mesh.vertex(cell, edge);
        const FloorPoint b = mesh.vertex(cell, (edge + 1) % 3);
        const float ex = b[0] - a[0];
        const float ez = b[2] - a[2];
        const float endSide = planeSide(triangle, end);
        if (endSide < 0.0f) {
            const float startSide = planeSide(triangle, start);
            const float fraction = startSide / (startSide - endSide);
            const float reach = length * fraction;
            const FloorPoint point = {start[0] + direction[0] * reach, start[1] + direction[1] * reach, start[2] + direction[2] * reach};
            if ((point[0] - a[0]) * ez - ex * (point[2] - a[2]) > 0.0f) return FloorRayHit{fraction, point, cell};
        }
        if (0.0f < (end[0] - a[0]) * ez - ex * (end[2] - a[2])) return std::nullopt;
        const int next = floorNeighbour(mesh.cell(cell).neighbour[edge]);
        if (next < 0) return std::nullopt;
        cell = next;
    }
    return std::nullopt;
}

int walkFloor(const FloorMesh& mesh, int cell, const FloorPoint& from, FloorPoint& to, FloorWalkEdge edge) {
    const float dx = to[0] - from[0];
    const float dz = to[2] - from[2];
    const auto side = [&](const FloorPoint& v) { return (v[2] - from[2]) * dx - dz * (v[0] - from[0]); };
    for (int steps = kFloorWalkSteps; steps > 0; --steps) {
        if (cell < 0) return -1;
        int leave;
        if (side(mesh.vertex(cell, 0)) <= 0.0f) leave = side(mesh.vertex(cell, 2)) > 0.0f ? 2 : 1;
        else leave = side(mesh.vertex(cell, 1)) > 0.0f ? 1 : 0;
        const FloorPoint a = mesh.vertex(cell, leave);
        const FloorPoint b = mesh.vertex(cell, (leave + 1) % 3);
        if ((b[2] - a[2]) * (to[0] - a[0]) - (b[0] - a[0]) * (to[2] - a[2]) > 0.0f) return cell;
        const int next = floorNeighbour(mesh.cell(cell).neighbour[leave]);
        if (next >= 0) {
            cell = next;
            continue;
        }
        if (edge == FloorWalkEdge::Stay) return cell;
        if (edge == FloorWalkEdge::Fail) return -1;
        float ex = b[0] - a[0];
        float ez = b[2] - a[2];
        const float ey = b[1] - a[1];
        const float scale = 1.0f / std::sqrt(ex * ex + ey * ey + ez * ez);
        ex *= scale;
        ez *= scale;
        const float start = (from[2] - a[2]) * ex - ez * (from[0] - a[0]);
        const float span = start - ((to[2] - a[2]) * ex - ez * (to[0] - a[0]));
        if (span != 0.0f) {
            const float t = start * 0.999f / span;
            to = {from[0] + (to[0] - from[0]) * t, from[1] + (to[1] - from[1]) * t, from[2] + (to[2] - from[2]) * t};
        }
        return cell;
    }
    return -1;
}

std::vector<int> floorCellsWithin(const FloorMesh& mesh, float radius, const FloorPoint& point, int cell) {
    std::vector<int> found{cell};
    std::vector<bool> visited(std::size_t(mesh.cellCount()), false);
    visited[std::size_t(cell)] = true;
    for (std::size_t at = 0; at < found.size(); ++at) {
        const int current = found[at];
        for (int edge = 0; edge < 3; ++edge) {
            const int next = floorNeighbour(mesh.cell(current).neighbour[edge]);
            if (next < 0 || visited[std::size_t(next)]) continue;
            const FloorPoint a = mesh.vertex(current, edge);
            const FloorPoint b = mesh.vertex(current, (edge + 1) % 3);
            const float ex = b[0] - a[0];
            const float ez = b[2] - a[2];
            const float length = std::sqrt(ex * ex + 0.0f + ez * ez);
            const float inverse = 1.0f / length;
            const float dx = point[0] - a[0];
            const float dz = point[2] - a[2];
            if (!(std::fabs(dx * ez * inverse - dz * ex * inverse) < radius)) continue;
            const float along = dx * ex * inverse + inverse * 0.0f * 0.0f + dz * ez * inverse;
            float squared = 0.0f;
            bool outside = false;
            if (0.0f <= along) {
                if (length < along) {
                    const float fx = point[0] - b[0];
                    const float fz = point[2] - b[2];
                    squared = fx * fx + 0.0f + fz * fz;
                    outside = true;
                }
            } else {
                squared = dx * dx + 0.0f + dz * dz;
                outside = true;
            }
            if (outside && radius * radius < squared) continue;
            visited[std::size_t(next)] = true;
            found.push_back(next);
        }
    }
    return found;
}

std::array<int, 2> floorProjectionAxes(const std::array<float, 3>& normal) {
    const float x = normal[0], y = normal[1], z = normal[2];
    const float ax = std::fabs(x), ay = std::fabs(y), az = std::fabs(z);
    if (ax < az) {
        if (ay < az) return z < 0.0f ? std::array<int, 2>{1, 0} : std::array<int, 2>{0, 1};
        return y <= 0.0f ? std::array<int, 2>{0, 2} : std::array<int, 2>{2, 0};
    }
    if (ay < ax) return x < 0.0f ? std::array<int, 2>{2, 1} : std::array<int, 2>{1, 2};
    return y <= 0.0f ? std::array<int, 2>{0, 2} : std::array<int, 2>{2, 0};
}

bool floorProjectedContains(const FloorPoint& corner0, const FloorPoint& corner1, const FloorPoint& corner2, const FloorPoint& point,
                            const std::array<int, 2>& axes) {
    const int a = axes[0], b = axes[1];
    const float s4 = point[a] - corner0[a];
    const float s5 = point[b] - corner0[b];
    const float s8 = corner2[a] - corner0[a];
    const float s9 = corner2[b] - corner0[b];
    if (s5 * s8 > s4 * s9) return false;
    const float p0 = (corner1[a] - corner0[a]) * s5;
    const float p1 = (corner1[b] - corner0[b]) * s4;
    if (p1 > p0) return false;
    const float q1 = (point[b] - corner2[b]) * (corner1[a] - corner2[a]);
    const float q0 = (point[a] - corner2[a]) * (corner1[b] - corner2[b]);
    return q0 >= q1;
}

}
