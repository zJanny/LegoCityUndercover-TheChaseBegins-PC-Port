#pragma once
// Pregen/Traffic/<level>_Traffic.bin (loader 0x2e02d4, traffic manager 0x527410) and
// Pregen/Pedestrians/<level>_Pedestrians.bin (loader 0x2ee868, pedestrian manager 0x51b33c): spawn points on level
// paths and a quadtree of spawn point lists (node reader 0x1edbe4 / 0x1eb254, lookup 0x1eb490). No SDL dependency.
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace chase::assets {

enum class PregenSpawnKind { Traffic, Pedestrians };

struct PregenSpawnPoint {
    std::array<float, 3> position{};
    std::uint32_t storedPathPointer = 0;  // replaced by the loader with the path record of pathHash
    std::uint32_t pathHash = 0;           // name hash of a level path (section +0x20)
    float pathPosition = 0;               // spline parameter on that path
};

struct PregenSpawnCell {
    std::uint16_t halfSize = 0;
    float centreX = 0;
    float centreZ = 0;
    std::uint32_t storedListPointer = 0;
    std::uint32_t storedChildPointer = 0;
    std::array<std::int32_t, 4> children{-1, -1, -1, -1};  // low x low z, high x low z, low x high z, high x high z
    std::vector<std::uint16_t> points;
};

struct PregenSpawns {
    std::vector<PregenSpawnPoint> points;
    std::vector<PregenSpawnCell> cells;  // cells[0] is the root
};

// Index list capacity of one cell: the stack buffer of the node reader (400 bytes in 0x1edbe4, 512 in 0x1eb254).
std::size_t pregenSpawnCellCapacity(PregenSpawnKind kind);

// Archive path the loader opens for a level name of the level table (for example "lvl_01_cherrytreehills").
std::string pregenSpawnPath(PregenSpawnKind kind, const std::string& levelName);

PregenSpawns parsePregenSpawns(const std::vector<std::uint8_t>& data, PregenSpawnKind kind);

// 0x1eb490: the cell whose list holds candidates for the point (x, z), or -1 when the descent ends in an empty cell.
std::int32_t findPregenSpawnCell(const PregenSpawns& spawns, float x, float z);

}
