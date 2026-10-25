#include "assets/assets.hpp"
#include "assets/pregen_traffic.hpp"
#include "runtime/dcam.hpp"
#include "runtime/pregen_derive.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <set>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

using namespace chase::assets;
using namespace chase::runtime;

namespace {
void require(bool condition, const std::string& what) {
    if (!condition) throw std::runtime_error(what);
}

LevelPath line(const std::string& name, bool closed) {
    LevelPath path;
    path.nameHash = nameHash(name);
    path.closed = closed;
    path.points = {{0, 0, 0}, {10, 0, 0}, {20, 0, 0}, {30, 0, 0}};
    return path;
}

PregenSpawns spawns(std::initializer_list<PregenSpawnPoint> points) {
    PregenSpawns result;
    result.points = points;
    return result;
}

PregenSpawnPoint pointOn(const std::string& path, float parameter, std::array<float, 3> position) {
    PregenSpawnPoint point;
    point.position = position;
    point.pathHash = nameHash(path);
    point.pathPosition = parameter;
    return point;
}

void synthetic() {
    Level first, second;
    first.paths = {line("PathA", false)};
    second.paths = {line("PathA", false), line("PathB", true)};
    second.paths[0].points[3] = {20, 0, 20};
    const PregenSpawns file = spawns({pointOn("PathA", 1.5f, {0, 0, 0}), pointOn("PathB", 3.5f, {0, 0, 0}), pointOn("Missing", 0, {0, 0, 0})});
    const auto fields = deriveSpawnFields(file, {&first, &second}, {});
    require(fields.size() == 3, "one result per point");
    require(fields[0].pathFound && fields[0].levelIndex == 0 && fields[0].pathIndex == 0, "the first level that holds the hash wins");
    require(std::fabs(fields[0].tangent[0] - 1.0f) < 1e-6f && fields[0].tangent[1] == 0 && fields[0].tangent[2] == 0,
            "tangent along a straight path");
    require(fields[1].pathFound && fields[1].levelIndex == 1 && fields[1].pathIndex == 1, "second level path");
    require(!fields[2].pathFound && fields[2].tangent == std::array<float, 3>{0, 0, 0}, "missing path");
    const auto reversed = deriveSpawnFields(file, {&second, &first}, {});
    require(reversed[0].levelIndex == 0 && reversed[0].tangent[2] != 0.0f, "level order decides the path");

    Level typed;
    typed.paths = {line("PathA", false)};
    typed.paths[0].type = 1;
    bool threw = false;
    try {
        deriveSpawnFields(spawns({pointOn("PathA", 0, {0, 0, 0})}), {&typed}, {});
    } catch (const std::runtime_error&) {
        threw = true;
    }
    require(threw, "path type other than 0 is rejected");

    StreamRooms rooms;
    rooms[1];
    rooms[2];
    std::map<int, RoomBox> boxes;
    boxes[1] = {{0, 0, 0}, {10, 10, 10}};
    boxes[2] = {{5, 0, 0}, {20, 10, 10}};
    PregenRoomContext context{&rooms, &boxes, 0};
    const PregenSpawns inRooms = spawns({pointOn("x", 0, {7, 1, 5}), pointOn("x", 0, {15, 1, 5}), pointOn("x", 0, {50, 1, 5})});
    const auto roomed = deriveSpawnFields(inRooms, {}, context);
    require(roomed[0].room == 1 && roomed[1].room == 2 && roomed[2].room == 0, "first room in number order, none outside");
    context.currentRoom = 2;
    require(deriveSpawnFields(inRooms, {}, context)[0].room == 2, "the current room is tried first");
}

CollisionItem floorStrip(std::size_t count) {
    CollisionItem item;
    item.name = "collision_floor";
    item.floor.emplace();
    for (std::size_t k = 0; k < count; ++k) {
        const float x = 10.0f * float(k);
        for (const float v : {x, 0.0f, 0.0f, x, 0.0f, 1.0f, x + 1.0f, 0.0f, 0.0f}) item.vertices.push_back(v);
        CollisionTriangle triangle;
        triangle.vertex[0] = std::uint32_t(3 * k);
        triangle.vertex[1] = std::uint32_t(3 * k + 1);
        triangle.vertex[2] = std::uint32_t(3 * k + 2);
        item.triangles.push_back(triangle);
        FloorCell cell;
        cell.itemTriangle = true;
        cell.triangle = std::uint32_t(k);
        item.floor->cells.push_back(cell);
    }
    return item;
}

bool findThrows(std::size_t cells, const FloorPoint& point) {
    const CollisionItem strip = floorStrip(cells);
    try {
        findTrafficFloorCell(FloorMesh(strip), point);
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

void trafficFloorCell() {
    const CollisionItem strip = floorStrip(8);
    const FloorMesh mesh(strip);
    require(floorCellContains(mesh, 5, {50.25f, 3.0f, 0.5f}) && !floorCellContains(mesh, 5, {50.75f, 3.0f, 0.5f}), "inside the triangle in xz, any height");
    require(!floorCellContains(mesh, 5, {49.0f, 0.0f, 0.5f}), "left of the first edge");
    require(findTrafficFloorCell(mesh, {50.2f, 3.0f, 0.3f}) == 5, "the bisection ends next to the cell");
    require(findTrafficFloorCell(mesh, {70.2f, 3.0f, 0.3f}) == 7, "the upward scan covers the last cell");
    require(findTrafficFloorCell(mesh, {20.2f, 0.0f, 0.3f}) == 2, "the downward scan finds a cell below the bisection");
    require(findTrafficFloorCell(mesh, {35.0f, 0.0f, 0.3f}) == -1, "a point between two triangles");
    require(findTrafficFloorCell(mesh, {0.2f, 0.0f, 0.3f}) == -1, "cell 0 is never tested");
    require(findThrows(1, {0.2f, 0.0f, 0.3f}), "one cell: the original's bisection never ends");
    require(findThrows(2, {0.2f, 0.0f, 0.3f}), "two cells with the first closer: the original's bisection never ends");
    require(findTrafficFloorCell(FloorMesh(floorStrip(2)), {10.2f, 0.0f, 0.3f}) == 1, "two cells with the second closer");
    const CollisionItem none;
    require(findTrafficFloorCell(FloorMesh(none), {0, 0, 0}) == -1, "an item without a floor graph");
    const PregenSpawns points = spawns({pointOn("x", 0, {50.2f, 3.0f, 0.3f}), pointOn("x", 0, {35.0f, 0.0f, 0.3f})});
    require(deriveTrafficFloorCells(points, &mesh) == std::vector<int>({5, -1}), "one floor cell per point");
    require(deriveTrafficFloorCells(points, nullptr) == std::vector<int>({-1, -1}), "no floor graph, no cells");
}

struct Shipped {
    const char* level;
    const char* blvl;
    const char* models;
    PregenSpawnKind kind;
    std::size_t points, rooms, off;
};

std::size_t differingAxes(const float sample[3], const std::array<float, 3>& stored) {
    std::size_t axes = 0;
    for (int axis = 0; axis < 3; ++axis) axes += std::fabs(sample[axis] - stored[std::size_t(axis)]) > 1e-3f;
    return axes;
}

// Reference values (floor cells of the level's collision_floor item, points of the traffic file whose 0x1edaac lookup finds a cell): this
// port's findTrafficFloorCell run once over the shipped files, 2026-10-25, 20261025-traffic-C, command `build/pregen-derive-test <fib> <csv>`
// with a printed line per level. Not a recorded value of the original; the lookup is transcribed from 0x1edaac / 0x15f82c, and finding a cell for
// 2418 of 2421 points is the sanity check.
std::pair<std::size_t, std::size_t> floorCellCounts(const std::string& level) {
    if (level == "lvl_01_cherrytreehills") return {23713, 648};
    if (level == "lvl_03_auburn") return {27562, 469};
    if (level == "lvl_04_fortmeadows") return {34732, 229};
    if (level == "lvl_05_bluebellwood") return {34438, 187};
    if (level == "lvl_06_apolloisland") return {28425, 158};
    if (level == "lvl_07_downtown") return {15186, 727};
    throw std::runtime_error("no floor cell reference for " + level);
}

int shipped(const char* fib, const char* csv) {
    if (!std::ifstream(fib) || !std::ifstream(csv)) {
        std::cout << "game data not extracted; skipping\n";
        return 77;
    }
    // Reference values (rooms, off): this port's derivation run once over the shipped files, 2026-10-20, command
    // `build/pregen-derive-test <fib> <csv>` with the printed line of each file. Distinct rooms of the points, and points
    // whose stored position differs from the path sample at the stored parameter by more than 1e-3 on one axis
    // (docs/re/pregen-traffic.md). The tangent is checked against a central difference of the sampler, not a recorded value.
    const PregenSpawnKind T = PregenSpawnKind::Traffic, P = PregenSpawnKind::Pedestrians;
    const Shipped files[] = {
        {"lvl_01_cherrytreehills", "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl", "models/lvl_01_cherrytreehills", P, 1115, 32, 0},
        {"lvl_03_auburn", "levels/levels/lvl_03_auburn/lvl_03_auburn_main.blvl", "models/lvl_03_auburn", P, 615, 24, 0},
        {"lvl_04_fortmeadows", "levels/levels/lvl_04_fortmeadows/lvl_04_fortmeadows_main.blvl", "models/lvl_04_fortmeadows", P, 32, 2, 0},
        {"lvl_05_bluebellwood", "levels/levels/lvl_05_bluebellwood/lvl_05_bluebellwood_main.blvl", "models/lvl_05_bluebellwood", P, 354, 16, 0},
        {"lvl_06_apolloisland", "levels/levels/lvl_06_apolloisland/lvl_06_apolloisland_main.blvl", "models/lvl_06_apolloisland", P, 198, 11, 0},
        {"lvl_07_downtown", "levels/levels/lvl_07_downtown/lvl_07_downtown_main.blvl", "models/lvl_07_downtown", P, 1906, 51, 6},
        {"lvl_test_playground_traffic", "levels/levels/lvl_test_playground_traffic.blvl", "models/lvl_testplayground_traffic", P, 90, 1, 66},
        {"lvl_01_cherrytreehills", "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl", "models/lvl_01_cherrytreehills", T, 649, 27, 0},
        {"lvl_03_auburn", "levels/levels/lvl_03_auburn/lvl_03_auburn_main.blvl", "models/lvl_03_auburn", T, 469, 22, 0},
        {"lvl_04_fortmeadows", "levels/levels/lvl_04_fortmeadows/lvl_04_fortmeadows_main.blvl", "models/lvl_04_fortmeadows", T, 231, 13, 0},
        {"lvl_05_bluebellwood", "levels/levels/lvl_05_bluebellwood/lvl_05_bluebellwood_main.blvl", "models/lvl_05_bluebellwood", T, 187, 14, 0},
        {"lvl_06_apolloisland", "levels/levels/lvl_06_apolloisland/lvl_06_apolloisland_main.blvl", "models/lvl_06_apolloisland", T, 158, 15, 0},
        {"lvl_07_downtown", "levels/levels/lvl_07_downtown/lvl_07_downtown_main.blvl", "models/lvl_07_downtown", T, 727, 35, 0},
        {"lvl_test_playground_traffic", "levels/levels/lvl_test_playground_traffic.blvl", "models/lvl_testplayground_traffic", T, 123, 1, 0},
    };
    FuseArchive archive(fib);
    archive.loadNames(csv);
    for (const Shipped& want : files) {
        const std::string name = std::string(want.level) + (want.kind == T ? " traffic" : " pedestrians");
        const FuseEntry* spawnEntry = archive.find(pregenSpawnPath(want.kind, want.level));
        const FuseEntry* levelEntry = archive.find(want.blvl);
        require(spawnEntry && levelEntry, name + ": file not found");
        const PregenSpawns file = parsePregenSpawns(archive.read(*spawnEntry), want.kind);
        const Level level = parseLevel(archive.read(*levelEntry));
        const std::map<int, RoomBox> boxes = roomBoxesOf(archive, want.models);
        const StreamRooms rooms;
        const std::vector<PregenDerivedFields> fields = deriveSpawnFields(file, {&level}, {&rooms, &boxes, 0});
        require(fields.size() == want.points && file.points.size() == want.points, name + ": point count");
        std::set<int> distinct;
        std::size_t off = 0;
        for (std::size_t i = 0; i < fields.size(); ++i) {
            const std::string where = name + " point " + std::to_string(i);
            require(fields[i].pathFound, where + ": path not found");
            const LevelPath& path = level.paths[fields[i].pathIndex];
            require(path.nameHash == file.points[i].pathHash, where + ": wrong path");
            const std::array<float, 3>& tangent = fields[i].tangent;
            require(std::fabs(std::sqrt(tangent[0] * tangent[0] + tangent[1] * tangent[1] + tangent[2] * tangent[2]) - 1.0f) < 1e-4f,
                    where + ": tangent is not a unit vector");
            const float parameter = file.points[i].pathPosition, step = 1e-2f;
            float before[3], after[3], sample[3], difference[3], length = 0;
            sampleDirectorPath(path, parameter - step, before);
            sampleDirectorPath(path, parameter + step, after);
            sampleDirectorPath(path, parameter, sample);
            for (int axis = 0; axis < 3; ++axis) {
                difference[axis] = after[axis] - before[axis];
                length += difference[axis] * difference[axis];
            }
            length = std::sqrt(length);
            if (length > 1e-2f) {
                float dot = 0;
                for (int axis = 0; axis < 3; ++axis) dot += difference[axis] / length * tangent[std::size_t(axis)];
                require(dot > 0.99f, where + ": tangent does not follow the sampler, dot " + std::to_string(dot) + " length " + std::to_string(length));
            }
            const std::size_t axes = differingAxes(sample, file.points[i].position);
            require(axes <= 1, where + ": position differs from the sample on several axes");
            off += axes;
            require(fields[i].room > 0, where + ": no room");
            distinct.insert(fields[i].room);
        }
        require(distinct.size() == want.rooms && off == want.off,
                name + ": rooms " + std::to_string(distinct.size()) + " off " + std::to_string(off));
        if (want.kind == T) {
            const std::string level = want.level;
            const FuseEntry* modelEntry = archive.find("models/" + level + "/" + level + ".bfnmdl");
            if (!modelEntry) {
                require(level == "lvl_test_playground_traffic", name + ": master model not found");
                continue;
            }
            const Model model = parseModel(archive.read(*modelEntry));
            const CollisionItem* floorItem = nullptr;
            for (const CollisionItem& item : model.collision)
                if (item.floor && !floorItem) floorItem = &item;
            if (!floorItem) continue;
            const FloorMesh mesh(*floorItem);
            const std::vector<int> cells = deriveTrafficFloorCells(file, &mesh);
            std::size_t found = 0;
            for (const int cell : cells) found += cell >= 0;
            const std::pair<std::size_t, std::size_t> want_cells = floorCellCounts(level);
            require(std::size_t(mesh.cellCount()) == want_cells.first && found == want_cells.second,
                    name + ": floor cells " + std::to_string(mesh.cellCount()) + " found " + std::to_string(found));
        }
    }
    return 0;
}
}

int main(int argc, char** argv) {
    try {
        synthetic();
        trafficFloorCell();
        if (argc == 3) return shipped(argv[1], argv[2]);
        if (argc != 1) throw std::runtime_error("usage: pregen-derive-test [<fib> <csv>]");
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << "\n";
        return 1;
    }
    return 0;
}
