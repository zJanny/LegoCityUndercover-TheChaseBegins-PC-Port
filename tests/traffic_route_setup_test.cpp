// Tests of src/runtime/traffic_route_setup.*: the junctions of a level, the route list and pairs of the room set-up, the route index of a pregen
// point and EnableTrafficPath. Synthetic expectations follow from the disassembly read by 20261025-traffic-A (docs/re/traffic-ai-driver.md):
// 0x1ca5a4 (junction set-up, InputPath01..08 / OutputPath01..08), 0x2df574 / 0x2c8718 (route list, path record +0x24 = index), 0x2dfc84
// (pairs), 0x27bc30 -> 0x2a7004 -> 0x2dece4 (EnableTrafficPath), 0x2dedbc (pending replay). With <fib> <csv> it also checks the shipped
// traffic levels (traffic-route-setup-real-data).
#include "runtime/traffic_route_setup.hpp"

#include "assets/assets.hpp"
#include "assets/pregen_traffic.hpp"

#include <cstring>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using namespace chase;
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

assets::LevelParam bytesParameter(const std::string& name, std::vector<std::uint8_t> bytes) {
    assets::LevelParam param;
    param.hash = assets::nameHash(name);
    param.type = 2;
    param.blocks.push_back(std::move(bytes));
    return param;
}

assets::LevelParam textParameter(const std::string& name, const std::string& text) {
    std::vector<std::uint8_t> bytes(text.begin(), text.end());
    bytes.push_back(0);
    return bytesParameter(name, std::move(bytes));
}

assets::LevelParam positionParameter(float x, float y, float z) {
    std::vector<std::uint8_t> bytes(12);
    const float values[3] = {x, y, z};
    std::memcpy(bytes.data(), values, 12);
    return bytesParameter("initialposition", std::move(bytes));
}

assets::LevelObject junctionObject(const std::string& instance, float x, float z, std::vector<assets::LevelParam> paths) {
    assets::LevelObject object;
    object.className = "props/traffic_junction";
    object.instance = instance;
    object.params.push_back(positionParameter(x, 0.0f, z));
    for (assets::LevelParam& path : paths) object.params.push_back(std::move(path));
    return object;
}

assets::LevelPath road(const std::string& name, float x) {
    assets::LevelPath path;
    path.nameHash = assets::nameHash(name);
    path.points = {{x, 0.0f, -10.0f}, {x, 0.0f, 10.0f}};
    return path;
}

assets::Level syntheticLevel() {
    assets::Level level;
    for (const char* name : {"RoadA", "RoadB", "RoadC", "RoadD", "RoadFar", "Ignored"}) level.paths.push_back(road(name, float(level.paths.size()) * 4.0f));
    level.objects.push_back(junctionObject("J1", 10.0f, 20.0f,
                                           {textParameter("OutputPath02", "RoadC"), textParameter("InputPath01", "RoadA"), textParameter("InputPath03", "RoadB"),
                                            textParameter("OutputPath01", "RoadA"), textParameter("InputPath04", ""), textParameter("OutputPath05", "Missing")}));
    assets::LevelObject other;
    other.className = "props/not_a_junction";
    other.instance = "X";
    other.params.push_back(textParameter("InputPath01", "Ignored"));
    level.objects.push_back(other);
    level.objects.push_back(junctionObject("Far", 9000.0f, 0.0f, {textParameter("InputPath01", "RoadFar")}));
    level.objects.push_back(junctionObject("J2", -30.0f, 40.0f, {textParameter("InputPath02", "roadb"), textParameter("OutputPath01", "RoadD")}));
    return level;
}

