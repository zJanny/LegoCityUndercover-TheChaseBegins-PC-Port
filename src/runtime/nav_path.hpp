#pragma once
// The AI path finder 0x204284 on the nav grids (docs/re/ai-navigation.md): both end points are looked up (0x1c2ccc);
// equal cells need no path; otherwise a search over the grids runs from the goal grid toward the start grid (0x1d0700,
// 0x294768, edge cost 0x1f9ea0, entry cell 0x1fc6a4) and a search over the cells of the start grid runs from the exit
// cell toward the start cell (0x208338, 0x294b74, 0x215e08, 0x215c08, post pass 0x27dc98). The result stays in the path
// record (0xc8 of the AI component, created by 0x206d18). STAND-INS: the time budget of the cell search (200 ticks of the
// stopwatch at path +0x10 while global 0x376bec byte 0 is zero) is not modelled, and the dynamic obstacle pass 0x2af1cc
// (reached only with a non-zero third argument of 0x1d0700, which 0x204284 never passes) is not ported. No SDL dependency.
#include <array>
#include <cstdint>
#include <vector>

#include "runtime/nav_grid.hpp"

namespace chase::runtime {

constexpr std::size_t kNavMaxGridNodes = 8192;  // allocation 0x10000 bytes at path +4, 8 bytes per node
constexpr std::size_t kNavMaxCellNodes = 256;   // allocation 2560 bytes at path +8, 10 bytes per node

struct NavGridNode {
    std::uint8_t state = 0;  // +0: 0 new, 1 open, 2 reopened, 3 closed
    std::uint16_t parent = 0;  // +2: the grid toward the goal
    std::uint16_t cost = 0;    // +4
    std::uint16_t key = 0;     // +6: sort key of the open list
};

struct NavCellNode {
    std::uint8_t state = 0;
    std::uint8_t parent = 0;  // +1: the cell toward the exit cell
    std::uint8_t mark = 0;    // +2: end cell of the straight run (0x27dc98) or the exit cell
    std::uint16_t cost = 0;   // +4
    std::uint16_t key = 0;    // +6
    std::uint16_t total = 0;  // +8: key plus the straight distance to the start cell
};

// Path record fields the finder reads and writes. Offsets are those of the original record.
struct NavPathRecord {
    std::uint16_t startRoom = kNavNone;  // +0x38
    std::uint16_t goalRoom = kNavNone;   // +0x3a
    std::uint16_t startGrid = kNavNone;  // +0x3c
    std::uint16_t goalGrid = kNavNone;   // +0x3e
    std::uint16_t startCell = kNavNone;  // +0x40
    std::uint16_t exitCell = kNavNone;   // +0x42
    std::uint16_t goalCell = kNavNone;   // +0x44
    std::uint32_t avoidMask = 0;         // +0x74: bit 0 lets the finder cross cells and grids marked 3 / bit 14
    std::uint32_t gateMask = 0;          // +0x78: bit n opens gate n of a grid link
    std::uint16_t avoidFlags = 0;        // +0x7c: bit 0 lets the finder cross cells marked 2 and grids with the temporary mark
    std::uint8_t phase = 1;              // +0x7e: 0 none, 1 blocked or failed, 2 within a grid, 3 after a grid transition, 4 start only near a grid
    std::array<float, 3> target{};       // +0x58: where the follower walks next
    // Class parameter +0x70 (read through the record's first word): twice it is the height tolerance of the lookups.
    float heightTolerance = 0;
    std::array<std::int8_t, 8> neighbour{};  // +0x64..+0x6b
    std::vector<NavGridNode> gridNodes;      // +4
    std::vector<NavCellNode> cellNodes;      // +8
    std::vector<std::uint8_t> entryCell;     // +0xc: per grid the cell where the path enters it
    std::vector<std::uint16_t> gridOpen;     // +0x4c, count at +0x48
    std::vector<std::uint8_t> cellOpen;      // +0x54, count at +0x50
    // Byte 1 of global 0x376bec after the last find (diagnostic code, 0x204284 sets it to 6 first).
    std::uint8_t diagnostic = 0;

    NavPathRecord();
};

// 0x204284: 0 arrived (same cell), 1 failed, 2 path across grids, 3 path within the grid (or a position handed over).
int navFindPath(NavWorld& world, NavPathRecord& record, const float start[3], const float goal[3]);

// Cell the path leads to next from `cell` of the start grid: the parent link of the cell node.
inline std::uint8_t navNextCell(const NavPathRecord& record, std::uint8_t cell) { return record.cellNodes[cell].parent; }

// Helpers of the search, exposed for the tests.
int navEdgeCost(NavWorld& world, const NavPathRecord& record, std::uint16_t from, std::uint16_t to);
bool navEntryCell(NavWorld& world, NavPathRecord& record, std::uint16_t grid);
bool navSelectExitCell(NavWorld& world, NavPathRecord& record);
void navInsertGridNode(NavPathRecord& record, std::uint16_t grid, int cost);
bool navGridStep(NavWorld& world, NavPathRecord& record);
void navInsertCellNode(NavWorld& world, NavPathRecord& record, int cell, int cost);
int navProcessCell(NavWorld& world, NavPathRecord& record, int x, int z, int previous, int extra);
bool navCellStep(NavWorld& world, NavPathRecord& record);
void navMarkStraightRuns(NavPathRecord& record);

}
