#include "runtime/nav_follow.hpp"

#include <cmath>

namespace chase::runtime {
namespace {

constexpr float kOver256 = 1.0f / 256.0f;
constexpr float kHeadingScale = 0x1.45f306p-2f;
constexpr float kHeadingUnits = 128.0f;
constexpr float kNearCentreSquared = 0x1.99999ap-4f;
constexpr float kAboveLinkTolerance = 0x1.ff7ceep-1f;

std::uint8_t headingByte(float dx, float dz) {
    float angle = std::atan2(dx, dz);
    angle = angle * kHeadingUnits;
    angle = angle * kHeadingScale;
    return static_cast<std::uint8_t>(static_cast<std::int32_t>(angle));
}

std::uint8_t headingToward(const std::array<float, 3>& target, const float position[3]) {
    return headingByte(target[0] - position[0], target[2] - position[2]);
}

NavFollowResult blocked(NavPathRecord& record, std::uint8_t diagnostic) {
    record.diagnostic = diagnostic;
    record.phase = 1;
    return NavFollowResult{};
}

NavFollowResult keepHeading() {
    NavFollowResult result;
    result.code = 3;
    return result;
}

std::uint8_t cellFlagsOf(const assets::NavGrid& grid, std::size_t cell) { return cell < grid.cells.size() ? grid.cells[cell] : 0; }

// 0x272598: the cell across the link named by `extra`, found from the position's cell in this grid plus the extra's offset.
NavFollowResult transition(NavWorld& world, NavPathRecord& record, const assets::NavGrid& grid, const float position[3],
                           const std::array<std::uint8_t, 4>& extra) {
    NavFollowResult result;
    const unsigned gate = extra[1];
    const auto refuse = [&](int code, std::uint8_t heading) {
        record.diagnostic = 1;
        result.code = code;
        result.heading = heading;
        return result;
    };
    if (gate != 0 && !(gate < 32 && ((record.avoidMask >> gate) & 1u))) return refuse(2, static_cast<std::uint8_t>(gate));
    const unsigned slot = extra[0] & 63u;
    const assets::NavGrid* across = slot < 16 ? navGridById(world, grid.linkSlots[slot]) : nullptr;
    if (!across) return refuse(2, 0xff);
    if ((across->flags & 0x4000) && !(record.avoidMask & 1)) return refuse(2, 0xff);
    if (across->flags & 0x8000) return keepHeading();
    const float cell = world.cellSize;
    float cornerX = static_cast<float>(grid.x) * cell;
    cornerX = world.origin[0] + cornerX;
    float cornerZ = static_cast<float>(grid.z) * cell;
    cornerZ = world.origin[2] + cornerZ;
    float cornerY = static_cast<float>(grid.height) * kOver256;
    cornerY = world.origin[1] + world.heightUnit * cornerY;
    const int cellX = static_cast<int>((position[0] - cornerX) / cell) + static_cast<std::int8_t>(extra[2]);
    const int cellZ = static_cast<int>((position[2] - cornerZ) / cell) + static_cast<std::int8_t>(extra[3]);
    float targetX = cell * static_cast<float>(cellX);
    targetX = targetX + cell * 0.5f;
    targetX = targetX + cornerX;
    float rise = static_cast<float>(static_cast<int>(across->height) - static_cast<int>(grid.height));
    rise = rise * kOver256;
    const float targetY = cornerY + world.heightUnit * rise;
    float targetZ = cell * static_cast<float>(cellZ);
    targetZ = targetZ + cell * 0.5f;
    targetZ = targetZ + cornerZ;
    record.target = {targetX, targetY, targetZ};
    record.phase = 3;
    result.code = static_cast<int>(gate) + 5;
    result.heading = headingToward(record.target, position);
    result.target = record.target;
    return result;
}

// 0x27ddb4 reached through phase 2.
NavFollowResult followInGrid(NavWorld& world, NavPathRecord& record, const float position[3]) {
    NavFollowResult result;
    assets::NavGrid* grid = navGridById(world, record.startGrid);
    NavGridState* state = navGridStateById(world, record.startGrid);
    if (!grid || !state || grid->width == 0) return blocked(record, 7);
    const float cornerX = world.origin[0] + static_cast<float>(grid->x) * world.cellSize;
    const float cornerY = world.origin[1] + world.heightUnit * (static_cast<float>(grid->height) * kOver256);
    const float cornerZ = world.origin[2] + static_cast<float>(grid->z) * world.cellSize;
    const int cellX = static_cast<int>((position[0] - cornerX) / world.cellSize);
    const int cellZ = static_cast<int>((position[2] - cornerZ) / world.cellSize);
    if (cellX < 0 || grid->width <= cellX || cellZ < 0 || grid->depth <= cellZ) return blocked(record, 4);
    const int cell = cellZ * grid->width + cellX;
    if (record.exitCell == cell && (grid->flags & 0x3fffu) != 0) {
        const std::size_t index = cellFlagsOf(*grid, std::size_t(cell)) >> 3;
        if (index < state->extras.size() && !(state->extras[index][0] & 64)) {
            if (record.startGrid == record.goalGrid) {
                record.phase = 0;
                result.code = 0;
                return result;
            }
            const unsigned slot = state->extras[index][0] & 63u;
            if (slot >= 16 || grid->linkSlots[slot] != record.gridNodes[record.startGrid].parent) return blocked(record, 0);
            return transition(world, record, *grid, position, state->extras[index]);
        }
    }
    if (record.cellNodes[record.startCell].mark == cell) record.startCell = static_cast<std::uint16_t>(cell);
    if (record.exitCell != kNavNone && (cellFlagsOf(*grid, record.exitCell) & 3) == 3 && !(record.avoidMask & 1)) {
        record.diagnostic = 1;
        result.code = 2;
        result.heading = 0xff;
        return result;
    }
    const NavCellNode* start = &record.cellNodes[record.startCell];
    if (!(start->state == 3 && start->cost < 10000)) {
        if (record.exitCell == kNavNone || record.startGrid == record.goalGrid) return blocked(record, 2);
        std::uint8_t& exitFlags = grid->cells[record.exitCell];
        if (!(exitFlags & 3)) exitFlags = static_cast<std::uint8_t>((exitFlags & ~3) | 2);
        state->stamp = world.now;
        navBuildLegs(world, record);
        if (record.exitCell == kNavNone) {
            const std::uint8_t startFlags = cellFlagsOf(*grid, record.startCell);
            if (startFlags & 4) {
                record.exitCell = record.startCell;
                const std::size_t index = startFlags >> 3;
                if (index >= state->extras.size()) return blocked(record, 7);
                return transition(world, record, *grid, position, state->extras[index]);
            }
        }
        start = &record.cellNodes[record.startCell];
        if (start->state != 3 || start->cost >= 10000) return keepHeading();
    }
    if (record.exitCell == cell) return keepHeading();
    const int waypoint = start->mark;
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
    result.code = 4;
    result.heading = headingByte(targetX - position[0], targetZ - position[2]);
    return result;
}

// 0x1fb104: walking to the cell across a link; once within half a cell of it the next grid becomes the start grid.
NavFollowResult followLink(NavWorld& world, NavPathRecord& record, const float position[3], bool airborne) {
    NavFollowResult result;
    const assets::NavGrid* grid = navGridById(world, record.startGrid);
    const NavGridState* state = navGridStateById(world, record.startGrid);
    if (!grid || !state) return blocked(record, 7);
    const std::uint8_t exitFlags = cellFlagsOf(*grid, record.exitCell);
    if (!(exitFlags & 4)) return blocked(record, 0);
    const std::size_t index = exitFlags >> 3;
    const unsigned gate = index < state->extras.size() ? state->extras[index][1] : 0u;
    const float dx = position[0] - record.target[0];
    const float dy = position[1] - record.target[1];
    const float dz = position[2] - record.target[2];
    const float distance = std::sqrt(dx * dx + dz * dz);
    if (distance > world.cellSize * 0.5f) {
        result.code = static_cast<int>(gate) + 5;
        result.heading = headingToward(record.target, position);
        result.target = record.target;
        return result;
    }
    const std::uint16_t nextId = record.gridNodes[record.startGrid].parent;
    const assets::NavGrid* next = navGridById(world, nextId);
    if (!next) return blocked(record, 3);
    if (!navInsideBox(position, navGridBox(world, *next))) {
        if (std::fabs(dy) > kAboveLinkTolerance && !(airborne && gate == 0)) return keepHeading();
        return blocked(record, 3);
    }
    const int cell = navCellAt(world, *next, position);
    if (cell < 0) return blocked(record, 3);
    record.startGrid = nextId;
    record.startCell = static_cast<std::uint16_t>(cell);
    if (next->flags & 0x8000) return keepHeading();
    if ((next->flags & 0x4000) && !(record.avoidMask & 1)) return keepHeading();
    navBuildLegs(world, record);
    if (record.cellNodes[record.startCell].state != 3) return blocked(record, 2);
    record.phase = 2;
    if (gate != 0) return keepHeading();
    return followInGrid(world, record, position);
}

// 0x1faf70: the start position lay beside the grid; walk to its centre until the position is inside it.
NavFollowResult followToGrid(NavWorld& world, NavPathRecord& record, const float position[3]) {
    NavFollowResult result;
    const assets::NavGrid* grid = navGridById(world, record.startGrid);
    if (!grid) return result;
    const NavBox box = navGridBox(world, *grid);
    int cell = -1;
    if (navInsideBox(position, box)) cell = navCellAt(world, *grid, position);
    if (cell < 0) {
        const float dx = position[0] - box.centre[0];
        const float dz = position[2] - box.centre[2];
        const float squared = dx * dx + dz * dz;
        if (squared < kNearCentreSquared) return blocked(record, 4);
        record.target = {box.centre[0], box.centre[1], box.centre[2]};
        result.code = 4;
        result.heading = headingToward(record.target, position);
        result.target = record.target;
        return result;
    }
    record.startCell = static_cast<std::uint16_t>(cell);
    navResetGrids(world);
    navSearchGrids(world, record);
    if (record.cellNodes[record.startCell].state == 3) {
        record.phase = 2;
        return followInGrid(world, record, position);
    }
    return blocked(record, record.gridNodes[record.startGrid].state == 3 ? 2 : 6);
}

}

NavFollowResult navFollow(NavWorld& world, NavPathRecord& record, const float position[3], bool airborne) {
    switch (record.phase) {
    case 0: {
        NavFollowResult arrived;
        arrived.code = 0;
        return arrived;
    }
    case 2: return followInGrid(world, record, position);
    case 3: return followLink(world, record, position, airborne);
    case 4: return followToGrid(world, record, position);
    default: return NavFollowResult{};
    }
}

}
