#pragma once
// Navigation grids as the path finder sees them (docs/re/ai-navigation.md): the loaded section record (height unit,
// cell size, origin), the room descriptors with their global grid number ranges, and the two position lookups 0x1c2ccc
// (world position -> room, grid, cell) and 0x20d8a8 (cell of a grid). No SDL dependency.
#include <array>
#include <cstdint>
#include <optional>
#include <vector>

#include "assets/assets.hpp"

namespace chase::runtime {

constexpr std::uint16_t kNavNone = 0xffff;

// Fields the grid record gets at run time (record +0x10, +0x36 and the extras that 0x1d05d0 edits).
struct NavGridState {
    std::uint16_t linkMask = 0xffff;
    float stamp = 0;
    std::vector<std::array<std::uint8_t, 4>> extras;
};

struct NavWorldRoom {
    std::uint16_t base = 0;
    std::uint16_t end = 0;
    assets::NavMesh mesh;
    std::vector<NavGridState> state;
};

// Record +8 of the section the rooms belong to (global 0x376bec, field +4): heightUnit at +4, cellSize at +8, origin at +0xc.
struct NavWorld {
    float heightUnit = 0;
    float cellSize = 0;
    std::array<float, 3> origin{};
    std::vector<NavWorldRoom> rooms;
    // Game time of 0x2355c0 in seconds, read by the cell clean-up of 0x208338.
    float now = 0;
    // Global 0x376bec field +0xc: per-gate weights read by the grid cost 0x1f9ea0 and the exit cell choice 0x27d90c.
    // Empty means the pointer is null, the state of the game until something sets it.
    std::vector<float> gateWeights;

    static NavWorld fromRooms(const std::vector<assets::NavRoom>& descriptors, const std::vector<assets::NavMesh>& meshes);
};

// One world per section record of the level, in order; `meshes` is parallel to `descriptors` (level.navRooms). Which
// section the original keeps in global 0x376bec field +4 is not recovered (stand-in SCRIPT-S93: callers use the first
// world whose lookup finds the position).
std::vector<NavWorld> navWorldsFromLevel(const std::vector<assets::NavRoom>& descriptors, const std::vector<assets::NavMesh>& meshes);

std::uint16_t navTotalGrids(const NavWorld& world);

// 0x221de4: the grid with global number `id`, null when no room range holds it.
assets::NavGrid* navGridById(NavWorld& world, std::uint16_t id);
const assets::NavGrid* navGridById(const NavWorld& world, std::uint16_t id);
NavGridState* navGridStateById(NavWorld& world, std::uint16_t id);

// 0x20d8a8: cell index of the position in the grid, -1 outside or on a cell whose low two bits are 1. The cell offset is
// the quotient truncated toward zero, so a position up to one cell before the grid origin still maps to cell 0.
int navCellAt(const NavWorld& world, const assets::NavGrid& grid, const float position[3]);

struct NavBox {
    float centre[3];
    float half[3];
};

// The box of a grid as the lookup 0x1c2ccc and the follower (0x1faf70, 0x1fb1dc) build it, and the test 0x2397a0.
NavBox navGridBox(const NavWorld& world, const assets::NavGrid& grid);
bool navInsideBox(const float position[3], const NavBox& box);

struct NavLocation {
    std::uint16_t room = kNavNone;
    std::uint16_t grid = kNavNone;
    std::uint16_t cell = kNavNone;
    bool found = false;
    // Global number of the last grid whose box scaled by 1.25 held the position (the optional output of 0x1c2ccc), -1 if none.
    std::int64_t nearGrid = -1;
};

// 0x1c2ccc. `tolerance` is the largest height of the position above a grid box that still counts (s0), `wantNear` the
// presence of the optional output pointer.
NavLocation navLocate(const NavWorld& world, const float position[3], float tolerance, bool wantNear);

}
