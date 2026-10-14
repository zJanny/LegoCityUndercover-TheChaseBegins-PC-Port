// AI path finder 0x204284 (src/runtime/nav_path.cpp, docs/re/ai-navigation.md). Every expected value is evaluated by hand
// from the disassembly (addresses next to the checks) on small made-up grids; no original data is used and no run of the
// original was compared (the oracle was not available for this step).
#include "runtime/nav_follow.hpp"
#include "runtime/nav_path.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;
using chase::assets::NavGrid;
using chase::assets::NavMesh;
using chase::assets::NavPath;
using chase::assets::NavRoom;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

// Unit cells (cell size 1, height unit 1, origin 0): a cell (x, z) is the world square x .. x + 1, z .. z + 1; the sample
// positions stand at y 1, inside the grid box (height 0, extent 512: y 0 .. 2).
NavGrid makeGrid(int x, int z, int width, int depth) {
    NavGrid grid;
    grid.x = static_cast<std::uint16_t>(x);
    grid.z = static_cast<std::uint16_t>(z);
    grid.width = static_cast<std::uint16_t>(width);
    grid.depth = static_cast<std::uint16_t>(depth);
    grid.height = 0;
    grid.heightExtent = 512;
    grid.cells.assign(std::size_t(width) * depth, 0);
    grid.linkSlots.fill(0xffff);
    return grid;
}

NavWorld makeWorld(std::vector<NavGrid> grids, std::uint16_t base) {
    NavMesh mesh;
    NavPath box;
    box.position[0] = box.position[2] = 50;
    box.position[1] = 1;
    box.extent[0] = box.extent[1] = box.extent[2] = 100;
    for (std::size_t i = 0; i < grids.size(); ++i) box.indices.push_back(static_cast<std::uint16_t>(i));
    mesh.grids = std::move(grids);
    mesh.paths.push_back(box);
    NavRoom descriptor;
    descriptor.heightUnit = 1;
    descriptor.cellSize = 1;
    descriptor.base = base;
    descriptor.end = static_cast<std::uint16_t>(base + mesh.grids.size());
    return NavWorld::fromRooms({descriptor}, {mesh});
}

void at(float out[3], float x, float z) {
    out[0] = x;
    out[1] = 1;
    out[2] = z;
}

void sameCell() {
    NavWorld world = makeWorld({makeGrid(0, 0, 6, 4)}, 0);
    NavPathRecord record;
    float start[3], goal[3];
    at(start, 1.5f, 1.5f);
    at(goal, 1.7f, 1.2f);
    require(navFindPath(world, record, start, goal) == 0, "both positions in cell 7: arrived (0x204284 0x204388)");
    require(record.phase == 0 && record.startCell == 7 && record.goalCell == 7 && record.startGrid == 0, "phase 0 (strb r11, [r4, #0x7e])");
}

void straightLineInOneGrid() {
    NavWorld world = makeWorld({makeGrid(0, 0, 6, 4)}, 0);
    NavPathRecord record;
    float start[3], goal[3];
    at(start, 0.5f, 0.5f);
    at(goal, 4.5f, 2.5f);
    require(navFindPath(world, record, start, goal) == 3 && record.phase == 2, "within a grid: result 3, phase 2 (0x2043e8)");
    require(record.startCell == 0 && record.goalCell == 16 && record.exitCell == 16, "the exit cell of a single grid is the goal cell (0x2084b8)");
    // Line from the exit (4, 2) to the start (0, 0), x major: 0x2087e4 error 2 * 2 - 4 = 0, diagonal 2 * (2 - 4), straight 4;
    // the cells are (3,1) (2,1) (1,0) (0,0) = 9, 8, 1, 0; each step costs 3 (diagonal) or 2 (straight) from the table 0x376c0c.
    const auto& nodes = record.cellNodes;
    require(nodes[0].parent == 1 && nodes[1].parent == 8 && nodes[8].parent == 9 && nodes[9].parent == 16 && nodes[16].parent == 16,
            "parent chain 0 -> 1 -> 8 -> 9 -> 16 (0x215f70)");
    require(nodes[9].cost == 3 && nodes[8].cost == 5 && nodes[1].cost == 8 && nodes[0].cost == 10, "costs 3, 5, 8, 10 (0x215e84)");
    require(nodes[0].mark == 16 && nodes[1].mark == 16 && nodes[8].mark == 16 && nodes[9].mark == 16 && nodes[16].mark == 16,
            "every cell of the clear line is marked with the exit cell (0x2088e8)");
    require(nodes[0].state == 3, "the start cell node is closed");
}

