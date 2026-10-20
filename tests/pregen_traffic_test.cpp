// pregen-traffic: synthetic files for the spawn file reader and the cell lookup. With <fib> <csv> it also reads the
// 14 shipped files (pregen-traffic-real-data); with missing files that part exits 77 (skip).
#include "assets/assets.hpp"
#include "assets/pregen_traffic.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace chase::assets;

namespace {
void require(bool condition, const std::string& what) {
    if (!condition) throw std::runtime_error(what);
}
void requireThrows(const std::function<void()>& run, const std::string& what) {
    try {
        run();
    } catch (const std::runtime_error&) {
        return;
    }
    throw std::runtime_error("accepted: " + what);
}

struct Writer {
    std::vector<std::uint8_t> bytes;
    void u16(std::uint16_t v) {
        bytes.push_back(std::uint8_t(v));
        bytes.push_back(std::uint8_t(v >> 8));
    }
    void u32(std::uint32_t v) {
        for (int i = 0; i < 4; ++i) bytes.push_back(std::uint8_t(v >> (8 * i)));
    }
    void f32(float v) {
        std::uint32_t bits;
        std::memcpy(&bits, &v, sizeof bits);
        u32(bits);
    }
    void cell(std::uint16_t half, std::uint16_t count, float x, float z, std::uint32_t list, std::uint32_t child) {
        u16(half);
        u16(count);
        f32(x);
        f32(z);
        u32(list);
        u32(child);
    }
};

void point(Writer& w, float x, float y, float z, std::uint32_t hash, float t) {
    w.f32(x);
    w.f32(y);
    w.f32(z);
    w.u32(0x037f1614);
    w.u32(hash);
    w.f32(t);
}

std::vector<std::uint8_t> sampleFile() {
    Writer w;
    w.u32(3);
    point(w, 10, 1, 10, 0x6b3e4292, 0.2f);
    point(w, 30, 2, 10, 0x722573d3, 1.5f);
    point(w, 10, 3, 30, 0x722573d3, 8.0f);
    w.cell(20, 0, 20, 20, 0, 0x04134290);
    const float centres[4][2] = {{10, 10}, {30, 10}, {10, 30}, {30, 30}};
    const std::uint16_t counts[4] = {2, 0, 1, 1};
    for (int k = 0; k < 4; ++k) w.cell(10, counts[k], centres[k][0], centres[k][1], counts[k] ? 0x04134360 : 0, 0);
    for (int k = 0; k < 4; ++k) {
        w.cell(10, counts[k], centres[k][0], centres[k][1], counts[k] ? 0x04134360 : 0, 0);
        if (k == 0) w.u16(0), w.u16(1);
        if (k == 2) w.u16(2);
        if (k == 3) w.u16(1);
    }
    return w.bytes;
}

void synthetic() {
    const PregenSpawns spawns = parsePregenSpawns(sampleFile(), PregenSpawnKind::Traffic);
    require(spawns.points.size() == 3 && spawns.cells.size() == 5, "point and cell counts");
    const PregenSpawnPoint& p = spawns.points[1];
    require(p.position[0] == 30 && p.position[1] == 2 && p.position[2] == 10 && p.storedPathPointer == 0x037f1614 &&
                p.pathHash == 0x722573d3 && p.pathPosition == 1.5f,
            "point fields");
    const PregenSpawnCell& root = spawns.cells[0];
    require(root.halfSize == 20 && root.centreX == 20 && root.centreZ == 20 && root.points.empty() &&
                root.storedChildPointer == 0x04134290 && root.children[0] == 1 && root.children[3] == 4,
            "root cell");
    require(spawns.cells[1].points == std::vector<std::uint16_t>({0, 1}) && spawns.cells[2].points.empty() &&
                spawns.cells[2].children[0] == -1 && spawns.cells[4].points == std::vector<std::uint16_t>({1}),
            "child cells");

    require(findPregenSpawnCell(spawns, 5, 5) == 1, "low x low z");
    require(findPregenSpawnCell(spawns, 20, 20) == 1, "a point on the low edge stays low");
    require(findPregenSpawnCell(spawns, 20.001f, 5) == -1, "high x low z is an empty cell");
    require(findPregenSpawnCell(spawns, 5, 25) == 3, "low x high z");
    require(findPregenSpawnCell(spawns, 25, 25) == 4, "high x high z");
    require(findPregenSpawnCell(spawns, -500, 500) == 3, "points outside the root fall into the nearest corner");
    require(findPregenSpawnCell(spawns, std::nanf(""), std::nanf("")) == 1, "unordered compares take the low side");

    const PregenSpawns pedestrians = parsePregenSpawns(sampleFile(), PregenSpawnKind::Pedestrians);
    require(pedestrians.cells.size() == 5, "pedestrian file uses the same layout");
    require(pregenSpawnPath(PregenSpawnKind::Traffic, "lvl_01_cherrytreehills") ==
                    "Pregen/Traffic/lvl_01_cherrytreehills_Traffic.bin" &&
                pregenSpawnPath(PregenSpawnKind::Pedestrians, "lvl_07_downtown") ==
                    "Pregen/Pedestrians/lvl_07_downtown_Pedestrians.bin",
            "loader paths (strings 0x2e0560, 0x2eeaa0)");
    require(pregenSpawnCellCapacity(PregenSpawnKind::Traffic) == 200 &&
                pregenSpawnCellCapacity(PregenSpawnKind::Pedestrians) == 256,
            "cell list capacities (stack 400 bytes at 0x1edbe8, 512 at 0x1eb258)");

    const auto traffic = [](const std::vector<std::uint8_t>& data) { parsePregenSpawns(data, PregenSpawnKind::Traffic); };
    std::vector<std::uint8_t> bad = sampleFile();
    bad.pop_back();
    requireThrows([&] { traffic(bad); }, "truncated file");
    bad = sampleFile();
    bad.push_back(0);
    requireThrows([&] { traffic(bad); }, "trailing byte");
    bad = sampleFile();
    bad[0] = 11;
    requireThrows([&] { traffic(bad); }, "point count beyond the file");
    bad = sampleFile();
    bad[0] = 0xff;
    bad[1] = bad[2] = bad[3] = 0xff;
    requireThrows([&] { traffic(bad); }, "huge point count");
    bad = sampleFile();
    bad[bad.size() - 2] = 3;
    requireThrows([&] { traffic(bad); }, "list index beyond the points");
    requireThrows([&] { traffic({}); }, "empty file");

    Writer full;
    full.u32(1);
    point(full, 0, 0, 0, 1, 0);
    full.cell(4, 201, 0, 0, 1, 0);
    for (int i = 0; i < 201; ++i) full.u16(0);
    requireThrows([&] { traffic(full.bytes); }, "201 entries in a traffic cell");
    require(parsePregenSpawns(full.bytes, PregenSpawnKind::Pedestrians).cells[0].points.size() == 201,
            "201 entries fit a pedestrian cell");
    Writer over;
    over.u32(1);
    point(over, 0, 0, 0, 1, 0);
    over.cell(4, 257, 0, 0, 1, 0);
    for (int i = 0; i < 257; ++i) over.u16(0);
    requireThrows([&] { parsePregenSpawns(over.bytes, PregenSpawnKind::Pedestrians); }, "257 entries in a pedestrian cell");

    Writer deep;
    deep.u32(0);
    for (int depth = 0; depth < 18; ++depth) {
        deep.cell(1, 0, 0, 0, 0, 1);
        for (int k = 0; k < 4; ++k) deep.cell(1, 0, 0, 0, 0, 0);
    }
    requireThrows([&] { traffic(deep.bytes); }, "cell tree deeper than 16 levels");
}

struct Shipped {
    const char* level;
    PregenSpawnKind kind;
    std::size_t points, cells, leaves, empty, entries, largest, ownCell, size;
    std::uint32_t fnv;
};

std::uint32_t fnv1a(const std::vector<std::uint8_t>& data) {
    std::uint32_t value = 0x811c9dc5;
    for (auto byte : data) value = (value ^ byte) * 0x01000193;
    return value;
}

int shipped(const char* fib, const char* csv) {
    if (!std::ifstream(fib) || !std::ifstream(csv)) {
        std::cout << "game data not extracted; skipping\n";
        return 77;
    }
    // Reference: a throwaway Python struct walk of artifacts/fib/files/pregen/{traffic,pedestrians}/*.bin (2026-10-20,
    // not kept): points, cells, cells with a list, empty cells, list entries, longest list, points found in the list of
    // their own cell by the 0x1eb490 descent, file size and FNV-1a. SHA-256 of each file: docs/re/pregen-traffic.md.
    const PregenSpawnKind T = PregenSpawnKind::Traffic, P = PregenSpawnKind::Pedestrians;
    const Shipped files[] = {
        {"lvl_01_cherrytreehills", P, 1115, 85, 64, 0, 7173, 237, 1115, 44490, 0x09bef631},
        {"lvl_03_auburn", P, 615, 85, 64, 0, 3899, 117, 615, 25942, 0x340f2bff},
        {"lvl_04_fortmeadows", P, 32, 5, 4, 0, 106, 30, 32, 1164, 0x9c3821f1},
        {"lvl_05_bluebellwood", P, 354, 85, 53, 11, 2372, 109, 354, 16624, 0xbf63ed2f},
        {"lvl_06_apolloisland", P, 198, 21, 16, 0, 951, 96, 198, 7478, 0x05d57da2},
        {"lvl_07_downtown", P, 1906, 341, 178, 78, 12785, 169, 1906, 84938, 0xc3659d07},
        {"lvl_test_playground_traffic", P, 90, 21, 16, 0, 454, 43, 90, 3892, 0x09cbb9a1},
        {"lvl_01_cherrytreehills", T, 649, 85, 53, 11, 4326, 168, 649, 27612, 0x751aec63},
        {"lvl_03_auburn", T, 469, 85, 61, 3, 2719, 87, 469, 20078, 0x749845eb},
        {"lvl_04_fortmeadows", T, 231, 341, 88, 168, 1586, 42, 231, 22340, 0x0c67d059},
        {"lvl_05_bluebellwood", T, 187, 85, 38, 26, 1127, 59, 187, 10126, 0xd39caf1f},
        {"lvl_06_apolloisland", T, 158, 85, 41, 23, 885, 41, 158, 8946, 0x0d9230dc},
        {"lvl_07_downtown", T, 727, 341, 141, 115, 4649, 70, 711, 40370, 0xf4c881b2},
        {"lvl_test_playground_traffic", T, 123, 21, 16, 0, 669, 71, 123, 5114, 0xf777eb30},
    };
    FuseArchive archive(fib);
    archive.loadNames(csv);
    for (const Shipped& want : files) {
        const std::string path = pregenSpawnPath(want.kind, want.level);
        const FuseEntry* entry = archive.find(path);
        require(entry, path + " not found");
        const std::vector<std::uint8_t> data = archive.read(*entry);
        require(data.size() == want.size && fnv1a(data) == want.fnv, path + " differs from the recorded file");
        const PregenSpawns spawns = parsePregenSpawns(data, want.kind);
        std::size_t leaves = 0, empty = 0, entries = 0, largest = 0, ownCell = 0;
        for (const PregenSpawnCell& cell : spawns.cells) {
            leaves += !cell.points.empty();
            empty += cell.points.empty() && cell.children[0] < 0;
            entries += cell.points.size();
            largest = std::max(largest, cell.points.size());
        }
        for (std::size_t i = 0; i < spawns.points.size(); ++i) {
            const std::int32_t found = findPregenSpawnCell(spawns, spawns.points[i].position[0], spawns.points[i].position[2]);
            if (found < 0) continue;
            for (auto listed : spawns.cells[std::size_t(found)].points) ownCell += listed == i;
        }
        require(spawns.points.size() == want.points && spawns.cells.size() == want.cells && leaves == want.leaves &&
                    empty == want.empty && entries == want.entries && largest == want.largest && ownCell == want.ownCell,
                path + " differs from the recorded census");
    }
    return 0;
}
}

int main(int argc, char** argv) {
    try {
        synthetic();
        if (argc == 3) return shipped(argv[1], argv[2]);
        if (argc != 1) throw std::runtime_error("usage: pregen-traffic-test [<fib> <csv>]");
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << "\n";
        return 1;
    }
    return 0;
}
