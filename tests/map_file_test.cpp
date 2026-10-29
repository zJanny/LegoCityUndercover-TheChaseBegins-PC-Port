// Level map files (src/frontend/map_file.*): the 84-byte layout read by 0x1ee2d0, the projection of 0x1ee7a8 and the map name
// rule of 0x1ee2d0. Without arguments only the pinned values are checked; with FIB and CSV the shipped files are compared to them.
#include "frontend/map_file.hpp"
#include "frontend/resources.hpp"
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace chase::frontend;

int failures = 0;

void require(bool condition, const std::string& what) {
    if (!condition) {
        std::cerr << "FAIL: " << what << '\n';
        ++failures;
    }
}

struct PinnedMap {
    const char* name;
    std::uint32_t words[21];
};

// Provenance: round 20261029-hudmap-A, 2026-10-29, the 21 little-endian words of each
// sprites/ui_bottomscreen/maps/<name>.txt of the shipped archive (extracted with tools/extract_fib.py).
const PinnedMap kPinned[] = {
    {"lvl_01_cherrytreehills", {0x3fcce636, 0, 0, 0, 0, 0xb3964394, 0xbe08f891, 0, 0, 0x3fcce636, 0xb1c8e594, 0, 0xc4965a81, 0xc48c33b6,
                                0x428af17d, 0x44a72f25, 0x3fcce636, 0x3fcce636, 0x44a72f25, 0x44800000, 0x44800000}},
    {"lvl_02_albatrossisland", {0x3fcce636, 0, 0, 0, 0, 0xb3964394, 0xbd406a99, 0, 0, 0x3fcce636, 0xb10d1c0f, 0, 0xc45da3e3, 0xc5373a70,
                                0x425724fb, 0x43eadc1c, 0x3fcce636, 0x3fcce636, 0x43eadc1c, 0x44800000, 0x44800000}},
    {"lvl_03_auburn", {0x3fcce636, 0, 0, 0, 0, 0xb3964394, 0xbded0183, 0, 0, 0x3fcce636, 0xb1adcf3f, 0, 0xc4be1ed6, 0xc40f6fe3, 0x42379423,
                       0x4490a478, 0x3fcce636, 0x3fcce636, 0x4490a478, 0x44800000, 0x44800000}},
    {"lvl_04_fortmeadows", {0x3fcce636, 0, 0, 0, 0, 0xb3964394, 0xbe07b37d, 0, 0, 0x3fcce636, 0xb1c708c8, 0, 0xc502070c, 0x44f9a341,
                            0x4301170c, 0x44a5a25c, 0x3fcce636, 0x3fcce636, 0x44a5a25c, 0x44800000, 0x44800000}},
    {"lvl_05_bluebellwood", {0x3fcce636, 0, 0, 0, 0, 0xb3964394, 0xbe075159, 0, 0, 0x3fcce636, 0xb1c678d7, 0, 0xc4b3fc8a, 0x45228fd4,
                             0x41b03cc5, 0x44a52a92, 0x3fcce636, 0x3fcce636, 0x44a52a92, 0x44800000, 0x44800000}},
    {"lvl_06_apolloisland", {0x3fcce636, 0, 0, 0, 0, 0xb3964394, 0xbe0ac9f1, 0, 0, 0x3fcce636, 0xb1cb9026, 0, 0x4588e685, 0xc4ec7dfe,
                             0x42bcaf66, 0x44a9672c, 0x3fcce636, 0x3fcce636, 0x44a9672c, 0x44800000, 0x44800000}},
    {"lvl_07_downtown", {0x3fcce636, 0, 0, 0, 0, 0xb3964394, 0xbe35ef3c, 0, 0, 0x3fcce636, 0xb2056c33, 0, 0x4488c4cd, 0xc4990ff4,
                         0x4295b391, 0x44de10d9, 0x3fcce636, 0x3fcce636, 0x44de10d9, 0x44800000, 0x44800000}},
    {"lvl_08_paradisesands", {0x3f800000, 0, 0, 0, 0x80000000, 0xb33bbd2e, 0x3f800000, 0, 0, 0xbf800000, 0xb33bbd2e, 0, 0xc4a1a7bd,
                              0x42c4c443, 0xc43e4b68, 0, 0x3f78d3ca, 0x3fcce636, 0x44d1c4aa, 0x44af0000, 0x44530000}},
};

std::vector<std::uint8_t> bytesOf(const PinnedMap& map) {
    std::vector<std::uint8_t> bytes;
    for (std::uint32_t word : map.words)
        for (int shift = 0; shift < 32; shift += 8) bytes.push_back(std::uint8_t(word >> shift));
    return bytes;
}

float floatOf(std::uint32_t word) {
    float value;
    std::memcpy(&value, &word, sizeof value);
    return value;
}