void searchAroundAWall() {
    NavGrid grid = makeGrid(0, 0, 3, 3);
    grid.cells[1] = 1;
    NavWorld world = makeWorld({grid}, 0);
    NavPathRecord record;
    float start[3], goal[3];
    at(start, 0.5f, 0.5f);
    at(goal, 2.5f, 0.5f);
    require(navFindPath(world, record, start, goal) == 3 && record.phase == 2, "the wall cell forces the cell search");
    // Exit (2, 0) -> (1, 1) costs 3 (offset +w-1, table[5]) -> (0, 0) costs 3 (table[0]): 6 against 3 + 2 + 2 = 7 over (0, 1).
    const auto& nodes = record.cellNodes;
    require(nodes[0].cost == 6 && nodes[0].parent == 4 && nodes[4].parent == 2, "chain 0 -> 4 -> 2 of cost 6 (0x294b74)");
    // 0x27dc98: direction 7 (+w+1) from cell 0, direction 2 (1-w) from cell 4; the runs end at the corner cell 4 and the exit cell 2.
    require(nodes[0].mark == 4 && nodes[4].mark == 2 && nodes[2].mark == 2, "straight runs end at the corners (0x27dd70)");
}

// Grid 100 (x 0..3, z 0..1) and grid 101 (x 4..7, z 0..1). Edge cells carry flag bit 2 and 3 * extra index: the cells at
// x 3 of grid 100 name extras 0 and 1, the cells at x 4 of grid 101 name extras 0 and 1; every extra links slot 0, gate 0,
// and crosses one cell (offset +1 / -1 in x).
NavWorld twoGrids(bool linked) {
    NavGrid left = makeGrid(0, 0, 4, 2);
    NavGrid right = makeGrid(4, 0, 4, 2);
    if (linked) {
        left.linkSlots[0] = 101;
        right.linkSlots[0] = 100;
        left.flags = 2;
        right.flags = 2;
        left.extras = {{0, 0, 1, 0}, {0, 0, 1, 0}};
        right.extras = {{0, 0, 0xff, 0}, {0, 0, 0xff, 0}};
        left.cells[3] = 4;
        left.cells[7] = 12;
        right.cells[0] = 4;
        right.cells[4] = 12;
    }
    return makeWorld({left, right}, 100);
}

void acrossTwoGrids() {
    NavWorld world = twoGrids(true);
    NavPathRecord record;
    float start[3], goal[3];
    at(start, 0.5f, 0.5f);
    at(goal, 7.5f, 1.5f);
    require(navFindPath(world, record, start, goal) == 3 && record.phase == 2, "a path across two grids ends with the cell search of the first grid");
    require(record.startGrid == 100 && record.goalGrid == 101 && record.startCell == 0 && record.goalCell == 7, "lookups (0x1c2ccc)");
    // Grid cost 0x1f9ea0: centres (2, 1, 1) and (6, 1, 1) are 4 apart; 4 * 0.5 * 2.0 = 4.
    require(record.gridNodes[101].state == 3 && record.gridNodes[101].parent == 101 && record.gridNodes[101].cost == 0, "the goal grid is the root (0x1d0764)");
    require(record.gridNodes[100].state == 3 && record.gridNodes[100].parent == 101 && record.gridNodes[100].cost == 4, "grid 100 costs 4 (0x294768)");
    // Entry cells (0x1fc6a4): the goal grid enters at the goal cell 7; in grid 100 the edge cell nearest to (7, 1) is cell 7 (16) not cell 3 (17).
    require(record.entryCell[101] == 7 && record.entryCell[100] == 7, "entry cells 7 and 7");
    // Exit cell (0x27d90c): cells 3 and 7 both total 26 (9 + 17 and 10 + 16); the first one found keeps it.
    require(record.exitCell == 3, "exit cell 3");
    require(record.cellNodes[0].cost == 6 && record.cellNodes[0].mark == 3, "the cell line to the exit: three straight steps of 2");
}

