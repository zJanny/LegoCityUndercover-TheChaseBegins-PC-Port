// Tests of src/runtime/traffic_junction_update.*: the junction records kept from the scene enter, the class update 0x1d8924 run over them
// as the entity pass 0x1a3f80 calls it, and the route functions over their paths. Synthetic expectations follow from the disassembly read
// by 20261103-large-A (docs/re/traffic-junction.md §2026-11-03T08:53:43) and 20261107-traffic-A (§ of 2026-11-07, the entity pass). With
// <fib> <csv> it also checks lvl_01 (traffic-junction-update-real-data).
#include "runtime/traffic_junction_update.hpp"

#include "assets/assets.hpp"
#include "runtime/traffic_route_setup.hpp"

#include <array>
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

assets::LevelPath path(const std::string& name, std::vector<std::array<float, 3>> points) {
    assets::LevelPath result;
    result.nameHash = assets::nameHash(name);
    result.points = std::move(points);
    return result;
}

struct Crossing {
    assets::Level level;
    std::vector<TrafficJunctionFill> fills;
};

Crossing crossing() {
    Crossing result;
    result.level.paths = {path("South", {{0, 0, -20}, {0, 0, -5}}), path("West", {{-20, 0, 0}, {-5, 0, 0}}),
                          path("North", {{0, 0, 5}, {0, 0, 10}, {0, 0, 20}}), path("East", {{5, 0, 0}, {20, 0, 0}})};
    const std::vector<assets::LevelPath>& p = result.level.paths;
    TrafficJunctionSource lit;
    lit.inputs = {&p[0], &p[1]};
    lit.outputs = {&p[2], &p[3]};
    lit.lights = {{TrafficLight{{p[0].nameHash}}, std::nullopt}, {TrafficLight{{p[1].nameHash}}, std::nullopt}};
    result.fills.push_back(fillTrafficJunction(lit, 700));
    TrafficJunctionSource plain;
    plain.inputs = {&p[2]};
    plain.outputs = {&p[3]};
    plain.lights = {{TrafficLight{{p[2].nameHash}}, std::nullopt}};
    result.fills.push_back(fillTrafficJunction(plain, 704));
    return result;
}

TrafficJunctions kept(const Crossing& source) {
    TrafficRouteTable table;
    const TrafficRoutes routes = setUpTrafficJunctionRoutes(table, trafficJunctionLayouts(source.fills));
    return keepTrafficJunctions(source.fills, {&source.level}, routes.signals);
}

void keep() {
    const Crossing source = crossing();
    const TrafficJunctions junctions = kept(source);
    require(junctions.world.junctions.size() == 2 && junctions.world.junctions[0].layout.lanes == std::vector<std::uint32_t>({700, 701, 702, 703}),
            "the records stay in object order with their lane ids");
    require(junctions.paths.size() == 4 + 5, "four level routes (North, East shared by both junctions) and five lanes");
    require(junctions.paths.find(assets::nameHash("North"))->points.size() == 3 && junctions.paths.find(702)->points.size() == kTrafficLanePoints &&
                !junctions.paths.find(assets::nameHash("Nowhere")),
            "a route finds its level path, a lane its own 4-point path");
    require(junctions.world.signals.inputs.size() == 3, "0x2dfc84: one input pair per input route of each junction");
}

