#pragma once
// Loads the nav grids (.bfnnav) of every room of a level for the AI path finder (docs/re/ai-navigation.md). No SDL.
#include <string>
#include <vector>

#include "assets/assets.hpp"
#include "runtime/nav_grid.hpp"

namespace chase::runtime {

// Path of the nav file of a room: the level path without ".blvl", "_", the room name in lower case, ".bfnnav" (loader 0x1971c4).
std::string navFilePath(const std::string& levelPath, const std::string& room);

// Fix-up of the loader 0x1971c4 after it reads a file (third argument 1 from its only caller 0x1c31ac): every cell whose low
// two bits are 3 loses them, and bit 14 of every grid's flags is cleared.
void navNormalizeLoaded(assets::NavMesh& mesh);

// One world per section record. A room without a nav file stops with an error.
std::vector<NavWorld> loadNavWorlds(assets::FuseArchive& archive, const std::string& levelPath, const assets::Level& level);

}
