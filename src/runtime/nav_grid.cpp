#include "runtime/nav_grid.hpp"

#include <cstddef>

namespace chase::runtime {
namespace {

using assets::NavGrid;

constexpr float kHalf = 0.5f;
constexpr float kOver256 = 1.0f / 256.0f;
constexpr float kOver512 = 1.0f / 512.0f;
constexpr float kNearScale = 1.25f;

struct Box {
    float centre[3];
    float half[3];
};

Box gridBox(const NavWorld& world, const NavGrid& grid) {
    Box box;
    const float widthSigned = static_cast<float>(static_cast<std::int32_t>(grid.width));
    const float depthSigned = static_cast<float>(static_cast<std::int32_t>(grid.depth));
    float x = static_cast<float>(grid.x);
    x = x + widthSigned * kHalf;
    box.centre[0] = world.origin[0] + x * world.cellSize;
    float y = static_cast<float>(static_cast<std::uint32_t>(grid.height) + (static_cast<std::uint32_t>(grid.heightExtent) >> 1));
    y = y * kOver256;
    box.centre[1] = world.origin[1] + world.heightUnit * y;
    float z = static_cast<float>(grid.z);
    z = z + depthSigned * kHalf;
    box.centre[2] = world.origin[2] + z * world.cellSize;
    box.half[0] = world.cellSize * (static_cast<float>(grid.width) * kHalf);
    box.half[1] = world.heightUnit * (static_cast<float>(grid.heightExtent) * kOver512);
    box.half[2] = world.cellSize * (static_cast<float>(grid.depth) * kHalf);
    return box;
}

bool insideAxis(float position, float centre, float half) {
    const float d = position - centre;
    if (d > half) return false;
    return !(d < -half);
}

// 0x2397a0
bool insideBox(const float position[3], const Box& box) {
    return insideAxis(position[0], box.centre[0], box.half[0]) && insideAxis(position[1], box.centre[1], box.half[1]) &&
           insideAxis(position[2], box.centre[2], box.half[2]);
}

// 0x20d99c: x and z only.
bool insideFlat(const float position[3], const float centre[3], const float half[3]) {
    return insideAxis(position[0], centre[0], half[0]) && insideAxis(position[2], centre[2], half[2]);
}

}

NavWorld NavWorld::fromRooms(const std::vector<assets::NavRoom>& descriptors, const std::vector<assets::NavMesh>& meshes) {
    NavWorld world;
    if (!descriptors.empty()) {
        world.heightUnit = descriptors.front().heightUnit;
        world.cellSize = descriptors.front().cellSize;
        world.origin = {descriptors.front().origin[0], descriptors.front().origin[1], descriptors.front().origin[2]};
    }
    for (std::size_t i = 0; i < descriptors.size() && i < meshes.size(); ++i)
        world.rooms.push_back({descriptors[i].base, descriptors[i].end, meshes[i], {}});
    for (auto& room : world.rooms) {
        room.state.resize(room.mesh.grids.size());
        for (std::size_t g = 0; g < room.mesh.grids.size(); ++g) room.state[g].extras = room.mesh.grids[g].extras;
    }
    return world;
}

std::vector<NavWorld> navWorldsFromLevel(const std::vector<assets::NavRoom>& descriptors, const std::vector<assets::NavMesh>& meshes) {
    std::vector<NavWorld> worlds;
    std::size_t first = 0;
    while (first < descriptors.size()) {
        std::size_t last = first;
        while (last < descriptors.size() && descriptors[last].section == descriptors[first].section) ++last;
        std::vector<assets::NavRoom> rooms(descriptors.begin() + std::ptrdiff_t(first), descriptors.begin() + std::ptrdiff_t(last));
        std::vector<assets::NavMesh> sectionMeshes;
        for (std::size_t i = first; i < last && i < meshes.size(); ++i) sectionMeshes.push_back(meshes[i]);
        worlds.push_back(NavWorld::fromRooms(rooms, sectionMeshes));
        first = last;
    }
    return worlds;
}

NavGrid* navGridById(NavWorld& world, std::uint16_t id) {
    for (auto& room : world.rooms) {
        if (room.base <= id && id < room.end) {
            if (room.mesh.grids.empty()) return nullptr;
            const std::size_t local = std::size_t(id) - room.base;
            return local < room.mesh.grids.size() ? &room.mesh.grids[local] : nullptr;
        }
    }
    return nullptr;
}

NavGridState* navGridStateById(NavWorld& world, std::uint16_t id) {
    for (auto& room : world.rooms) {
        if (room.base <= id && id < room.end) {
            const std::size_t local = std::size_t(id) - room.base;
            return local < room.state.size() ? &room.state[local] : nullptr;
        }
    }
    return nullptr;
}

std::uint16_t navTotalGrids(const NavWorld& world) {
    std::uint16_t total = 0;
    for (const auto& room : world.rooms) total = room.end > total ? room.end : total;
    return total;
}

const NavGrid* navGridById(const NavWorld& world, std::uint16_t id) {
    return navGridById(const_cast<NavWorld&>(world), id);
}

int navCellAt(const NavWorld& world, const NavGrid& grid, const float position[3]) {
    const float cornerX = static_cast<float>(grid.x) * world.cellSize + world.origin[0];
    const float cornerZ = static_cast<float>(grid.z) * world.cellSize + world.origin[2];
    const int cx = static_cast<int>((position[0] - cornerX) / world.cellSize);
    const int cz = static_cast<int>((position[2] - cornerZ) / world.cellSize);
    if (cx < 0 || grid.width <= cx) return -1;
    if (cz < 0 || grid.depth <= cz) return -1;
    const int index = cz * grid.width + cx;
    if ((grid.cells[std::size_t(index)] & 3) == 1) return -1;
    return index;
}

NavLocation navLocate(const NavWorld& world, const float position[3], float tolerance, bool wantNear) {
    NavLocation result;
    for (std::size_t r = 0; r < world.rooms.size(); ++r) {
        const NavWorldRoom& room = world.rooms[r];
        if (room.mesh.grids.empty()) continue;
        for (const assets::NavPath& box : room.mesh.paths) {
            if (!insideFlat(position, box.position, box.extent)) continue;
            int best = -1;
            float bestDistance = tolerance;
            for (const std::uint16_t local : box.indices) {
                if (local >= room.mesh.grids.size()) continue;
                const NavGrid& grid = room.mesh.grids[local];
                Box gridBounds = gridBox(world, grid);
                if (insideBox(position, gridBounds)) {
                    const int cell = navCellAt(world, grid, position);
                    if (cell >= 0) {
                        result.room = static_cast<std::uint16_t>(r);
                        result.grid = static_cast<std::uint16_t>(local + room.base);
                        result.cell = static_cast<std::uint16_t>(cell);
                        result.found = true;
                        return result;
                    }
                }
                const float bottom = gridBounds.centre[1] - gridBounds.half[1];
                if (bottom >= position[1]) continue;
                if (insideFlat(position, gridBounds.centre, gridBounds.half)) {
                    const float above = position[1] - (gridBounds.centre[1] + gridBounds.half[1]);
                    if (above < bestDistance) {
                        bestDistance = above;
                        best = local;
                    }
                }
                if (wantNear) {
                    for (float& h : gridBounds.half) h = h * kNearScale;
                    if (insideBox(position, gridBounds)) result.nearGrid = static_cast<std::int64_t>(local) + room.base;
                }
            }
            if (best < 0) break;
            const NavGrid& grid = room.mesh.grids[std::size_t(best)];
            const int cell = navCellAt(world, grid, position);
            if (cell < 0) break;
            result.room = static_cast<std::uint16_t>(r);
            result.grid = static_cast<std::uint16_t>(best + room.base);
            result.cell = static_cast<std::uint16_t>(cell);
            result.found = true;
            return result;
        }
    }
    return result;
}

}
