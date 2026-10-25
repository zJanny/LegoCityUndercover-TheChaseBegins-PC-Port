// Tests of src/runtime/traffic_route.*: the routes of the traffic cars. Every synthetic expectation follows from the disassembly
// (llvm-objdump --triple=armv6k-none-eabi --mattr=+vfp2 of artifacts/game/game.arm.elf, read by 20261025-traffic-A on 2026-10-25,
// docs/re/traffic.md): 0x2df574 / 0x2c8718 (route list), 0x2dece4 (EnableTrafficPath), 0x2dedbc (pending replay), 0x2dfc84 / 0x20ff40
// (route → junction), 0x2cdde4 (junction test), 0x2bf2ec (path follower), 0x1f47f0 (cell), table 0x3581d4 (route type function).
// With <fib> <csv> it also checks the shipped traffic levels.
#include "runtime/traffic_route.hpp"

#include "assets/assets.hpp"
#include "assets/pregen_traffic.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using namespace chase::runtime;
using chase::assets::LevelPath;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b) { return std::fabs(a - b) < 1e-5f; }

TrafficJunction junctionAt(float x, float y, float z, std::vector<std::uint32_t> inputs, std::vector<std::uint32_t> outputs) {
    TrafficJunction junction;
    junction.position = {x, y, z};
    junction.inputs = std::move(inputs);
    junction.outputs = std::move(outputs);
    return junction;
}

void routeList() {
    TrafficRouteTable table;
    std::vector<TrafficJunction> junctions = {
        junctionAt(0.0f, 50000.0f, 0.0f, {11, 12}, {13}),
        junctionAt(9000.0f, 0.0f, 0.0f, {21}, {22}),
        junctionAt(6000.0f, 0.0f, -6000.0f, {31}, {11, 32}),
        junctionAt(100.0f, 0.0f, 100.0f, {13}, {41}),
    };
    require(collectTrafficRoutes(table, junctions) == 3, "0x2c8718 takes junctions with 9000 > sqrt(dx^2 + dz^2): y is ignored, 9000 is out");
    const std::vector<std::uint32_t> order = {11, 12, 13, 31, 32, 41};
    require(table.routes == order, "0x2df574: inputs then outputs per junction, a route already listed is not added again");
    for (std::size_t i = 0; i < order.size(); ++i) require(table.enabled.at(i), "0x2df574 sets the bit of every new route");
    require(*trafficRouteIndex(table, 31) == 3 && !trafficRouteIndex(table, 21), "the route index is its place in the list (record +0x24)");

    table.enabled[1] = false;
    junctions.erase(junctions.begin() + 2);
    require(collectTrafficRoutes(table, junctions) == 2, "two junctions within reach");
    require((table.routes == std::vector<std::uint32_t>{11, 12, 13, 41}), "the count +0x2d4 is reset to 0 first");
    require(table.enabled.at(1) && table.enabled.at(4) && table.enabled.at(5), "bits are only set, the old bits 4 and 5 stay");

    std::vector<TrafficJunction> many;
    for (std::uint32_t i = 0; i < 40; ++i) many.push_back(junctionAt(float(i), 0.0f, 0.0f, {100 + i}, {}));
    TrafficRouteTable full;
    require(collectTrafficRoutes(full, many) == 32 && full.routes.size() == 32 && full.routes.back() == 131, "0x2c8718 stops at 32 junctions");

    TrafficRouteTable kept;
    kept.routes = {7};
    kept.enabled = {false};
    require(collectTrafficRoutes(kept, {junctionAt(9500.0f, 0.0f, 0.0f, {8}, {})}) == 0, "no junction within reach");
    require(kept.routes.empty() && kept.enabled.size() == 1 && !kept.enabled[0],
            "0x2df574 sets the count to 0 (0x2df5a0) before 0x2c8718 and returns with none: an empty list, the bits untouched");

    std::vector<TrafficJunction> wide;
    for (std::uint32_t i = 0; i < 8; ++i) {
        TrafficJunction junction = junctionAt(float(i), 0.0f, 0.0f, {}, {});
        for (std::uint32_t k = 0; k < 8; ++k) junction.inputs.push_back(1000 + i * 16 + k);
        for (std::uint32_t k = 8; k < 16; ++k) junction.outputs.push_back(1000 + i * 16 + k);
        wide.push_back(junction);
    }
    TrafficRouteTable limit;
    require(collectTrafficRoutes(limit, wide) == 8 && limit.routes.size() == kTrafficMaxRoutes, "128 routes fit in the list +0xc4");
    wide.push_back(junctionAt(0.0f, 0.0f, 0.0f, {5000}, {}));
    bool refused = false;
    try {
        collectTrafficRoutes(limit, wide);
    } catch (const std::length_error&) {
        refused = true;
    }
    require(refused, "TRAFFIC-S4: the 129th route is refused");
}

