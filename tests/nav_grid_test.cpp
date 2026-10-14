// Navigation grid lookups (src/runtime/nav_grid.cpp, docs/re/ai-navigation.md). Every expected value is evaluated by
// hand from the code of 0x1c2ccc (position -> room, grid, cell), 0x20d8a8 (cell of a grid), 0x221de4 (grid by number)
// on a made-up grid; the addresses stand next to each check. No original data is used.
#include "runtime/nav_grid.hpp"
#include "runtime/nav_load.hpp"

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

// Section record: height unit 1, cell size 0.5, origin (10, 2, 20). Grid 100 (room 0, first of base 100): x 4, z 8,
// width 4, depth 2, height 256, height extent 512. Hand values: corner x 4 * 0.5 + 10 = 12, corner z 8 * 0.5 + 20 = 24;
// box centre x 10 + (4 + 4 * 0.5) * 0.5 = 13, half x 0.5 * (4 * 0.5) = 1; centre z 20 + (8 + 1) * 0.5 = 24.5, half z 0.5;
// y: centre 2 + 1 * ((256 + 256) / 256) = 4, half 1 * (512 / 512) = 1, so the box spans y 3 .. 5.
NavWorld sample(std::uint8_t cellSix = 0) {
    NavGrid grid;
    grid.x = 4;
    grid.z = 8;
    grid.width = 4;
    grid.depth = 2;
    grid.height = 256;
    grid.heightExtent = 512;
    grid.cells.assign(8, 0);
    grid.cells[6] = cellSix;
    NavPath box;
    box.position[0] = 13;
    box.position[1] = 4;
    box.position[2] = 24.5f;
    box.extent[0] = box.extent[1] = box.extent[2] = 3;
    box.indices = {0};
    NavMesh mesh;
    mesh.grids.push_back(grid);
    mesh.paths.push_back(box);
    NavRoom descriptor;
    descriptor.heightUnit = 1;
    descriptor.cellSize = 0.5f;
    descriptor.origin[0] = 10;
    descriptor.origin[1] = 2;
    descriptor.origin[2] = 20;
    descriptor.base = 100;
    descriptor.end = 101;
    return NavWorld::fromRooms({descriptor}, {mesh});
}

void gridNumbers() {
    NavWorld world = sample();
    require(navGridById(world, 100) == &world.rooms[0].mesh.grids[0], "grid 100 is the first grid (0x221de4)");
    require(!navGridById(world, 99), "below the room range (base <= id)");
    require(!navGridById(world, 101), "the end of the range is exclusive (id < end)");
}

void cellOfAGrid() {
    const NavWorld world = sample();
    const NavGrid& grid = world.rooms[0].mesh.grids[0];
    const float inside[3] = {13.2f, 4, 24.7f};
    require(navCellAt(world, grid, inside) == 6, "(13.2 - 12) / 0.5 = 2.4 -> 2, (24.7 - 24) / 0.5 = 1.4 -> 1, 1 * 4 + 2 (0x20d8a8)");
    const float first[3] = {12.1f, 4, 24.1f};
    require(navCellAt(world, grid, first) == 0, "cell 0");
    const float justBefore[3] = {11.8f, 4, 24.1f};
    require(navCellAt(world, grid, justBefore) == 0, "(11.8 - 12) / 0.5 = -0.4 truncates to 0 (vcvt.s32.f32)");
    const float before[3] = {11.4f, 4, 24.1f};
    require(navCellAt(world, grid, before) == -1, "-1.2 -> -1 is outside (cmp r0, #0)");
    const float beyond[3] = {14.2f, 4, 24.1f};
    require(navCellAt(world, grid, beyond) == -1, "4.4 -> 4 is not below the width 4 (cmp r2, r0)");
    const float afterDepth[3] = {13.2f, 4, 25.2f};
    require(navCellAt(world, grid, afterDepth) == -1, "2.4 -> 2 is not below the depth 2");
    const NavWorld blocked = sample(1);
    require(navCellAt(blocked, blocked.rooms[0].mesh.grids[0], inside) == -1, "(flags & 3) == 1 is not walkable (0x20d980)");
    const NavWorld three = sample(3);
    require(navCellAt(three, three.rooms[0].mesh.grids[0], inside) == 6, "flags & 3 == 3 is walkable for the lookup");
}

