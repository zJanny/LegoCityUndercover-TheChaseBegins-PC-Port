#include "runtime/pregen_derive.hpp"

#include "runtime/detective.hpp"

#include <cmath>
#include <stdexcept>

namespace chase::runtime {

std::vector<PregenDerivedFields> deriveSpawnFields(const assets::PregenSpawns& spawns,
                                                   const std::vector<const assets::Level*>& loadedLevels,
                                                   const PregenRoomContext& context) {
    static const StreamRooms noRooms;
    static const std::map<int, RoomBox> noBoxes;
    const StreamRooms& rooms = context.rooms ? *context.rooms : noRooms;
    const std::map<int, RoomBox>& boxes = context.boxes ? *context.boxes : noBoxes;
    std::vector<PregenDerivedFields> fields;
    fields.reserve(spawns.points.size());
    for (const assets::PregenSpawnPoint& point : spawns.points) {
        PregenDerivedFields derived;
        for (std::size_t level = 0; level < loadedLevels.size() && !derived.pathFound; ++level) {
            const std::vector<assets::LevelPath>& paths = loadedLevels[level]->paths;
            for (std::size_t index = 0; index < paths.size(); ++index) {
                if (paths[index].nameHash != point.pathHash) continue;
                if (paths[index].type != 0) throw std::runtime_error("pregen point on a path of type other than 0");
                derived.pathFound = true;
                derived.levelIndex = level;
                derived.pathIndex = index;
                detectivePathTangent(paths[index], point.pathPosition, derived.tangent.data());
                break;
            }
        }
        const Vec3 position = {point.position[0], point.position[1], point.position[2]};
        derived.room = objectRoomAfterMove(rooms, boxes, 0, context.currentRoom, position);
        fields.push_back(derived);
    }
    return fields;
}

bool floorCellContains(const FloorMesh& mesh, int cell, const FloorPoint& point) {
    const FloorPoint a = mesh.vertex(cell, 0);
    const FloorPoint b = mesh.vertex(cell, 1);
    const FloorPoint c = mesh.vertex(cell, 2);
    const float aToPointX = point[0] - a[0];
    const float aToPointZ = point[2] - a[2];
    float side = (b[0] - a[0]) * aToPointZ;
    side = side - (b[2] - a[2]) * aToPointX;
    if (side >= 0.0f) return false;
    side = (a[0] - c[0]) * aToPointZ;
    side = side - (a[2] - c[2]) * aToPointX;
    if (side >= 0.0f) return false;
    side = (c[0] - b[0]) * (point[2] - b[2]);
    side = side - (c[2] - b[2]) * (point[0] - b[0]);
    return side < 0.0f;
}

namespace {

float distanceToFirstCorner(const FloorMesh& mesh, std::uint32_t cell, const FloorPoint& point) {
    const FloorPoint corner = mesh.vertex(static_cast<int>(cell), 0);
    const float dx = corner[0] - point[0];
    const float dy = corner[1] - point[1];
    const float dz = corner[2] - point[2];
    float sum = dx * dx;
    sum = sum + dy * dy;
    sum = sum + dz * dz;
    return std::sqrt(sum);
}

}

int findTrafficFloorCell(const FloorMesh& mesh, const FloorPoint& point) {
    if (!mesh.valid() || mesh.cellCount() == 0) return -1;
    const std::uint32_t count = static_cast<std::uint32_t>(mesh.cellCount());
    std::uint32_t low = 0;
    std::uint32_t high = count - 1;
    do {
        const std::uint32_t span = high - low;
        if (distanceToFirstCorner(mesh, low, point) < distanceToFirstCorner(mesh, high, point)) high = low + (span >> 1);
        else low = low + (span >> 1);
        if (high == 0) throw std::runtime_error("traffic floor cell search does not end");
    } while (low < high - 1);
    while (!(count <= high && low == 0)) {
        if (high < count) {
            if (floorCellContains(mesh, static_cast<int>(high), point)) return static_cast<int>(high);
            ++high;
        }
        if (low != 0) {
            if (floorCellContains(mesh, static_cast<int>(low), point)) return static_cast<int>(low);
            --low;
        }
    }
    return -1;
}

std::vector<int> deriveTrafficFloorCells(const assets::PregenSpawns& spawns, const FloorMesh* mesh) {
    std::vector<int> cells(spawns.points.size(), -1);
    if (!mesh || !mesh->valid()) return cells;
    for (std::size_t i = 0; i < spawns.points.size(); ++i)
        cells[i] = findTrafficFloorCell(*mesh, {spawns.points[i].position[0], spawns.points[i].position[1], spawns.points[i].position[2]});
    return cells;
}

}