void cycle() {
    const Crossing source = crossing();
    TrafficJunctions junctions = kept(source);
    TrafficJunctionWorld& world = junctions.world;
    std::vector<std::pair<int, TrafficJunctionLightEvent>> seen;
    for (int update = 1; update <= 24; ++update)
        for (const TrafficJunctionLightEvent& event : runTrafficJunctionUpdates(world, 0.5f)) seen.push_back({update, event});
    require(seen.size() == 3, "0x1d893c: only the junction with two pairs cycles");
    require(seen[0].first == 12 && seen[0].second.junction == 0 && seen[0].second.change.phase == kTrafficLightYellow && seen[0].second.change.pair == 0,
            "green 6 s: yellow after 12 steps of 0.5 s");
    require(seen[1].first == 16 && seen[1].second.change.phase == kTrafficLightRed, "yellow 2 s");
    require(seen[2].first == 22 && seen[2].second.change.phase == kTrafficLightGreen && seen[2].second.change.pair == 1 && world.junctions[0].passPair == 1,
            "red 3 s, then the next pair turns green");
    require(world.junctions[1].lightTimer == kTrafficLightGreenTime, "the single pair's timer stands");

    world.junctions[0].lightPhase = kTrafficLightRed;
    world.junctions[0].lightTimer = 0.0f;
    world.junctions[0].queuedCars = {5};
    const std::vector<TrafficJunctionLightEvent> waiting = runTrafficJunctionUpdates(world, 0.5f);
    require(waiting.size() == 1 && waiting[0].change.phase == kTrafficLightRed && waiting[0].change.previous == kTrafficLightRed,
            "0x1d89a4: red over red while a car is queued");
}

void functions() {
    const Crossing source = crossing();
    const TrafficJunctions junctions = kept(source);
    const AiRouteFunctions routes = trafficRouteFunctions(junctions);
    require(routes.length(assets::nameHash("North")) == 3 && routes.length(701) == 4, "route hword +10: the point count");
    AiVec3 position{}, direction{};
    routes.evaluate(701, 0.0f, &position, &direction);
    require(position == AiVec3({0, 0, -5}), "slot 0 of type 0 at 0: the lane starts at the input's end");
    routes.evaluate(assets::nameHash("North"), 2.0f, &position, nullptr);
    require(position == AiVec3({0, 0, 20}), "and a level route ends at its last point");
    require(routes.signal(assets::nameHash("West")) == 1 && routes.signal(assets::nameHash("North")) == 2 && routes.signal(assets::nameHash("East")) == 0,
            "0x20ff40: the junction of the first input pair (index + 1), 0 for an output only");
    bool threw = false;
    try {
        routes.length(12345);
    } catch (const std::out_of_range&) {
        threw = true;
    }
    require(threw, "a route without a path is refused");
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
    TrafficJunctions junctions = keepTrafficJunctions(std::move(fills), {&level}, routes.signals);
    // Reference: 20261107-traffic-A, 2026-11-07, keepTrafficJunctions over lvl_01: the 57 routes of the route list (traffic-route-setup-real-data)
    // and the 97 lanes of traffic-junction-fill-real-data each have a path.
    require(junctions.world.junctions.size() == 16 && junctions.paths.size() == 57 + 97, "lvl_01: 16 junctions, 154 route paths");
    for (const std::uint32_t route : table.routes) require(junctions.paths.find(route) != nullptr, "lvl_01: every listed route has a path");
    std::size_t first = 0, cycling = 0;
    for (std::size_t update = 1; update <= 200 && cycling == 0; ++update) {
        const std::vector<TrafficJunctionLightEvent> events = runTrafficJunctionUpdates(junctions.world, 1.0f / 30.0f);
        if (!events.empty()) {
            first = update;
            cycling = events.size();
        }
    }
    // Same run: the timer 6.0 minus 1/30 per update reaches 0 at update 181 (float steps); 13 of the 16 junctions have two or more light pairs.
    require(first == 181 && cycling == 13, "lvl_01: first light change at update " + std::to_string(first) + " in " + std::to_string(cycling) + " junctions");
    std::cout << "traffic-junction-update: lvl_01 ok\n";
    return 0;
}

}

int main(int argc, char** argv) {
    try {
        keep();
        cycle();
        functions();
        if (argc == 3) return shipped(argv[1], argv[2]);
        std::cout << "traffic-junction-update: ok\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "traffic-junction-update-test: FAIL: " << error.what() << '\n';
        return 1;
    }
}
