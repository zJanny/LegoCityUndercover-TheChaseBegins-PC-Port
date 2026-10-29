#include "frontend/map_file.hpp"
#include "frontend/resources.hpp"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <stdexcept>

namespace chase::frontend {

namespace {

float floatAt(const std::vector<std::uint8_t>& bytes, std::size_t index) {
    const std::size_t at = index * 4;
    const std::uint32_t bits = std::uint32_t(bytes[at]) | std::uint32_t(bytes[at + 1]) << 8 | std::uint32_t(bytes[at + 2]) << 16 |
                               std::uint32_t(bytes[at + 3]) << 24;
    float value;
    std::memcpy(&value, &bits, sizeof value);
    return value;
}

std::string lowered(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

}

MapFile parseMapFile(const std::vector<std::uint8_t>& bytes) {
    if (bytes.size() != kMapFileSize)
        throw std::runtime_error("map file: " + std::to_string(bytes.size()) + " bytes, expected " + std::to_string(kMapFileSize));
    MapFile map;
    for (std::size_t i = 0; i < map.matrix.size(); ++i) map.matrix[i] = floatAt(bytes, i);
    for (std::size_t i = 0; i < map.unread.size(); ++i) map.unread[i] = floatAt(bytes, 16 + i);
    map.halfWidth = floatAt(bytes, 19) * 0.5f;
    map.halfHeight = floatAt(bytes, 20) * 0.5f;
    return map;
}

std::array<float, 2> mapNormalised(const MapFile& map, float x, float y, float z) {
    const auto& m = map.matrix;
    const auto column = [&](std::size_t c) { return x * m[c] + y * m[4 + c] + z * m[8 + c] + m[12 + c]; };
    const float w = column(3);
    return {column(0) / w, column(1) / w};
}

std::string mapName(int levelRecord, const std::string& recordName) {
    return levelRecord == kHubLevelRecord ? kHubMapName : recordName;
}

int levelRecord(Resources& resources, const std::string& levelName) {
    if (!resources.has(kLevelTable)) return -1;
    const std::size_t rows = resources.tableRows(kLevelTable, 0);
    for (std::size_t row = 0; row < rows; ++row)
        if (lowered(resources.tableText(kLevelTable, 0, row, kLevelRecordNameColumn)) == levelName) return int(row);
    return -1;
}

std::optional<LevelMap> loadLevelMap(Resources& resources, const std::string& levelName) {
    const int record = levelRecord(resources, levelName);
    if (record < 0) return std::nullopt;
    const std::string name = mapName(record, levelName);
    const std::string file = kMapFolder + name + ".txt";
    if (!resources.has(file)) return std::nullopt;
    return LevelMap{kMapFolder + name + ".btga", parseMapFile(resources.bytes(file)), record};
}

}