void junctions() {
    const assets::Level level = syntheticLevel();
    const std::vector<TrafficJunction> list = trafficJunctions({&level});
    require(list.size() == 3, "only the props/traffic_junction objects are junctions, in object order");
    require(list[0].position == TrafficRouteVec3({10.0f, 0.0f, 20.0f}), "position from initialposition");
    require(list[0].inputs == std::vector<std::uint32_t>({assets::nameHash("RoadA"), assets::nameHash("RoadB")}),
            "inputs in slot order InputPath01..08, empty slots and an empty name skipped (0x1ca5a4, 0x22a3b0 returns 0)");
    require(list[0].outputs == std::vector<std::uint32_t>({assets::nameHash("RoadA"), assets::nameHash("RoadC")}),
            "outputs in slot order OutputPath01..08, a name without a level path skipped");
    require(list[0].lanes.size() == 2 && list[0].listedLanes.empty(), "the lanes of 0x1ca5a4 (side 2 A to A, side 1 B to C; traffic-junction-fill), no car queued");

    assets::Level broken;
    assets::LevelObject object;
    object.className = "props/traffic_junction";
    object.instance = "NoPosition";
    broken.objects.push_back(object);
    bool threw = false;
    try {
        trafficJunctions({&broken});
    } catch (const std::runtime_error&) {
        threw = true;
    }
    require(threw, "a junction without initialposition is refused");
}

void routes() {
    const assets::Level level = syntheticLevel();
    TrafficRouteTable table;
    const TrafficRoutes routes = setUpTrafficRoutes(table, {&level});
    require(routes.junctions.size() == 3 && routes.junctionsInReach == 2, "the junction at x = 9000 is not below 9000 (0x2c8718 strict)");
    require(table.routes == std::vector<std::uint32_t>({assets::nameHash("RoadA"), assets::nameHash("RoadB"), assets::nameHash("RoadC"), assets::nameHash("RoadD")}),
            "per junction its inputs then its outputs, each route once (0x2df574)");
    require(table.enabled == std::vector<bool>({true, true, true, true}), "a new route gets its bit set");
    require(trafficPointRoute(table, assets::nameHash("RoadC")) == 2u, "the route index is the position in the list (path record +0x24)");
    require(!trafficPointRoute(table, assets::nameHash("RoadFar")), "a path outside the list has no index (TRAFFIC-S3)");
    require(trafficSignalForRoute(routes.signals, assets::nameHash("RoadA")) == 0u && trafficSignalForRoute(routes.signals, assets::nameHash("RoadB")) == 0u &&
                !trafficSignalForRoute(routes.signals, assets::nameHash("RoadC")) && routes.signals.outputs.size() == 3,
            "the pairs of 0x2dfc84: input pairs of the first junction, output pairs of J1 (A, C) and J2 (D) (0x20ff40 uses the first input pair)");

    TrafficRouteTable none;
    const TrafficRoutes empty = setUpTrafficRoutes(none, {});
    require(empty.junctionsInReach == 0 && none.routes.empty() && !none.loaded, "without junctions no route list and nothing loaded");
}

void enableNative() {
    const assets::Level level = syntheticLevel();
    TrafficRouteTable table;
    runEnableTrafficPath(table, "ROADB", false);
    runEnableTrafficPath(table, "roadb", false);
    runEnableTrafficPath(table, "RoadA", true);
    require(table.pending == std::vector<std::uint32_t>({assets::nameHash("RoadB")}),
            "before the load a disable is queued once, an enable does nothing (0x2dece4), the name is case-insensitive");
    setUpTrafficRoutes(table, {&level});
    table.loaded = true;
    applyPendingTrafficRoutes(table);
    require(!trafficRouteEnabled(table, assets::nameHash("RoadB")) && trafficRouteEnabled(table, assets::nameHash("RoadA")) && table.pending.empty(),
            "the room set-up replays the queued disable and empties the list (0x2dedbc)");
    runEnableTrafficPath(table, "RoadB", true);
    runEnableTrafficPath(table, "RoadA", false);
    runEnableTrafficPath(table, "Unknown", false);
    require(trafficRouteEnabled(table, assets::nameHash("RoadB")) && !trafficRouteEnabled(table, assets::nameHash("RoadA")) && table.pending.empty(),
            "once loaded the bit is set or cleared and an unknown path is ignored");
}

