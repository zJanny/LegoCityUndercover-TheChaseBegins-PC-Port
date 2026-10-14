#include "runtime/nav_follow.hpp"

#include <cmath>

namespace chase::runtime {

NavFollowResult navFollow(NavWorld& world, NavPathRecord& record, const float position[3]) {
    NavFollowResult result;
    if (record.phase == 0) {
        result.code = 0;
        return result;
    }
    if (record.phase != 2) return result;
    const assets::NavGrid* grid = navGridById(world, record.startGrid);
    const NavGridState* state = navGridStateById(world, record.startGrid);
    const auto blocked = [&](std::uint8_t diagnostic) {
        record.diagnostic = diagnostic;
        record.phase = 1;
        return result;
    };
    if (!grid || !state || grid->width == 0) return blocked(7);
    const float cornerX = world.origin[0] + static_cast<float>(grid->x) * world.cellSize;
    const float cornerY = world.origin[1] + world.heightUnit * (static_cast<float>(grid->height) * (1.0f / 256.0f));
    const float cornerZ = world.origin[2] + static_cast<float>(grid->z) * world.cellSize;
    const int cellX = static_cast<int>((position[0] - cornerX) / world.cellSize);
    const int cellZ = static_cast<int>((position[2] - cornerZ) / world.cellSize);
    if (cellX < 0 || grid->width <= cellX || cellZ < 0 || grid->depth <= cellZ) return blocked(4);
    const int cell = cellZ * grid->width + cellX;
    const std::uint8_t flags = std::size_t(cell) < grid->cells.size() ? grid->cells[std::size_t(cell)] : 0;
    if (record.exitCell == cell && (grid->flags & 0x3fffu) != 0) {
        const std::size_t index = flags >> 3;
        if (index < state->extras.size() && !(state->extras[index][0] & 64)) {
            if (record.startGrid == record.goalGrid) {
                record.phase = 0;
                result.code = 0;
                return result;
            }
            return result;
        }
    }
    if (record.cellNodes[record.startCell].mark == cell) record.startCell = static_cast<std::uint16_t>(cell);
    if (record.exitCell != kNavNone) {
        const std::uint8_t exitFlags = std::size_t(record.exitCell) < grid->cells.size() ? grid->cells[record.exitCell] : 0;
        if ((exitFlags & 3) == 3 && !(record.avoidMask & 1)) {
            record.diagnostic = 1;
            result.code = 2;
            result.heading = 0xff;
            return result;
        }
    }
    const NavCellNode& start = record.cellNodes[record.startCell];
    if (!(start.state == 3 && start.cost < 10000)) {
        if (record.exitCell == kNavNone || record.startGrid == record.goalGrid) return blocked(2);
        return result;
    }
    if (record.exitCell == cell) {
        result.code = 3;
        return result;
    }
    const int waypoint = start.mark;
    const float remainder = static_cast<float>(waypoint % grid->width);
    const float quotient = static_cast<float>(waypoint / grid->width);
    float targetX = world.cellSize * remainder;
    targetX = targetX + world.cellSize * 0.5f;
    targetX = targetX + cornerX;
    float targetZ = world.cellSize * quotient;
    targetZ = targetZ + world.cellSize * 0.5f;
    targetZ = targetZ + cornerZ;
    record.target = {targetX, cornerY, targetZ};
    result.target = record.target;
    float angle = std::atan2(targetX - position[0], targetZ - position[2]);
    angle = angle * 128.0f;
    angle = angle * 0x1.45f306p-2f;
    result.code = 4;
    result.heading = static_cast<std::uint8_t>(static_cast<std::int32_t>(angle));
    return result;
}

}