void unlinkedGridsFail() {
    NavWorld world = twoGrids(false);
    NavPathRecord record;
    float start[3], goal[3];
    at(start, 0.5f, 0.5f);
    at(goal, 7.5f, 1.5f);
    require(navFindPath(world, record, start, goal) == 1 && record.phase == 1, "no link: result 1, phase 1 (0x204504)");
}

NavWorld corridor() {
    NavGrid grid = makeGrid(0, 0, 4, 2);
    grid.flags = 1;
    grid.extras = {{0, 0, 0, 0}};
    return makeWorld({grid}, 0);
}

void followsAlongTheCorridorAndArrives() {
    NavWorld world = corridor();
    NavPathRecord record;
    record.avoidFlags = 1;
    float start[3], goal[3];
    at(start, 0.5f, 0.5f);
    at(goal, 3.5f, 0.5f);
    require(navFindPath(world, record, start, goal) == 3 && record.exitCell == 3 && record.cellNodes[0].cost == 6 &&
                record.cellNodes[0].mark == 3,
            "straight corridor: exit = goal cell 3, cost 3 * 2, mark 3");
    float position[3];
    at(position, 0.5f, 0.5f);
    NavFollowResult step = navFollow(world, record, position);
    // 0x27e214..0x27e2e4: waypoint = mark of the start cell = 3, its centre 1 * 3 + 0.5, 0.5; heading atan2(3, 0) * 128 / pi = 64.
    require(step.code == 4 && step.heading == 64, "walks toward the exit: code 4, heading 64");
    require(step.target[0] == 3.5f && step.target[1] == 0.0f && step.target[2] == 0.5f, "target = centre of cell 3, y of the grid base (+0x58)");
    at(position, 1.5f, 0.5f);
    require(navFollow(world, record, position).code == 4 && record.startCell == 0, "cell 1 is not the mark of the start cell 0 (0x27df14)");
    at(position, 3.5f, 0.5f);
    step = navFollow(world, record, position);
    require(step.code == 0 && record.phase == 0, "standing on the exit cell of the goal grid: arrived, phase 0 (0x27df44)");
    require(navFollow(world, record, position).code == 0, "phase 0 keeps returning 0 (0x1faf04 0x1faf2c)");
}

void followerReportsLeavingTheGrid() {
    NavWorld world = corridor();
    NavPathRecord record;
    record.avoidFlags = 1;
    float start[3], goal[3], position[3];
    at(start, 0.5f, 0.5f);
    at(goal, 3.5f, 0.5f);
    navFindPath(world, record, start, goal);
    at(position, 9.5f, 0.5f);
    require(navFollow(world, record, position).code == 1 && record.phase == 1 && record.diagnostic == 4, "outside the grid: code 1, phase 1 (0x27deac)");
}

void temporaryMarkOnTheExitExtra() {
    NavWorld world = corridor();
    world.rooms[0].state[0].extras[0][0] = 64;
    NavPathRecord record;
    record.avoidFlags = 1;
    float start[3], goal[3], position[3];
    at(start, 0.5f, 0.5f);
    at(goal, 3.5f, 0.5f);
    navFindPath(world, record, start, goal);
    world.rooms[0].state[0].extras[0][0] = 64;
    at(position, 3.5f, 0.5f);
    const NavFollowResult step = navFollow(world, record, position);
    require(step.code == 3 && record.startCell == 3 && record.phase == 2,
            "extra with bit 6: no arrival (0x27df0c tst r2, #64), the start cell moves to the mark 3 and the exit test returns 3");
}

void positionOutsideEveryGrid() {
    NavWorld world = makeWorld({makeGrid(0, 0, 6, 4)}, 0);
    NavPathRecord record;
    float start[3], goal[3];
    at(start, 0.5f, 0.5f);
    at(goal, 20.5f, 0.5f);
    require(navFindPath(world, record, start, goal) == 1 && record.phase == 1, "goal outside every grid: failed");
    require(record.goalGrid == kNavNone && record.goalCell == kNavNone, "the outputs stay 0xffff");
}

}

int main() {
    try {
        sameCell();
        straightLineInOneGrid();
        searchAroundAWall();
        acrossTwoGrids();
        unlinkedGridsFail();
        positionOutsideEveryGrid();
        followsAlongTheCorridorAndArrives();
        followerReportsLeavingTheGrid();
        temporaryMarkOnTheExitExtra();
    } catch (const std::exception& error) {
        std::cerr << "nav-path-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "nav-path-test: ok\n";
    return 0;
}