void locatePositions() {
    NavWorld world = sample();
    const float inside[3] = {13.2f, 4, 24.7f};
    NavLocation found = navLocate(world, inside, 1.0f, false);
    require(found.found && found.room == 0 && found.grid == 100 && found.cell == 6,
            "inside the box: room 0, grid 0 + base 100, cell 6 (0x1c2f00..0x1c2f10)");
    const float above[3] = {13.2f, 5.5f, 24.7f};
    found = navLocate(world, above, 1.0f, false);
    require(found.found && found.grid == 100 && found.cell == 6, "0.5 above the top (5) is within the tolerance 1: best grid (0x1c2f88)");
    found = navLocate(world, above, 0.25f, false);
    require(!found.found && found.room == kNavNone && found.grid == kNavNone && found.cell == kNavNone,
            "0.5 above is not below the tolerance 0.25; no near request, outputs stay 0xffff (0x1c2cfc)");
    const float slightlyAbove[3] = {13.2f, 5.2f, 24.7f};
    found = navLocate(world, slightlyAbove, 0.1f, true);
    require(!found.found && found.nearGrid == 100, "5.2 is inside the box scaled by 1.25 (y to 5.25): near grid 0 + base (0x1c2fd8)");
    found = navLocate(world, above, 0.25f, true);
    require(!found.found && found.nearGrid == -1, "5.5 is outside the scaled box: the near output keeps -1 (0x1c2d14)");
    const float below[3] = {13.2f, 2.9f, 24.7f};
    found = navLocate(world, below, 5.0f, true);
    require(!found.found && found.nearGrid == -1, "below the bottom (3): skipped (vcmpe s1, s0; bhs 0x1c2ff0)");
    const float outsideBox[3] = {20, 4, 24.7f};
    require(!navLocate(world, outsideBox, 5.0f, false).found, "outside every path box in x and z (0x20d99c)");
}

void blockedCellFallsBackToTheVerticalSearch() {
    NavWorld world = sample(1);
    const float inside[3] = {13.2f, 4, 24.7f};
    const NavLocation found = navLocate(world, inside, 1.0f, false);
    require(!found.found, "the cell lookup fails again for the best grid (0x1c3028 blt 0x1c3078)");
}

void loaderFixUp() {
    require(navFilePath("levels/levels/lvl_01/lvl_01_main.blvl", "Room12") == "levels/levels/lvl_01/lvl_01_main_room12.bfnnav",
            "<level>_<room>.bfnnav, room name in lower case (string at 0x197660 and the name built by 0x1971c4)");
    NavMesh mesh;
    NavGrid grid;
    grid.flags = 0xc005;
    grid.cells = {0, 1, 2, 3, 7, 0xfb};
    mesh.grids.push_back(grid);
    navNormalizeLoaded(mesh);
    const std::vector<std::uint8_t> expected = {0, 1, 2, 0, 4, 0xf8};
    require(mesh.grids[0].cells == expected, "cells with both kind bits set lose them (0x1975f0 .. 0x197688: bics r2=3, biceq #3)");
    require(mesh.grids[0].flags == 0x8005, "bit 14 of every grid is cleared (0x197694: bic r0, r0, #16384)");
}

}

int main() {
    try {
        loaderFixUp();
        gridNumbers();
        cellOfAGrid();
        locatePositions();
        blockedCellFallsBackToTheVerticalSearch();
    } catch (const std::exception& error) {
        std::cerr << "nav-grid-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "nav-grid-test: ok\n";
    return 0;
}
