#pragma once
// The level map of the bottom screen (docs/re/objective-hud-map.md): 0x1ee2d0 loads the picture
// sprites/UI_Bottomscreen/Maps/<name>.tga and the 84-byte file of the same name with the extension .txt (binary floats), and
// 0x1ee7a8 places a world position on the map with it. No SDL dependency.
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace chase::frontend {

class Resources;

constexpr std::size_t kMapFileSize = 84;
constexpr const char* kMapFolder = "sprites/ui_bottomscreen/maps/";
constexpr const char* kLevelTable = "tables/tableslevels.bxls";
constexpr std::size_t kLevelRecordNameColumn = 21;
constexpr int kHubLevelRecord = 1;
constexpr const char* kHubMapName = "lvl_01_cherrytreehills";
constexpr float kMapQuadHalfSize = 512.0f;

struct MapFile {
    std::array<float, 16> matrix{};          // struct + 0x308, row-major, the row vector (x, y, z, 1) times the matrix
    std::array<float, 3> unread{};           // struct + 0x348, 0x34c, 0x350: no reader found
    float halfWidth = 0, halfHeight = 0;     // struct + 0x354, 0x358: the last two floats of the file times 0.5
};

MapFile parseMapFile(const std::vector<std::uint8_t>& bytes);
std::array<float, 2> mapNormalised(const MapFile& map, float x, float y, float z);
std::string mapName(int levelRecord, const std::string& recordName);

struct MapProjection {
    MapFile file;
    float zoom = 0;
    int levelRecord = -1;
};

struct LevelMap {
    std::string picture;
    MapFile file;
    int levelRecord = -1;
};

int levelRecord(Resources& resources, const std::string& levelName);
std::optional<LevelMap> loadLevelMap(Resources& resources, const std::string& levelName);

}