void enablePath() {
    TrafficRouteTable table;
    table.routes = {5, 6};
    table.enabled = {true, true};
    enableTrafficPath(table, 6, false);
    require(table.pending.size() == 1 && table.pending[0] == 6, "not loaded: disabling appends to the pending list +0xb9c");
    enableTrafficPath(table, 6, false);
    require(table.pending.size() == 1, "a route already pending is not appended again");
    enableTrafficPath(table, 6, true);
    require(table.pending.size() == 1 && table.enabled[1], "not loaded: enabling does nothing, the pending entry stays");
    table.loaded = true;
    enableTrafficPath(table, 5, false);
    require(!table.enabled[0] && table.pending.size() == 1, "loaded: the bit is cleared at once");
    enableTrafficPath(table, 5, true);
    require(table.enabled[0], "loaded: the bit is set again");
    enableTrafficPath(table, 99, false);
    require(table.enabled[0] && table.enabled[1] && table.pending.size() == 1, "loaded: a route missing from the list is ignored");

    applyPendingTrafficRoutes(table);
    require(!table.enabled[1] && table.enabled[0] && table.pending.empty(), "0x2dedbc: a pending route loses its bit, the list is emptied");

    TrafficRouteTable unloaded;
    unloaded.routes = {5};
    unloaded.enabled = {true};
    enableTrafficPath(unloaded, 5, false);
    applyPendingTrafficRoutes(unloaded);
    require(unloaded.enabled[0] && unloaded.pending.empty(), "0x2dedbc without bit 0: the pending list is only emptied");

    TrafficRouteTable crowded;
    for (std::uint32_t i = 0; i < kTrafficMaxPendingRoutes; ++i) enableTrafficPath(crowded, i, false);
    bool refused = false;
    try {
        enableTrafficPath(crowded, 1000, false);
    } catch (const std::length_error&) {
        refused = true;
    }
    require(refused, "TRAFFIC-S4: the 129th pending route is refused");
}

void signals() {
    TrafficRouteTable table;
    table.routes = {1, 2, 3};
    table.enabled = {true, true, true};
    const std::vector<TrafficJunction> junctions = {
        junctionAt(0.0f, 0.0f, 0.0f, {2}, {1}),
        junctionAt(0.0f, 0.0f, 0.0f, {2, 1}, {3}),
        junctionAt(0.0f, 0.0f, 0.0f, {4}, {}),
    };
    const TrafficRouteSignals linked = linkTrafficRouteSignals(table, junctions);
    require(linked.inputs.size() == 3 && linked.inputs[0].route == 1 && linked.inputs[0].junction == 1 && linked.inputs[1].route == 2 &&
                linked.inputs[1].junction == 0 && linked.inputs[2].junction == 1,
            "0x2dfc84: pairs in route list order, then junction order");
    require(linked.outputs.size() == 2 && linked.outputs[0].route == 1 && linked.outputs[1].route == 3, "output entries fill the second list");
    require(trafficSignalForRoute(linked, 2) == std::optional<std::size_t>(0), "0x20ff40 takes the first input pair");
    require(!trafficSignalForRoute(linked, 3), "0x20ff40 reads only the input pairs");
    require(!trafficSignalForRoute(linked, 4), "a junction path that is not in the route list has no pair");
}

void junctionTest() {
    TrafficJunction junction;
    junction.lanes = {70, 71, 72, 73};
    junction.conflicts = {{0, 2}, {3, 1}};
    require(trafficJunctionAllowsCar(junction, 71), "no listed lane: the car may go");
    require(trafficJunctionAllowsCar(junction, 99), "a route that is no lane of the junction: 1");
    junction.listedLanes = {2};
    require(!trafficJunctionAllowsCar(junction, 70), "lane 0 conflicts with listed lane 2: 0");
    require(trafficJunctionAllowsCar(junction, 72), "lane 2 itself listed, its partner 0 is not: 1");
    junction.listedLanes = {1};
    require(!trafficJunctionAllowsCar(junction, 73), "pair (3, 1) found through its first byte");
    require(trafficJunctionAllowsCar(junction, 71), "pair (3, 1) found through its second byte: the partner 3 is not listed");
}