struct ShippedLevel {
    const char* level;
    const char* blvl;
    std::size_t junctions;
    std::size_t routes;
    std::size_t pointPaths;
    std::size_t points;
};

int shipped(const std::string& fib, const std::string& csv) {
    if (!std::ifstream(fib) || !std::ifstream(csv)) {
        std::cout << "game data not extracted; skipping\n";
        return 77;
    }
    // Reference: 20261025-traffic-A, 2026-10-25 (docs/re/traffic-ai-driver.md, test traffic-route-real-data): junction objects, route list and
    // distinct pregen point paths per shipped traffic level; reproduced through setUpTrafficRoutes by 20261102-traffic-A. The route counts are
    // those of 20261103-large-A, 2026-11-03: a junction path name without a level path (empty, or "0" in three junctions) is skipped as 0x22a3b0
    // returns 0, so every route is a pregen point path.
    const ShippedLevel levels[] = {
        {"lvl_01_cherrytreehills", "levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl", 16, 57, 57, 649},
        {"lvl_03_auburn", "levels/levels/lvl_03_auburn/lvl_03_auburn_main.blvl", 7, 30, 30, 469},
        {"lvl_04_fortmeadows", "levels/levels/lvl_04_fortmeadows/lvl_04_fortmeadows_main.blvl", 2, 2, 2, 231},
        {"lvl_05_bluebellwood", "levels/levels/lvl_05_bluebellwood/lvl_05_bluebellwood_main.blvl", 4, 4, 4, 187},
        {"lvl_06_apolloisland", "levels/levels/lvl_06_apolloisland/lvl_06_apolloisland_main.blvl", 4, 4, 4, 158},
        {"lvl_07_downtown", "levels/levels/lvl_07_downtown/lvl_07_downtown_main.blvl", 14, 38, 38, 727},
        {"lvl_test_playground_traffic", "levels/levels/lvl_test_playground_traffic.blvl", 4, 13, 13, 123},
    };
    assets::FuseArchive archive(fib);
    archive.loadNames(csv);
    for (const ShippedLevel& want : levels) {
        const std::string where = want.level;
        const assets::FuseEntry* levelEntry = archive.find(want.blvl);
        const assets::FuseEntry* spawnEntry = archive.find(assets::pregenSpawnPath(assets::PregenSpawnKind::Traffic, want.level));
        require(levelEntry && spawnEntry, where + ": file not found");
        const assets::Level level = assets::parseLevel(archive.read(*levelEntry));
        const assets::PregenSpawns spawns = assets::parsePregenSpawns(archive.read(*spawnEntry), assets::PregenSpawnKind::Traffic);
        TrafficRouteTable table;
        const TrafficRoutes routes = setUpTrafficRoutes(table, {&level});
        require(routes.junctions.size() == want.junctions && routes.junctionsInReach == want.junctions,
                where + ": junctions " + std::to_string(routes.junctions.size()));
        require(table.routes.size() == want.routes, where + ": routes " + std::to_string(table.routes.size()));
        std::set<std::uint32_t> indexes;
        for (const assets::PregenSpawnPoint& point : spawns.points) {
            const std::optional<std::uint32_t> index = trafficPointRoute(table, point.pathHash);
            require(index && table.routes[*index] == point.pathHash && trafficRouteEnabled(table, point.pathHash), where + ": a point off the route list");
            indexes.insert(*index);
        }
        require(spawns.points.size() == want.points && indexes.size() == want.pointPaths, where + ": points " + std::to_string(spawns.points.size()) + " paths " +
                                                                                              std::to_string(indexes.size()));
    }
    std::cout << "traffic-route-setup: shipped levels ok\n";
    return 0;
}

}

int main(int argc, char** argv) {
    try {
        junctions();
        routes();
        enableNative();
        if (argc == 3) return shipped(argv[1], argv[2]);
        std::cout << "traffic-route-setup: ok\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "traffic-route-setup-test: FAIL: " << error.what() << '\n';
        return 1;
    }
}
