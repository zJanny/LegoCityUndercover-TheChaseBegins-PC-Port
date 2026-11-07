// Tests of src/runtime/traffic_route_functions.*: slots +8 (ahead, 0x29fb6c) and +0xc (nearest, 0x21d5b0) of the route type table
// 0x3581d4 and their use by trafficRouteFunctions. The synthetic expectations are hand-evaluated from the disassembly read by
// 20261107-traffic2-B on 2026-11-07 (llvm-objdump of 0x29fb6c..0x29fcd0, 0x21d5b0..0x21d6b8, 0x29fa58, 0x2878f0, the table words at
// 0x3581d4; docs/re/traffic-route-functions.md): on a straight type 0 path with points 10 apart the inner segments are linear, so the
// samples at whole and half parameters lie exactly on 10 x parameter. With <fib> <csv> it also checks the lvl_01 junction paths
// (traffic-route-functions-real-data).
#include "runtime/traffic_route_functions.hpp"

#include "assets/assets.hpp"
#include "runtime/traffic_junction_update.hpp"
#include "runtime/traffic_route_setup.hpp"

#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using namespace chase;
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

assets::LevelPath path(std::uint8_t type, bool closed, std::vector<std::array<float, 3>> points) {
    assets::LevelPath result;
    result.type = type;
    result.closed = closed;
    result.points = std::move(points);
    return result;
}

assets::LevelPath straight(std::size_t count) {
    std::vector<std::array<float, 3>> points;
    for (std::size_t i = 0; i < count; ++i) points.push_back({10.0f * float(i), 0.0f, 0.0f});
    return path(0, false, points);
}

assets::LevelPath square(bool closed) {
    return path(0, closed, {{0, 0, 0}, {10, 0, 0}, {10, 0, 10}, {0, 0, 10}});
}

void nearest() {
    const assets::LevelPath line = straight(21);
    require(trafficRouteNearest(line, {13.0f, 5.0f, 0.0f}, 10, 1.0f) == 13.0f / 10.0f, "0x21d5b0: the sample k / 10 nearest to the position");
    require(trafficRouteNearest(line, {13.0f, 5.0f, 0.0f}, 10, 2.0f) == 13.0f / 10.0f, "the scale divides every distance alike");
    require(trafficRouteNearest(line, {50.0f, 0.0f, 0.0f}, 10, 1.0f) == 5.0f, "a path point is found at its index");
    require(trafficRouteNearest(line, {0.0f, 0.0f, 0.0f}, 10, 1.0f) == 1.0f / 10.0f, "k starts at 1: parameter 0 is never sampled");
    require(trafficRouteNearest(line, {500.0f, 0.0f, 0.0f}, 10, 1.0f) == 20.0f, "an open path ends at k = (count - 1) x samples");
    require(trafficRouteNearest(line, {17.5f, 0.0f, 0.0f}, 2, 1.0f) == 1.5f, "movlo: of two equal distances (15 and 20) the first is kept");
    require(trafficRouteNearest(square(true), {0.0f, 0.0f, 0.0f}, 10, 1.0f) == 4.0f, "a closed path samples count x samples, up to its start again");
    require(trafficRouteNearest(square(false), {0.0f, 0.0f, -1.0f}, 10, 1.0f) == 1.0f / 10.0f, "open: the first segment only from k = 1");
    require(trafficRouteNearest(path(0, false, {{5, 0, 5}}), {5.0f, 0.0f, 5.0f}, 10, 1.0f) == 0.0f, "one point: no sample, the result stays 0");
    require(trafficRouteNearest(path(1, false, {{0, 0, 0}, {10, 0, 0}}), {10.0f, 0.0f, 0.0f}, 10, 1.0f) == 0.0f, "type 1: the stub 0x2878f0");
}

void ahead() {
    const assets::LevelPath line = straight(21);
    require(trafficRouteAhead(line, 8.0f, 0.5f, 1.0f) == 2.5f, "0x29fb6c below 100: squared steps 25, 50, 75 reach 8 x 8 after three steps");
    require(trafficRouteAhead(line, 100.0f, 0.5f, 1.0f) == 11.0f, "at 100: twenty steps of 5 reach 100");
    require(trafficRouteAhead(line, 8.0f, 0.5f, 19.5f) == 20.0f, "the walk stops at the open path's last parameter");
    require(trafficRouteAhead(line, 8.0f, 0.5f, 25.0f) == 25.0f, "a start beyond the end comes back unchanged");
    require(trafficRouteAhead(line, 0.0f, 0.5f, 3.0f) == 3.0f, "nothing to walk: the start");
    require(trafficRouteAhead(square(true), 8.0f, 0.5f, 3.5f) == 4.0f, "closed: the limit is the point count (one step of 26.5625)");
    require(trafficRouteAhead(square(false), 8.0f, 0.5f, 3.5f) == 3.5f, "open: the limit is count - 1");
    require(trafficRouteAhead(path(1, false, {{0, 0, 0}, {10, 0, 0}}), 8.0f, 0.5f, 0.5f) == 0.0f, "type 1: the stub 0x29fa58");
}