void junctionPairOrder() {
    TrafficJunction junction;
    junction.lanes = {70, 71};
    junction.conflicts = {{1, 0}};
    junction.listedLanes = {0};
    require(!trafficJunctionAllowsCar(junction, 71), "pair (1, 0): lane 1 meets listed lane 0");
    require(trafficJunctionAllowsCar(junction, 70), "pair (1, 0): lane 0's partner 1 is not listed");
    junction.conflicts = {{1, 1}};
    junction.listedLanes = {1};
    require(!trafficJunctionAllowsCar(junction, 71), "pair (1, 1): the partner is the lane itself");
}

TrafficRouteTable openRoutes() {
    TrafficRouteTable table;
    table.loaded = true;
    table.routes = {1, 2, 3};
    table.enabled = {true, false, true};
    return table;
}

void follower() {
    const TrafficRouteTable table = openRoutes();
    TrafficRouteQuery query;
    query.position = {0.0f, 0.0f, 0.0f};
    query.forward = {0.0f, 0.0f, 1.0f};
    const std::vector<TrafficRoutePoint> cell = {
        {{0.0f, 0.0f, 5.0f}, 2, 1.0f},
        {{0.0f, 0.0f, -3.0f}, 1, 2.0f},
        {{0.0f, 0.0f, 20.0f}, 1, 3.0f},
        {{1.0f, 0.0f, 10.0f}, 3, 4.0f},
        {{0.0f, 0.0f, 10.05f}, 1, 5.0f},
        {{0.0f, 0.0f, 30.0f}, 9, 6.0f},
    };
    const TrafficRouteHit hit = findTrafficRoutePoint(cell, table, query);
    require(hit.found && hit.route == 3 && near(hit.parameter, 4.0f) && near(hit.position[0], 1.0f) && near(hit.position[2], 10.0f),
            "0x2bf2ec: the nearest enabled point ahead (route 2 is disabled, z = -3 is behind, route 9 is in no list)");

    const std::vector<TrafficRoutePoint> tie = {{{3.0f, 0.0f, 4.0f}, 1, 1.0f}, {{-3.0f, 0.0f, 4.0f}, 3, 2.0f}};
    require(findTrafficRoutePoint(tie, table, query).route == 1, "equal distances: the first point stays (strict compare)");

    TrafficRouteQuery shortForward = query;
    shortForward.forward = {0.0f, 0.0f, 0.5f};
    const std::vector<TrafficRoutePoint> ahead = {{{0.0f, 0.0f, 5.0f}, 1, 1.0f}};
    require(!findTrafficRoutePoint(ahead, table, shortForward).found, "dot exactly 0.5 (bits 0x3f000000) is not above");
    shortForward.forward = {0.0f, 0.0f, std::nextafter(0.5f, 1.0f)};
    require(findTrafficRoutePoint(ahead, table, shortForward).found, "the next float above 0.5 is");
}

void followerBounds() {
    const TrafficRouteTable table = openRoutes();
    TrafficRouteQuery query;
    query.forward = {0.0f, 0.0f, 1.0f};
    require(!findTrafficRoutePoint({{{0.0f, 0.0f, 9999.0f}, 1, 0.0f}}, table, query).found, "9999 is not below the start distance 9999");
    require(findTrafficRoutePoint({{{0.0f, 0.0f, 9998.0f}, 1, 0.0f}}, table, query).found, "9998 is");
    require(findTrafficRoutePoint({{{0.0f, 0.0f, 0.0f}, 1, 7.0f}}, table, query).found,
            "a point at the position: 0x257ea0 gives (0, 0, 1), dot 1 with forward z");
    TrafficRouteQuery sideways = query;
    sideways.forward = {1.0f, 0.0f, 0.0f};
    require(!findTrafficRoutePoint({{{0.0f, 0.0f, 0.0f}, 1, 7.0f}}, table, sideways).found, "the same point with forward x: dot 0");
    const float nan = std::numeric_limits<float>::quiet_NaN();
    require(!findTrafficRoutePoint({{{nan, 0.0f, 0.0f}, 1, 0.0f}}, table, query).found, "a NaN distance fails the strict compare");
    require(!findTrafficRoutePoint({}, table, query).found, "an empty cell finds nothing");
}

