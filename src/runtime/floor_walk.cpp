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

}
