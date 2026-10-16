#pragma once
// The floor graph of a level's "collision_floor" item (assets::FloorGraph; the game keeps it at 0x376c60..0x376c6c) and the walk 0x23b5e8 that
// follows a segment from cell to cell across the triangle edges in the xz plane (docs/re/vehicles-physics.md). The vehicle body's collision
// pass 0x1c54f8, its cell tracker 0x20c0e4 and the wheel rays start from the cell the body stands on. No SDL dependency.
#include "assets/assets.hpp"

#include <array>
#include <vector>

namespace chase::runtime {

using FloorPoint = std::array<float, 3>;

class FloorMesh {
public:
    explicit FloorMesh(const assets::CollisionItem& item) : item_(&item) {}
    bool valid() const { return item_->floor.has_value(); }
    int cellCount() const { return int(item_->floor->cells.size()); }
    const assets::FloorCell& cell(int index) const { return item_->floor->cells[std::size_t(index)]; }
    const assets::CollisionTriangle& triangle(int index) const;
    FloorPoint vertex(int index, int corner) const;

private:
    const assets::CollisionItem* item_;
};

// The neighbour value of an edge as 0x23b5e8 reads it: signed values -1..-3 are markers (no cell across the edge), everything else an unsigned
// cell index.
int floorNeighbour(std::uint16_t raw);

// What 0x23b5e8 does when the segment leaves the graph across an edge without a neighbour (its fifth argument).
enum class FloorWalkEdge { Fail = 0, Stay = 1, Clip = 2 };
// At most kFloorWalkSteps cells are visited; then the walk fails.
constexpr int kFloorWalkSteps = 30;

// 0x23b5e8 (fourth argument 0): from `cell`, which holds `from`, the cell that holds `to` in the xz plane, or -1. Per cell the edge the segment
// leaves through is picked from the sides of the corners; `to` inside that edge ends the walk, else the walk crosses to the neighbour. At an
// edge without a neighbour Stay returns the cell, Fail returns -1 and Clip also moves `to` onto the edge (0.999 of the way from `from`, the w
// word cleared; the edge length includes y).
int walkFloor(const FloorMesh& mesh, int cell, const FloorPoint& from, FloorPoint& to, FloorWalkEdge edge);

// 0x1aa478: the cells reached from `cell` across edges whose segment in the xz plane lies within `radius` of `point`, in discovery order, `cell`
// first.
std::vector<int> floorCellsWithin(const FloorMesh& mesh, float radius, const FloorPoint& point, int cell);

// 0x2384dc: the two axes of the projection that drops the dominant component of `normal` (first, second).
std::array<int, 2> floorProjectionAxes(const std::array<float, 3>& normal);

// 0x158cb8 as 0x2384dc calls it: the three side tests of `point` against the triangle corners in the projection axes.
bool floorProjectedContains(const FloorPoint& corner0, const FloorPoint& corner1, const FloorPoint& corner2, const FloorPoint& point,
                            const std::array<int, 2>& axes);

}