void cellLookup() {
    chase::assets::PregenSpawns spawns;
    spawns.points = {{{1.0f, 0.0f, 1.0f}, 0, 11, 0.5f}, {{2.0f, 0.0f, 2.0f}, 0, 12, 1.5f}, {{3.0f, 0.0f, 3.0f}, 0, 13, 2.5f}};
    chase::assets::PregenSpawnCell root;
    root.halfSize = 100;
    root.points = {2, 0};
    spawns.cells.push_back(root);
    const std::vector<TrafficRoutePoint> cell = trafficRouteCell(spawns, 0.0f, 0.0f);
    require(cell.size() == 2 && cell[0].route == 13 && near(cell[0].parameter, 2.5f) && cell[1].route == 11 && near(cell[1].position[0], 1.0f),
            "the cell's points in list order: position, path hash (+0x18), parameter (+0x20)");
    spawns.cells[0].points.clear();
    require(trafficRouteCell(spawns, 0.0f, 0.0f).empty(), "an empty leaf gives no point (0x1f47f0 returns it, its count is 0)");
}

void routeType() {
    LevelPath path;
    path.type = 0;
    path.points = {{0.0f, 0.0f, 0.0f}, {10.0f, 0.0f, 0.0f}, {20.0f, 0.0f, 0.0f}};
    const TrafficRouteSample spline = sampleTrafficRoute(path, 1.0f);
    require(near(spline.position[0], 10.0f) && near(spline.tangent[0], 1.0f) && near(spline.tangent[2], 0.0f),
            "type 0 (0x1c17c0): at an integer parameter the point itself, the unit tangent along the path");
    path.type = 1;
    const TrafficRouteSample linear = sampleTrafficRoute(path, 0.5f);
    require(near(linear.position[0], 5.0f) && near(linear.tangent[0], 1.0f), "type 1 (0x1b33fc): linear between the points");
    path.type = 2;
    bool refused = false;
    try {
        sampleTrafficRoute(path, 0.5f);
    } catch (const std::invalid_argument&) {
        refused = true;
    }
    require(refused, "table 0x3581d4 has entries for types 0 and 1 only");
}

struct ShippedLevel {
    const char* level;
    const char* blvl;
    std::size_t junctions;
    std::size_t routes;
    std::size_t pointPaths;
    std::size_t points;
    std::size_t pathsIntoJunction;
};

std::string stringValue(const chase::assets::LevelParam& param) {
    if (param.blocks.empty() || param.blocks[0].empty()) return {};
    const auto& block = param.blocks[0];
    std::size_t length = 0;
    while (length < block.size() && block[length] != 0) ++length;
    return std::string(reinterpret_cast<const char*>(block.data()), length);
}

