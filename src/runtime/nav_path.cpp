#include "runtime/nav_path.hpp"

#include <algorithm>
#include <cfloat>
#include <cmath>

namespace chase::runtime {
namespace {

using assets::NavGrid;

constexpr int kCostBlocked = 10000;
constexpr int kCostSoft = 2000;
constexpr int kCap = 0xffff;
constexpr int kGridSearchLimit = 0xc350;
constexpr int kDx[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
constexpr int kDz[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
constexpr int kStepCost[8] = {3, 2, 3, 2, 2, 3, 2, 3};

int minCap(int value) { return value >= kCap ? kCap : value; }

bool gateOpen(std::uint32_t mask, unsigned gate) { return gate < 32 && ((mask >> gate) & 1u); }

std::size_t cellCount(const NavGrid& grid) { return std::size_t(grid.width) * grid.depth; }

std::uint8_t cellFlags(const NavGrid& grid, std::size_t cell) { return cell < grid.cells.size() ? grid.cells[cell] : 0; }

// The cost of a step by the flags of the cell it is judged on (0x215e08 at 0x216018, 0x294b74 at 0x294cc4 and the
// following blocks): 1 blocks, 2 is soft unless the other cell is also 2, 3 is open for an avoid mask with bit 0.
int cellStepCost(const NavPathRecord& record, int flags, int otherFlags, int direction) {
    switch (flags & 3) {
    case 1: return kCostBlocked;
    case 2:
        if (record.avoidFlags & 1) return kStepCost[direction];
        return (otherFlags & 3) == 2 ? kCostSoft : kCostBlocked;
    case 3: return (record.avoidMask & 1) ? kStepCost[direction] : kCostBlocked;
    default: return kStepCost[direction];
    }
}

bool cellCrossable(const NavPathRecord& record, int flags) {
    switch (flags & 3) {
    case 0: return true;
    case 3: return (record.avoidMask & 1) != 0;
    case 2: return (record.avoidFlags & 1) != 0;
    default: return false;
    }
}

template <typename List, typename Value>
void removeFirst(List& list, Value value) {
    const auto found = std::find(list.begin(), list.end(), value);
    if (found != list.end()) list.erase(found);
}

}

NavPathRecord::NavPathRecord() : gridNodes(kNavMaxGridNodes), cellNodes(kNavMaxCellNodes), entryCell(kNavMaxGridNodes, 0) {}

// 0x1f9ea0
int navEdgeCost(NavWorld& world, const NavPathRecord& record, std::uint16_t from, std::uint16_t to) {
    const NavGrid* a = navGridById(world, from);
    const NavGrid* b = navGridById(world, to);
    NavGridState* aState = navGridStateById(world, from);
    if (!a || !b || !aState) return kCostBlocked;
    if ((a->flags & 0x8000) || (b->flags & 0x8000)) return kCostBlocked;
    const std::uint32_t mask = record.gateMask;
    if (!(mask & 1) && ((a->flags | b->flags) & 0x4000)) return kCostSoft;
    float lowestPositive = 50.0f;
    float highestNonPositive = -FLT_MAX;
    bool linked = false;
    bool linkFound = false;
    int slot = 0;
    for (; slot < 16; ++slot) {
        if (a->linkSlots[std::size_t(slot)] != to) continue;
        if ((aState->linkMask >> slot) & 1) return kCostBlocked;
        linkFound = true;
        break;
    }
    if (linkFound) {
        const unsigned count = a->flags & 0x3fffu;
        if (count == 0) return kCostBlocked;
        bool gateClosed = false;
        for (unsigned i = 0; i < count && i < aState->extras.size(); ++i) {
            const auto& extra = aState->extras[i];
            if ((extra[0] & 63) != slot || (extra[0] & 64)) continue;
            const unsigned gate = extra[1];
            if (gate != 0 && !gateOpen(mask, gate)) {
                gateClosed = true;
                continue;
            }
            linked = true;
            if (world.gateWeights.empty()) continue;
            const float weight = gate < world.gateWeights.size() ? world.gateWeights[gate] : 0.0f;
            if (weight > 0.0f) {
                if (weight < lowestPositive) lowestPositive = weight;
            } else if (highestNonPositive < weight) {
                highestNonPositive = weight;
            }
        }
        if (!linked) return gateClosed ? kCostSoft : kCostBlocked;
    }
    const auto centre = [&](const NavGrid& grid, float out[3]) {
        out[0] = static_cast<float>(static_cast<std::uint32_t>(grid.x) + (grid.width >> 1)) * world.cellSize;
        float y = static_cast<float>(static_cast<std::uint32_t>(grid.height) + (static_cast<std::uint32_t>(grid.heightExtent) >> 1));
        y = y * (1.0f / 256.0f);
        out[1] = y * world.heightUnit;
        out[2] = static_cast<float>(static_cast<std::uint32_t>(grid.z) + (grid.depth >> 1)) * world.cellSize;
    };
    float p[3], q[3];
    centre(*a, p);
    centre(*b, q);
    float sum = (p[0] - q[0]) * (p[0] - q[0]);
    sum = sum + (p[1] - q[1]) * (p[1] - q[1]);
    sum = sum + (p[2] - q[2]) * (p[2] - q[2]);
    const float half = std::sqrt(sum) * 0.5f;
    float result;
    if (world.gateWeights.empty()) {
        result = half * 2.0f;
    } else {
        const float scaled = half * lowestPositive;
        if (scaled <= -highestNonPositive) result = half + half * lowestPositive;
        else result = half - highestNonPositive;
    }
    return static_cast<int>(result);
}

// 0x2160d8
void navInsertGridNode(NavPathRecord& record, std::uint16_t grid, int cost) {
    NavGridNode& node = record.gridNodes[grid];
    const std::uint16_t stored = static_cast<std::uint16_t>(cost);
    int key = cost;
    switch (node.state) {
    case 0:
        node.key = stored;
        node.state = 1;
        key = stored;
        break;
    case 1:
    case 2:
        if (node.key < static_cast<unsigned>(cost)) key = node.key;
        node.key = static_cast<std::uint16_t>(key);
        removeFirst(record.gridOpen, grid);
        break;
    case 3:
        if (node.cost < static_cast<unsigned>(cost)) key = node.cost;
        node.key = static_cast<std::uint16_t>(key);
        node.state = 2;
        break;
    default: break;
    }
    node.cost = stored;
    const std::uint16_t wanted = node.key;
    std::size_t low = 0;
    std::size_t count = record.gridOpen.size();
    while (count > 0) {
        const std::size_t half = count / 2;
        if (wanted >= record.gridNodes[record.gridOpen[low + half]].key) {
            low += half + 1;
            count -= half + 1;
        } else {
            count = half;
        }
    }
    record.gridOpen.insert(record.gridOpen.begin() + static_cast<std::ptrdiff_t>(low), grid);
}

// 0x1fc6a4
bool navEntryCell(NavWorld& world, NavPathRecord& record, std::uint16_t grid) {
    if (record.gridNodes[grid].state == 1) record.gridNodes[grid].state = 2;
    const std::uint16_t parent = record.gridNodes[grid].parent;
    const NavGrid* from = navGridById(world, grid);
    const NavGrid* to = navGridById(world, parent);
    if (!from || !to) return false;
    bool goalCase = parent == grid;
    if (!goalCase) {
        const std::uint8_t state = record.gridNodes[parent].state;
        if (state == 0) goalCase = true;
        else if (state == 1 && !navEntryCell(world, record, parent)) return false;
    }
    const int cells = static_cast<int>(cellCount(*from));
    if (goalCase) {
        const int value = record.goalGrid == grid ? record.goalCell : cells / 2;
        record.entryCell[grid] = static_cast<std::uint8_t>(value);
        return true;
    }
    if (to->width == 0 || from->width == 0) return false;
    const int entry = record.entryCell[parent];
    const int targetX = entry % to->width + to->x;
    const int targetZ = entry / to->width + to->z;
    int best = -1;
    int bestDistance = 0x7fffffff;
    const NavGridState* state = navGridStateById(world, grid);
    for (int cell = 0; cell < cells; ++cell) {
        const std::uint8_t flags = cellFlags(*from, std::size_t(cell));
        if (!(flags & 4)) continue;
        const std::size_t extraIndex = flags >> 3;
        if (!state || extraIndex >= state->extras.size()) continue;
        if (from->linkSlots[state->extras[extraIndex][0] & 63u & 15u] != parent) continue;
        const int dx = cell % from->width + from->x - targetX;
        const int dz = cell / from->width + from->z - targetZ;
        const int distance = dx * dx + dz * dz;
        if (distance < bestDistance) {
            bestDistance = distance;
            best = cell;
        }
    }
    record.entryCell[grid] = static_cast<std::uint8_t>(best >= 0 ? best : cells / 2);
    return true;
}

// 0x294768
bool navGridStep(NavWorld& world, NavPathRecord& record) {
    if (record.gridOpen.empty()) return false;
    const std::uint16_t u = record.gridOpen.front();
    record.gridOpen.erase(record.gridOpen.begin());
    NavGridNode& node = record.gridNodes[u];
    node.state = 3;
    if (node.parent == u) {
        record.entryCell[u] = static_cast<std::uint8_t>(record.goalCell);
    } else if (!navEntryCell(world, record, u)) {
        return false;
    }
    const int key = node.key;
    const NavGrid* grid = navGridById(world, u);
    if (!grid) return false;
    auto& nodes = record.gridNodes;
    if (node.cost > key) {
        for (int k = 0; k < 16; ++k) {
            const std::uint16_t n = grid->linkSlots[std::size_t(k)];
            if (n == kNavNone) break;
            const int cost = navEdgeCost(world, record, u, n);
            if (nodes[n].state == 0) continue;
            const int neighbourCost = nodes[n].cost;
            if (key <= neighbourCost) continue;
            const int candidate = minCap(neighbourCost + cost);
            if (node.cost <= candidate) continue;
            node.parent = n;
            node.cost = static_cast<std::uint16_t>(minCap(nodes[n].cost + cost));
            if (node.state == 2) node.state = 1;
        }
    }
    if (kGridSearchLimit <= key) return !record.gridOpen.empty();
    if (node.cost == key) {
        for (int k = 0; k < 16; ++k) {
            const std::uint16_t n = grid->linkSlots[std::size_t(k)];
            if (n == kNavNone) break;
            const int cost = navEdgeCost(world, record, n, u);
            const bool reassign = [&] {
                if (nodes[n].state == 0) return true;
                const int wanted = minCap(node.cost + cost);
                if (nodes[n].parent == u) return nodes[n].cost != wanted;
                return nodes[n].cost > wanted;
            }();
            if (!reassign) continue;
            nodes[n].parent = u;
            navInsertGridNode(record, n, minCap(node.cost + cost));
            if (nodes[n].state == 2) nodes[n].state = 1;
        }
    } else {
        for (int k = 0; k < 16; ++k) {
            const std::uint16_t n = grid->linkSlots[std::size_t(k)];
            if (n == kNavNone) break;
            const int cost = navEdgeCost(world, record, n, u);
            const std::uint8_t state = nodes[n].state;
            const std::uint16_t parent = nodes[n].parent;
            bool reassign = state == 0;
            if (!reassign && parent == u) reassign = nodes[n].cost != minCap(node.cost + cost);
            if (reassign) {
                nodes[n].parent = u;
                navInsertGridNode(record, n, minCap(node.cost + cost));
                if (nodes[n].state == 2) nodes[n].state = 1;
                continue;
            }
            if (parent != u && nodes[n].cost > minCap(node.cost + cost)) {
                navInsertGridNode(record, u, node.cost);
                continue;
            }
            if (parent == u) continue;
            if (node.cost > minCap(nodes[n].cost + cost) && state == 3 && nodes[n].cost > key)
                navInsertGridNode(record, n, nodes[n].cost);
        }
    }
    return !record.gridOpen.empty();
}

// 0x27d90c
bool navSelectExitCell(NavWorld& world, NavPathRecord& record) {
    record.exitCell = kNavNone;
    const std::uint16_t parent = record.gridNodes[record.startGrid].parent;
    if (record.startGrid == parent) return false;
    const NavGrid* to = navGridById(world, parent);
    if (!to) return false;
    if (record.gridNodes[parent].state == 1 && !navEntryCell(world, record, parent)) return false;
    const NavGrid* from = navGridById(world, record.startGrid);
    const NavGridState* fromState = navGridStateById(world, record.startGrid);
    const NavGridState* toState = navGridStateById(world, parent);
    if (!from || !fromState || !toState || from->width == 0 || to->width == 0) return false;
    const bool recent = world.now <= toState->stamp + 1.0f;
    const int entry = record.entryCell[parent];
    const int targetX = entry % to->width + to->x;
    const int targetZ = entry / to->width + to->z;
    const int startX = record.startCell % from->width + from->x;
    const int startZ = record.startCell / from->width + from->z;
    int best = 0x7fffffff;
    const int cells = static_cast<int>(cellCount(*from));
    for (int cell = 0; cell < cells; ++cell) {
        const std::uint8_t flags = cellFlags(*from, std::size_t(cell));
        if (!(flags & 4)) continue;
        if (!cellCrossable(record, flags)) continue;
        const std::size_t extraIndex = flags >> 3;
        if (extraIndex >= fromState->extras.size()) continue;
        const auto& extra = fromState->extras[extraIndex];
        if (from->linkSlots[extra[0] & 63u & 15u] != parent) continue;
        if (extra[0] & 64) continue;
        const int cellX = cell % from->width + from->x;
        const int cellZ = cell / from->width + from->z;
        const int across = (cellZ + static_cast<std::int8_t>(extra[3]) - to->z) * to->width + (cellX + static_cast<std::int8_t>(extra[2]) - to->x);
        if (recent) {
            if (across < 0 || std::size_t(across) >= to->cells.size()) continue;
            if (!cellCrossable(record, to->cells[std::size_t(across)])) continue;
        }
        const int toStart = (cellX - startX) * (cellX - startX) + (cellZ - startZ) * (cellZ - startZ);
        const int toTarget = (cellX - targetX) * (cellX - targetX) + (cellZ - targetZ) * (cellZ - targetZ);
        int total;
        const unsigned gate = extra[1];
        if (world.gateWeights.empty()) {
            total = toStart + toTarget;
        } else {
            const float weight = gate < world.gateWeights.size() ? world.gateWeights[gate] : 0.0f;
            if (weight > 0.0f) total = toStart + static_cast<int>(static_cast<float>(toTarget) * weight);
            else total = toStart - static_cast<int>(weight);
        }
        if (gate != 0 && !gateOpen(record.gateMask, gate)) total += 250000;
        if (total < best) {
            best = total;
            record.exitCell = static_cast<std::uint16_t>(cell);
        }
    }
    return true;
}

// 0x215c08
void navInsertCellNode(NavWorld& world, NavPathRecord& record, int cell, int cost) {
    NavCellNode& node = record.cellNodes[std::size_t(cell)];
    const std::uint16_t stored = static_cast<std::uint16_t>(cost);
    int key = cost;
    if (node.state == 0) {
        node.key = stored;
    } else if (node.state == 1) {
        if (node.key < static_cast<unsigned>(cost)) key = node.key;
        node.key = static_cast<std::uint16_t>(key);
        removeFirst(record.cellOpen, static_cast<std::uint8_t>(cell));
    } else if (node.state == 3) {
        if (node.cost < static_cast<unsigned>(cost)) key = node.cost;
        node.key = static_cast<std::uint16_t>(key);
    }
    const NavGrid* grid = navGridById(world, record.startGrid);
    int straight = 0;
    if (grid && grid->width) {
        const int width = grid->width;
        const int dx = cell % width - record.startCell % width;
        const int dz = cell / width - record.startCell / width;
        straight = static_cast<int>(std::sqrt(static_cast<float>(dz * dz + dx * dx)));
    }
    node.total = static_cast<std::uint16_t>(node.key + straight);
    node.cost = stored;
    node.state = 1;
    std::size_t low = 0;
    std::size_t count = record.cellOpen.size();
    while (count > 0) {
        const std::size_t half = count / 2;
        const NavCellNode& other = record.cellNodes[record.cellOpen[low + half]];
        bool right;
        if (node.total > other.total) right = true;
        else if (node.total < other.total) right = false;
        else right = !(node.key < other.key);
        if (right) {
            low += half + 1;
            count -= half + 1;
        } else {
            count = half;
        }
    }
    record.cellOpen.insert(record.cellOpen.begin() + static_cast<std::ptrdiff_t>(low), static_cast<std::uint8_t>(cell));
}

// 0x215e08
int navProcessCell(NavWorld& world, NavPathRecord& record, int x, int z, int previous, int extra) {
    const NavGrid& grid = *navGridById(world, record.startGrid);
    const int index = z * grid.width + x;
    const int previousFlags = cellFlags(grid, std::size_t(previous));
    if (!cellCrossable(record, previousFlags)) extra = (previousFlags & 3) == 1 ? kCostBlocked : kCostSoft;
    const int base = record.cellNodes[std::size_t(previous)].cost;
    navInsertCellNode(world, record, index, minCap(base + extra));
    const int flags = cellFlags(grid, std::size_t(index));
    if (cellCrossable(record, flags)) {
        removeFirst(record.cellOpen, static_cast<std::uint8_t>(index));
        record.cellNodes[std::size_t(index)].state = 3;
    }
    record.cellNodes[std::size_t(index)].parent = static_cast<std::uint8_t>(previous);
    for (int k = 0; k < 8; ++k) {
        const int nx = kDx[k] + x;
        if (nx < 0 || grid.width <= nx) continue;
        const int nz = kDz[k] + z;
        if (nz < 0 || grid.depth <= nz) continue;
        const int neighbour = record.neighbour[std::size_t(k)] + index;
        NavCellNode& node = record.cellNodes[std::size_t(neighbour)];
        if (node.state != 0) continue;
        node.parent = static_cast<std::uint8_t>(index);
        const int cost = cellStepCost(record, flags, cellFlags(grid, std::size_t(neighbour)), k);
        navInsertCellNode(world, record, neighbour, minCap(record.cellNodes[std::size_t(index)].cost + cost));
    }
    return index;
}

// 0x294b74
bool navCellStep(NavWorld& world, NavPathRecord& record) {
    if (record.cellOpen.empty()) return false;
    const int u = record.cellOpen.front();
    record.cellOpen.erase(record.cellOpen.begin());
    auto& nodes = record.cellNodes;
    nodes[std::size_t(u)].state = 3;
    const NavGrid& grid = *navGridById(world, record.startGrid);
    const int width = grid.width;
    const int depth = grid.depth;
    const int key = nodes[std::size_t(u)].key;
    const int ux = u % width;
    const int uz = u / width;
    NavCellNode& node = nodes[std::size_t(u)];
    const auto inside = [&](int k) {
        const int nx = kDx[k] + ux;
        const int nz = kDz[k] + uz;
        return nx >= 0 && width > nx && nz >= 0 && depth > nz;
    };
    if (node.cost > key) {
        for (int k = 0; k < 8; ++k) {
            if (!inside(k)) continue;
            const int n = record.neighbour[std::size_t(k)] + u;
            const int cost = cellStepCost(record, cellFlags(grid, std::size_t(n)), cellFlags(grid, std::size_t(u)), k);
            if (nodes[std::size_t(n)].state == 0) continue;
            const int neighbourCost = nodes[std::size_t(n)].cost;
            if (key <= neighbourCost) continue;
            if (node.cost <= minCap(neighbourCost + cost)) continue;
            node.parent = static_cast<std::uint8_t>(n);
            node.cost = static_cast<std::uint16_t>(minCap(nodes[std::size_t(n)].cost + cost));
        }
    }
    if (key >= kGridSearchLimit) return !record.cellOpen.empty();
    if (node.cost == key) {
        for (int k = 0; k < 8; ++k) {
            if (!inside(k)) continue;
            const int n = record.neighbour[std::size_t(k)] + u;
            const int cost = cellStepCost(record, cellFlags(grid, std::size_t(u)), cellFlags(grid, std::size_t(n)), k);
            NavCellNode& other = nodes[std::size_t(n)];
            const int wanted = minCap(node.cost + cost);
            bool reassign = other.state == 0;
            if (!reassign) reassign = other.parent == u ? other.cost != wanted : other.cost > wanted;
            if (!reassign) continue;
            other.parent = static_cast<std::uint8_t>(u);
            navInsertCellNode(world, record, n, wanted);
        }
    } else {
        for (int k = 0; k < 8; ++k) {
            if (!inside(k)) continue;
            const int n = record.neighbour[std::size_t(k)] + u;
            const int cost = cellStepCost(record, cellFlags(grid, std::size_t(u)), cellFlags(grid, std::size_t(n)), k);
            NavCellNode& other = nodes[std::size_t(n)];
            const std::uint8_t state = other.state;
            const int parent = other.parent;
            const int wanted = minCap(node.cost + cost);
            bool reassign = state == 0;
            if (!reassign && parent == u) reassign = other.cost != wanted;
            if (reassign) {
                other.parent = static_cast<std::uint8_t>(u);
                navInsertCellNode(world, record, n, wanted);
                continue;
            }
            if (parent == u) continue;
            if (other.cost > wanted) {
                navInsertCellNode(world, record, u, node.cost);
                continue;
            }
            if (node.cost > minCap(other.cost + cost) && state == 3 && other.cost > key) navInsertCellNode(world, record, n, other.cost);
        }
    }
    return !record.cellOpen.empty();
}

// 0x27dc98: marks every cell of the chain from the start cell with the cell where its straight run ends.
void navMarkStraightRuns(NavPathRecord& record) {
    auto& nodes = record.cellNodes;
    int segmentStart = record.startCell;
    int cursor = segmentStart;
    int previousDirection = -1;
    int hare = nodes[std::size_t(cursor)].parent;
    const auto markRun = [&](int from, int end) {
        int cell = from;
        std::size_t guard = 0;
        do {
            nodes[std::size_t(cell)].mark = static_cast<std::uint8_t>(end);
            cell = nodes[std::size_t(cell)].parent;
        } while (cell != end && ++guard < kNavMaxCellNodes);
    };
    while (nodes[std::size_t(cursor)].parent != cursor && cursor != hare) {
        const int parent = nodes[std::size_t(cursor)].parent;
        int direction = -1;
        for (int k = 0; k < 8; ++k) {
            if (record.neighbour[std::size_t(k)] + cursor == parent) {
                direction = k;
                break;
            }
        }
        if (previousDirection != direction) {
            if (segmentStart != cursor) markRun(segmentStart, cursor);
            segmentStart = cursor;
            previousDirection = direction;
        }
        hare = nodes[std::size_t(nodes[std::size_t(hare)].parent)].parent;
        cursor = parent;
    }
    if (segmentStart != cursor) markRun(segmentStart, cursor);
    nodes[std::size_t(cursor)].mark = static_cast<std::uint8_t>(cursor);
}

namespace {

// 0x1d05d0
void resetGrids(NavWorld& world) {
    for (auto& room : world.rooms) {
        for (std::size_t i = 0; i < room.mesh.grids.size(); ++i) {
            const NavGrid& grid = room.mesh.grids[i];
            NavGridState& state = room.state[i];
            state.linkMask = 0xffff;
            const std::size_t count = std::min<std::size_t>(grid.flags & 0x3fffu, state.extras.size());
            for (std::size_t e = 0; e < count; ++e) {
                const unsigned slot = state.extras[e][0] & 63u;
                if (slot < 16) state.linkMask = static_cast<std::uint16_t>(state.linkMask & ~(1u << slot));
            }
            if (world.now - 2.0f > state.stamp)
                for (std::size_t e = 0; e < count; ++e) state.extras[e][0] = static_cast<std::uint8_t>(state.extras[e][0] & ~64u);
        }
    }
}

// 0x208338 with the flag argument zero (the dynamic obstacle pass 0x2af1cc is skipped).
void buildLegs(NavWorld& world, NavPathRecord& record) {
    NavGrid* grid = navGridById(world, record.startGrid);
    NavGridState* gridState = navGridStateById(world, record.startGrid);
    if (!grid || !gridState || grid->width == 0) return;
    const int width = grid->width;
    record.neighbour = {static_cast<std::int8_t>(~width), static_cast<std::int8_t>(-width), static_cast<std::int8_t>(1 - width),
                        static_cast<std::int8_t>(-1), 1, static_cast<std::int8_t>(width - 1), static_cast<std::int8_t>(width),
                        static_cast<std::int8_t>(width + 1)};
    if (gridState->stamp + 1.0f < world.now) {
        for (auto& cell : grid->cells)
            if ((cell & 3) == 2) cell = static_cast<std::uint8_t>(cell & ~3);
    }
    auto& nodes = record.cellNodes;
    nodes[record.startCell].state = 0;
    if (record.startGrid == record.goalGrid) record.exitCell = record.goalCell;
    else if (!navSelectExitCell(world, record)) return;
    while (record.exitCell == kNavNone) {
        const std::uint16_t startGrid = record.startGrid;
        const std::uint16_t parent = record.gridNodes[startGrid].parent;
        for (int k = 0; k < 16; ++k) {
            if (grid->linkSlots[std::size_t(k)] == parent) {
                gridState->linkMask = static_cast<std::uint16_t>(gridState->linkMask | (1u << k));
                break;
            }
        }
        if (record.gridNodes[startGrid].state == 3) {
            const int cost = record.gridNodes[startGrid].cost;
            navInsertGridNode(record, startGrid, cost <= kCostSoft ? kCostSoft : cost);
        }
        if (record.gridNodes[parent].state == 3) navInsertGridNode(record, parent, record.gridNodes[parent].cost);
        while (!record.gridOpen.empty()) {
            if (record.gridNodes[startGrid].state == 3 &&
                !(record.gridNodes[record.gridOpen.front()].key < record.gridNodes[startGrid].cost))
                break;
            navGridStep(world, record);
        }
        if (record.gridNodes[startGrid].parent == parent) return;
        if (!navSelectExitCell(world, record)) return;
    }
    for (auto& node : nodes) node.state = 0;
    record.cellOpen.clear();
    const int exit = record.exitCell;
    const int exitX = exit % width;
    const int exitZ = exit / width;
    const int startX = record.startCell % width;
    const int startZ = record.startCell / width;
    nodes[std::size_t(exit)].cost = 0;
    bool clear = true;
    const int dx = startX - exitX;
    const int dz = startZ - exitZ;
    int previous = exit;
    navProcessCell(world, record, exitX, exitZ, previous, 0);
    const int absX = std::abs(dx);
    const int absZ = std::abs(dz);
    const auto sign = [](int value) { return value >= 1 ? 1 : (value <= -1 ? -1 : 0); };
    const auto visit = [&](int x, int z, int mode) {
        previous = navProcessCell(world, record, x, z, previous, mode);
        if (!cellCrossable(record, cellFlags(*grid, std::size_t(previous)))) clear = false;
    };
    int x = exitX;
    int z = exitZ;
    if (absZ > absX) {
        const int diagonal = 2 * (absX - absZ);
        const int straight = 2 * absX;
        int error = 2 * absX - absZ;
        const int stepX = sign(dx);
        const int stepZ = dz >= 1 ? 1 : (dz > -1 ? dz : -1);
        while (z != startZ) {
            int mode;
            if (error >= 0 && stepX != 0) {
                x += stepX;
                error += diagonal;
                mode = 3;
            } else {
                error += straight;
                mode = 2;
            }
            z += stepZ;
            visit(x, z, mode);
        }
    } else {
        const int diagonal = 2 * (absZ - absX);
        const int straight = 2 * absZ;
        int error = 2 * absZ - absX;
        const int stepX = sign(dx);
        const int stepZ = sign(dz);
        while (x != startX) {
            int mode;
            if (error >= 0 && stepZ != 0) {
                error += diagonal;
                x += stepX;
                z += stepZ;
                mode = 3;
            } else {
                error += straight;
                x += stepX;
                mode = 2;
            }
            visit(x, z, mode);
        }
    }
    if (record.exitCell != record.startCell) {
        int cell = record.startCell;
        std::size_t guard = 0;
        do {
            nodes[std::size_t(cell)].mark = static_cast<std::uint8_t>(record.exitCell);
            cell = nodes[std::size_t(cell)].parent;
        } while (cell != record.exitCell && ++guard < kNavMaxCellNodes);
    }
    nodes[record.exitCell].mark = static_cast<std::uint8_t>(record.exitCell);
    if (clear) return;
    if (record.cellOpen.empty()) navInsertCellNode(world, record, record.goalCell, 0);
    for (;;) {
        if (record.cellOpen.empty()) break;
        const NavCellNode& start = nodes[record.startCell];
        if (start.state == 3 && !(nodes[record.cellOpen.front()].total < start.cost)) break;
        navCellStep(world, record);
    }
    if (nodes[record.startCell].state != 3) return;
    if (kCostBlocked <= nodes[record.startCell].cost) return;
    navMarkStraightRuns(record);
}

// 0x1d0700 as called by 0x204284.
void searchGrids(NavWorld& world, NavPathRecord& record) {
    const std::uint16_t total = navTotalGrids(world);
    for (std::uint16_t i = 0; i < total && i < record.gridNodes.size(); ++i) record.gridNodes[i].state = 0;
    record.gridOpen.clear();
    record.gridNodes[record.goalGrid].parent = record.goalGrid;
    navInsertGridNode(record, record.goalGrid, 0);
    if (record.gridNodes[record.startGrid].state != 3) {
        do {
            if (!navGridStep(world, record)) break;
        } while (record.gridNodes[record.startGrid].state != 3);
    }
    if (NavGridState* state = navGridStateById(world, record.startGrid)) state->stamp = 0.0f;
    buildLegs(world, record);
}

}

void navResetGrids(NavWorld& world) { resetGrids(world); }

void navSearchGrids(NavWorld& world, NavPathRecord& record) { searchGrids(world, record); }

void navBuildLegs(NavWorld& world, NavPathRecord& record) { buildLegs(world, record); }

// 0x204284
int navFindPath(NavWorld& world, NavPathRecord& record, const float start[3], const float goal[3]) {
    record.startGrid = kNavNone;
    record.goalGrid = kNavNone;
    record.diagnostic = 6;
    const auto fail = [&] {
        record.phase = 1;
        return 1;
    };
    if (world.rooms.empty()) return fail();
    const float tolerance = record.heightTolerance * 2.0f;
    const NavLocation from = navLocate(world, start, tolerance, true);
    record.startRoom = from.room;
    record.startGrid = from.grid;
    record.startCell = from.cell;
    const NavLocation to = navLocate(world, goal, tolerance, false);
    record.goalRoom = to.room;
    record.goalGrid = to.grid;
    record.goalCell = to.cell;
    if (record.startGrid == kNavNone || record.goalGrid == kNavNone) {
        if (from.nearGrid < 0) return fail();
        if (record.goalGrid == kNavNone) return fail();
        record.startGrid = static_cast<std::uint16_t>(from.nearGrid);
        const NavGrid* nearGrid = navGridById(world, record.startGrid);
        if (!nearGrid) return fail();
        if (nearGrid->flags & 0x8000) return 3;
        if ((nearGrid->flags & 0x4000) && !(record.avoidMask & 1)) return 3;
        record.phase = 4;
        return 3;
    }
    const NavGrid* startGrid = navGridById(world, record.startGrid);
    if (!startGrid) return fail();
    if (startGrid->flags & 0x8000) return 3;
    if ((startGrid->flags & 0x4000) && !(record.avoidMask & 1)) return 3;
    if (record.startGrid == record.goalGrid && record.startCell == record.goalCell) {
        record.phase = 0;
        return 0;
    }
    resetGrids(world);
    searchGrids(world, record);
    if (record.cellNodes[record.startCell].state == 3) {
        record.phase = 2;
        return 3;
    }
    const NavGridNode& node = record.gridNodes[record.startGrid];
    if (node.state != 3) return fail();
    if (node.cost < kCostSoft) {
        record.diagnostic = 2;
        return fail();
    }
    const NavGridState* state = navGridStateById(world, record.startGrid);
    for (int slot = 0; slot < 16; ++slot) {
        if (startGrid->linkSlots[std::size_t(slot)] != node.parent) continue;
        const unsigned count = startGrid->flags & 0x3fffu;
        for (unsigned i = 0; state && i < count && i < state->extras.size(); ++i) {
            const auto& extra = state->extras[i];
            if ((extra[0] & 63) != slot) continue;
            if (extra[1] == 0) return fail();
            if (gateOpen(record.avoidMask, extra[1])) return fail();
        }
        record.phase = 1;
        return 2;
    }
    return fail();
}

}