void refused() {
    bool threw = false;
    try {
        trafficRouteNearest(path(2, false, {{0, 0, 0}, {1, 0, 0}}), {0.0f, 0.0f, 0.0f}, 10, 1.0f);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    require(threw, "only types 0 and 1 exist (the words after the tenth of 0x3581d4 are text)");
    threw = false;
    try {
        trafficRouteAhead(path(0, false, {}), 8.0f, 0.5f, 0.0f);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    require(threw, "a type 0 path without points is refused");
}

void wired() {
    assets::Level level;
    level.paths.push_back(straight(21));
    level.paths.back().nameHash = assets::nameHash("Road");
    level.paths.push_back(path(0, false, {{200, 0, 5}, {200, 0, 20}}));
    level.paths.back().nameHash = assets::nameHash("Away");
    TrafficJunctionSource source;
    source.inputs = {&level.paths[0]};
    source.outputs = {&level.paths[1]};
    std::vector<TrafficJunctionFill> fills = {fillTrafficJunction(source, 700)};
    TrafficRouteTable table;
    const TrafficRoutes routes = setUpTrafficJunctionRoutes(table, trafficJunctionLayouts(fills));
    const TrafficJunctions junctions = keepTrafficJunctions(std::move(fills), {&level}, routes.signals);
    const AiRouteFunctions functions = trafficRouteFunctions(junctions);
    const std::uint32_t road = assets::nameHash("Road");
    const float t0 = functions.nearest(road, {13.0f, 5.0f, 0.0f}, kAiNearestSamples, kAiNearestScale);
    require(t0 == 13.0f / 10.0f, "nearest is slot +0xc of the route's type");
    require(functions.ahead(road, kAiSteerLookAhead, kAiSteerLookStep, 1.0f) == 2.5f, "ahead is slot +8");
    require(functions.nearest(700, {200.0f, 0.0f, 5.0f}, 10, 1.0f) == 3.0f, "a lane is a type 0 path of 4 points: its end at parameter 3");
}

int shipped(const std::string& fib, const std::string& csv) {
    if (!std::ifstream(fib) || !std::ifstream(csv)) {
        std::cout << "game data not extracted; skipping\n";
        return 77;
    }
    assets::FuseArchive archive(fib);
    archive.loadNames(csv);
    const assets::FuseEntry* entry = archive.find("levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl");
    require(entry != nullptr, "lvl_01 not found");
    const assets::Level level = assets::parseLevel(archive.read(*entry));
    std::vector<TrafficJunctionFill> fills = trafficJunctionFills({&level});
    TrafficRouteTable table;
    const TrafficRoutes routes = setUpTrafficJunctionRoutes(table, trafficJunctionLayouts(fills));
    const TrafficJunctions junctions = keepTrafficJunctions(std::move(fills), {&level}, routes.signals);
    const AiRouteFunctions functions = trafficRouteFunctions(junctions);
    std::vector<std::uint32_t> ids = table.routes;
    for (const TrafficJunctionRecord& junction : junctions.world.junctions)
        for (const std::uint32_t lane : junction.layout.lanes) ids.push_back(lane);
    // Reference: 20261107-traffic-A, 2026-11-07: 57 listed routes and 97 lanes in lvl_01 (traffic-junction-update-real-data).
    require(ids.size() == 57 + 97, "lvl_01: 154 route paths");
    std::size_t points = 0;
    for (const std::uint32_t id : ids) {
        const assets::LevelPath& route = *junctions.paths.find(id);
        require(route.type == 0 && !route.closed, "lvl_01: every route and lane is an open type 0 path");
        for (std::size_t i = 1; i < route.points.size(); ++i, ++points) {
            const float t = functions.nearest(id, route.points[i], kAiNearestSamples, kAiNearestScale);
            require(t == float(i), "lvl_01: route " + std::to_string(id) + " point " + std::to_string(i) + " is nearest at " + std::to_string(t));
        }
        const float last = float(route.points.size() - 1);
        const float walked = functions.ahead(id, kAiSteerLookAhead, kAiSteerLookStep, 0.0f);
        require(walked > 0.0f && walked <= last + kAiSteerLookStep && std::fmod(walked, kAiSteerLookStep) == 0.0f,
                "lvl_01: route " + std::to_string(id) + " ahead from 0 moves in whole steps up to the end");
        require(functions.ahead(id, kAiSteerLookAhead, kAiSteerLookStep, last) == last, "lvl_01: ahead from the end stays there");
    }
    std::cout << "traffic-route-functions: lvl_01 ok (" << ids.size() << " paths, " << points << " points)\n";
    return 0;
}

}

int main(int argc, char** argv) {
    try {
        nearest();
        ahead();
        refused();
        wired();
        if (argc == 3) return shipped(argv[1], argv[2]);
        std::cout << "traffic-route-functions: ok\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "traffic-route-functions-test: FAIL: " << error.what() << '\n';
        return 1;
    }
}