int shipped(const std::string& fib, const std::string& csv) {
    if (!std::ifstream(fib) || !std::ifstream(csv)) {
        std::cout << "game data not extracted; skipping\n";
        return 77;
    }
    // Reference values: 20261025-traffic-A, 2026-10-25, `build/traffic-route-test <fib> <csv>` over the shipped levels (object params of
    // class props/traffic_junction, InputPath01..08 / OutputPath01..08 as named paths; docs/re/traffic.md).
    const ShippedLevel levels[] = {
        {"lvl_01_cherrytreehills", "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl", 16, 58, 57, 649, 54},
        {"lvl_03_auburn", "levels/levels/lvl_03_auburn/lvl_03_auburn_main.blvl", 7, 31, 30, 469, 26},
        {"lvl_04_fortmeadows", "levels/levels/lvl_04_fortmeadows/lvl_04_fortmeadows_main.blvl", 2, 3, 2, 231, 0},
        {"lvl_05_bluebellwood", "levels/levels/lvl_05_bluebellwood/lvl_05_bluebellwood_main.blvl", 4, 5, 4, 187, 4},
        {"lvl_06_apolloisland", "levels/levels/lvl_06_apolloisland/lvl_06_apolloisland_main.blvl", 4, 4, 4, 158, 4},
        {"lvl_07_downtown", "levels/levels/lvl_07_downtown/lvl_07_downtown_main.blvl", 14, 40, 38, 727, 36},
        {"lvl_test_playground_traffic", "levels/levels/lvl_test_playground_traffic.blvl", 4, 14, 13, 123, 13},
    };
    std::vector<std::uint32_t> inputNames, outputNames;
    for (int n = 1; n <= 8; ++n) {
        char name[16];
        std::snprintf(name, sizeof name, "InputPath%02d", n);
        inputNames.push_back(chase::assets::nameHash(name));
        std::snprintf(name, sizeof name, "OutputPath%02d", n);
        outputNames.push_back(chase::assets::nameHash(name));
    }
    chase::assets::FuseArchive archive(fib);
    archive.loadNames(csv);
    for (const ShippedLevel& want : levels) {
        const std::string where = want.level;
        const chase::assets::FuseEntry* levelEntry = archive.find(want.blvl);
        const chase::assets::FuseEntry* spawnEntry = archive.find(chase::assets::pregenSpawnPath(chase::assets::PregenSpawnKind::Traffic, want.level));
        require(levelEntry && spawnEntry, where + ": file not found");
        const chase::assets::Level level = chase::assets::parseLevel(archive.read(*levelEntry));
        const chase::assets::PregenSpawns spawns = chase::assets::parsePregenSpawns(archive.read(*spawnEntry), chase::assets::PregenSpawnKind::Traffic);
        std::vector<TrafficJunction> junctions;
        for (const chase::assets::LevelObject& object : level.objects) {
            if (object.className != "props/traffic_junction") continue;
            TrafficJunction junction;
            bool placed = false;
            for (const chase::assets::LevelParam& param : object.params)
                if (param.hash == chase::assets::nameHash("initialposition") && param.type == 2 && !param.blocks.empty() &&
                    param.blocks[0].size() >= 12) {
                    std::memcpy(junction.position.data(), param.blocks[0].data(), 12);
                    placed = true;
                }
            require(placed, where + ": junction " + object.instance + " has no initialposition");
            for (std::uint32_t name : inputNames)
                for (const chase::assets::LevelParam& param : object.params)
                    if (param.hash == name) junction.inputs.push_back(chase::assets::nameHash(stringValue(param)));
            for (std::uint32_t name : outputNames)
                for (const chase::assets::LevelParam& param : object.params)
                    if (param.hash == name) junction.outputs.push_back(chase::assets::nameHash(stringValue(param)));
            junctions.push_back(junction);
        }
        require(junctions.size() == want.junctions, where + ": junction count " + std::to_string(junctions.size()));
        TrafficRouteTable table;
        table.loaded = true;
        require(collectTrafficRoutes(table, junctions) == want.junctions, where + ": every junction lies within 9000 of the origin in x and z");
        require(table.routes.size() == want.routes, where + ": route count " + std::to_string(table.routes.size()));
        std::set<std::uint32_t> pointPaths;
        for (const chase::assets::PregenSpawnPoint& point : spawns.points) {
            require(trafficRouteEnabled(table, point.pathHash), where + ": a point off the junction paths (TRAFFIC-S3 would apply)");
            pointPaths.insert(point.pathHash);
        }
        require(spawns.points.size() == want.points && pointPaths.size() == want.pointPaths,
                where + ": points " + std::to_string(spawns.points.size()) + " paths " + std::to_string(pointPaths.size()));
        for (std::uint32_t hash : pointPaths) {
            const LevelPath* path = nullptr;
            for (const LevelPath& candidate : level.paths)
                if (candidate.nameHash == hash) {
                    path = &candidate;
                    break;
                }
            require(path && path->type == 0 && !path->closed, where + ": every traffic route is an open type 0 path (0x1c17c0)");
        }
        const TrafficRouteSignals linked = linkTrafficRouteSignals(table, junctions);
        std::size_t withJunction = 0;
        for (std::uint32_t hash : pointPaths)
            if (trafficSignalForRoute(linked, hash)) ++withJunction;
        require(withJunction == want.pathsIntoJunction, where + ": point paths that are a junction input " + std::to_string(withJunction));
    }
    std::cout << "traffic-route: shipped levels ok\n";
    return 0;
}

}

int main(int argc, char** argv) {
    try {
        routeList();
        enablePath();
        signals();
        junctionTest();
        junctionPairOrder();
        follower();
        followerBounds();
        cellLookup();
        routeType();
        if (argc == 3) return shipped(argv[1], argv[2]);
        if (argc != 1) throw std::runtime_error("usage: traffic-route-test [<fib> <csv>]");
    } catch (const std::exception& error) {
        std::cerr << "traffic-route: " << error.what() << '\n';
        return 1;
    }
    std::cout << "traffic-route: ok\n";
    return 0;
}