void layout() {
    for (const PinnedMap& pinned : kPinned) {
        const MapFile map = parseMapFile(bytesOf(pinned));
        bool same = true;
        for (std::size_t i = 0; i < 16; ++i) same = same && std::memcmp(&map.matrix[i], &pinned.words[i], 4) == 0;
        for (std::size_t i = 0; i < 3; ++i) same = same && std::memcmp(&map.unread[i], &pinned.words[16 + i], 4) == 0;
        require(same, std::string(pinned.name) + ": 16 matrix floats, then three floats");
        require(map.halfWidth == floatOf(pinned.words[19]) * 0.5f && map.halfHeight == floatOf(pinned.words[20]) * 0.5f,
                std::string(pinned.name) + ": the two sizes are halved");
    }
    const MapFile cherry = parseMapFile(bytesOf(kPinned[0]));
    require(cherry.halfWidth == 512.0f && cherry.halfHeight == 512.0f, "lvl_01: half sizes 512 (1024 in the file)");
    // The texel formula of objective-hud-map.md §2026-10-15T19:56 (oracle-checked): u = 512 + 512 * (1.6007755 x - 1202.8282) / 1337.4733,
    // v = 512 - 512 * (1.6007755 z - 1121.6160) / 1337.4733, at the player position of gameplay-start.
    const std::array<float, 2> n = mapNormalised(cherry, 886.7644f, 16.6f, 1111.1659f);
    const float u = 512.0f + 512.0f * n[0], v = 512.0f - 512.0f * n[1];
    require(std::fabs(u - (512.0f + 512.0f * (1.6007755f * 886.7644f - 1202.8282f) / 1337.4733f)) < 1e-3f, "lvl_01 texel u");
    require(std::fabs(v - (512.0f - 512.0f * (1.6007755f * 1111.1659f - 1121.6160f) / 1337.4733f)) < 1e-3f, "lvl_01 texel v");
}

void rejection() {
    std::vector<std::uint8_t> bytes = bytesOf(kPinned[0]);
    for (const std::size_t size : {std::size_t(0), std::size_t(83), std::size_t(85)}) {
        bytes.resize(size);
        bool threw = false;
        try {
            parseMapFile(bytes);
        } catch (const std::runtime_error& error) {
            threw = std::string(error.what()).find(std::to_string(size) + " bytes") != std::string::npos;
        }
        require(threw, "a map file of " + std::to_string(size) + " bytes is rejected with its size");
    }
}

void names() {
    require(mapName(1, "lvl_00_policestation") == "lvl_01_cherrytreehills", "record 1 shows the map of lvl_01 (0x1ee2d4)");
    require(mapName(2, "lvl_01_cherrytreehills") == "lvl_01_cherrytreehills", "other records use their own name (row + 0x30)");
    require(mapName(8, "lvl_07_downtown") == "lvl_07_downtown", "record 8");
}

void shipped(const char* fib, const char* csv) {
    chase::assets::FuseArchive archive(fib);
    archive.loadNames(csv);
    Resources resources(archive);
    for (const PinnedMap& pinned : kPinned)
        require(resources.bytes(std::string(kMapFolder) + pinned.name + ".txt") == bytesOf(pinned), std::string(pinned.name) + ": shipped file equals the pinned words");
    require(levelRecord(resources, "lvl_00_policestation") == 1 && levelRecord(resources, "lvl_01_cherrytreehills") == 2 &&
                levelRecord(resources, "lvl_07_downtown") == 8,
            "TablesLevels records of the hub, lvl_01 and lvl_07");
    const auto hub = loadLevelMap(resources, "lvl_00_policestation");
    require(hub && hub->picture == "sprites/ui_bottomscreen/maps/lvl_01_cherrytreehills.btga" && hub->file.matrix[15] == floatOf(kPinned[0].words[15]),
            "the hub loads the picture and file of lvl_01");
    for (std::size_t i = 0; i + 1 < std::size(kPinned); ++i) {
        const auto map = loadLevelMap(resources, kPinned[i].name);
        require(map && map->picture == std::string(kMapFolder) + kPinned[i].name + ".btga" && resources.has(map->picture),
                std::string(kPinned[i].name) + ": its own picture");
    }
    require(!loadLevelMap(resources, "lvl_08_paradisesands"), "lvl_08 is no TablesLevels record, so its map is never loaded");
    require(!loadLevelMap(resources, "lvl_missing"), "an unknown level has no map");
}

}

int main(int argc, char** argv) {
    try {
        layout();
        rejection();
        names();
        if (argc == 3) {
            if (!std::ifstream(argv[1]).good() || !std::ifstream(argv[2]).good()) {
                std::cout << "map-file-test: no game data, skipping\n";
                return 77;
            }
            shipped(argv[1], argv[2]);
        }
    } catch (const std::exception& error) {
        std::cerr << "map-file-test: " << error.what() << '\n';
        return 1;
    }
    if (failures) return 1;
    std::cout << "map-file-test: ok\n";
    return 0;
}
