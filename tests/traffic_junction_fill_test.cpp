// Tests of src/runtime/traffic_junction_fill.*: the junction set-up 0x1ca5a4 and the light update 0x1d8924. Synthetic expectations follow
// from the disassembly read by 20261103-large-A, 2026-11-03 (docs/re/traffic-junction.md). With <fib> <csv> it also checks the shipped
// levels (traffic-junction-fill-real-data).
#include "runtime/traffic_junction_fill.hpp"

#include "assets/assets.hpp"

#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using namespace chase;
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b, float tolerance = 1e-4f) {
    return std::fabs(a - b) <= tolerance;
}

template <typename Action>
bool throws(Action action) {
    try {
        action();
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

assets::LevelPath path(const std::string& name, std::vector<std::array<float, 3>> points) {
    assets::LevelPath result;
    result.nameHash = assets::nameHash(name);
    result.points = std::move(points);
    return result;
}

void ends() {
    const assets::LevelPath road = path("Road", {{5, 0, -20}, {0, 1, -10}, {0, 0, -4}});
    const TrafficJunctionEnd in = trafficJunctionInputEnd(road);
    require(in.route == assets::nameHash("Road") && in.point == TrafficRouteVec3({0, 0, -4}) && in.inner == TrafficRouteVec3({0, 1, -10}),
            "0x1ca700: an input ends at its last point, +0x10 is the point before");
    require(near(in.direction[0], 0.0f) && near(in.direction[1], 6.0f / std::sqrt(37.0f)) && in.side == 0,
            "0x1ca744..0x1ca760: +0x1c holds x and z of the unit 3D direction (not renormalised in x/z)");
    const TrafficJunctionEnd out = trafficJunctionOutputEnd(road);
    require(out.point == TrafficRouteVec3({5, 0, -20}) && out.inner == TrafficRouteVec3({0, 1, -10}) &&
                near(out.direction[0], -5.0f / std::sqrt(126.0f)) && near(out.direction[1], 10.0f / std::sqrt(126.0f)),
            "0x1ca784..0x1ca7d4: an output starts at its first point, toward the second");
    const assets::LevelPath still = path("Still", {{1, 2, 3}, {1, 2, 3}});
    const TrafficJunctionEnd zero = trafficJunctionInputEnd(still);
    require(zero.direction[0] == 0.0f && zero.direction[1] == 1.0f, "0x257ea0: a zero direction becomes (0, 0, 1)");
    require(throws([] { trafficJunctionInputEnd(path("Dot", {{0, 0, 0}})); }), "STAND-IN TRAFFIC-S13: a one-point path is refused");
}

void sides() {
    std::vector<TrafficJunctionEnd> list(3);
    list[0].point = {-2, 0, 0};
    list[0].direction = {0, 1};
    list[1].point = {2, 0, 0};
    list[1].direction = {0, 1};
    list[2].point = {0, 0, 5};
    list[2].direction = {1, 0};
    markTrafficJunctionSides(list);
    require(list[0].side == 2 && list[1].side == 1 && list[2].side == 0,
            "0x159ab8: parallel ends get 2 and 1 by the side of a against (-b.z, b.x); a crossing end keeps 0");
    std::swap(list[0], list[1]);
    list[0].side = list[1].side = 0;
    markTrafficJunctionSides(list);
    require(list[0].side == 1 && list[1].side == 2, "0x159b60: a negative side gives a = 1, b = 2");
    list[1].direction = {0.45f, 0.89f};
    list[0].side = list[1].side = 0;
    markTrafficJunctionSides(list);
    require(list[0].side == 0 && list[1].side == 0, "0x159b10: a dot below 0.9 by its bits is no pair");
}

TrafficJunctionEnd end(float x, float z, float dx, float dz, std::uint8_t side = 0) {
    TrafficJunctionEnd result;
    result.point = {x, 0, z};
    result.direction = {dx, dz};
    result.side = side;
    return result;
}

void choices() {
    const TrafficJunctionEnd north = end(0, -5, 0, 1);
    const std::vector<TrafficJunctionEnd> outputs = {end(0, 5, 0, 1), end(5, 0, 1, 0), end(-5, 0, -1, 0), end(-1, -5, 0, -1)};
    require(trafficJunctionChoices(north, outputs, 2) == std::vector<std::size_t>({0, 1, 2}),
            "0x1ca97c: the turn back (dot bits from 0xbf59999a) is no choice, the rest are taken without sides");
    require(trafficJunctionChoices(north, {end(0, 5, 0, -1)}, 1) == std::vector<std::size_t>({0}),
            "0x1ca944: one input and one output always join, even backwards");
    require(trafficJunctionChoices(north, {end(0, 5, 0, -1)}, 2).empty(), "with two inputs the backward output is refused");
    const std::vector<TrafficJunctionEnd> paired = {end(1, 5, 0, 1, 1), end(-1, 5, 0, 1, 2), end(5, 0, 1, 0), end(-5, 0, -1, 0)};
    require(trafficJunctionChoices(end(1, -5, 0, 1, 1), paired, 2) == std::vector<std::size_t>({0, 2}),
            "0x1ca998, 0x1caa4c: side 1 keeps its own side and the turn whose cross product (x0 z1 - z0 x1) is not above 0");
    require(trafficJunctionChoices(end(-1, -5, 0, 1, 2), paired, 2) == std::vector<std::size_t>({1, 3}),
            "0x1caa40: side 2 keeps its own side and the turn whose cross product is above 0");
    require(trafficJunctionChoices(end(-1, -5, 0, 1, 2), {end(0, 5, 0, 1), end(5, 0, 1, 0)}, 2) == std::vector<std::size_t>({0}),
            "0x1caa10: straight on (dot above 0.9) is taken whatever the side, the turn of the other side is not");
}

void lanes() {
    const TrafficJunctionEnd in = end(0, 0, 0, 1);
    const TrafficJunctionEnd out = end(10, 10, 1, 0);
    const assets::LevelPath lane = trafficJunctionLanePath(in, out, 77);
    require(lane.nameHash == 77 && lane.type == 0 && !lane.closed && lane.points.size() == kTrafficLanePoints,
            "0x1cab94..0x1caba4: four points, type 0 of the zero-filled lane");
    require(lane.points[0] == in.point && lane.points[3] == out.point, "0x1cac10, 0x1cac54: the lane joins the two ends");
    // Hermite 0x20b0c0 by hand: reach sqrt(200), tangents (0, 0, reach) and (reach, 0, 0).
    require(near(lane.points[1][0], 1.545027f) && near(lane.points[1][1], 0.0f) && near(lane.points[1][2], 4.687724f),
            "0x20b0c0 at 1/3: P0 h00 + T0 h10 + P3 h01 + T1 h11");
    require(near(lane.points[2][0], 5.312276f) && near(lane.points[2][2], 8.454973f), "0x20b0c0 at 2/3");

    const TrafficSegmentClosest cross = trafficSegmentClosest({-1, 0, 0}, {1, 0, 0}, {0, 0, -1}, {0, 0, 1});
    require(cross.found && cross.onA == TrafficRouteVec3({0, 0, 0}) && cross.onB == TrafficRouteVec3({0, 0, 0}),
            "0x20af64: crossing segments meet at their closest points");
    require(!trafficSegmentClosest({-1, 0, 0}, {1, 0, 0}, {-1, 0, 1}, {1, 0, 1}).found, "0x20b010: parallel segments are refused");
    require(!trafficSegmentClosest({-1, 0, 0}, {1, 0, 0}, {3, 0, -1}, {3, 0, 1}).found, "0x20b034: a closest point past the end is refused");

    const assets::LevelPath ns = trafficJunctionLanePath(end(0, -5, 0, 1), end(0, 5, 0, 1), 1);
    const assets::LevelPath we = trafficJunctionLanePath(end(-5, 0, 1, 0), end(5, 0, 1, 0), 2);
    require(trafficLanesCross(ns, we), "0x1cad80: crossing lanes conflict");
    const assets::LevelPath beside = trafficJunctionLanePath(end(0.2f, -5, 0, 1), end(0.2f, 5, 0, 1), 4);
    require(!trafficLanesCross(ns, beside), "parallel lanes 0.2 apart do not conflict");
    const assets::LevelPath offset = trafficJunctionLanePath(end(4, -5, 0, 1), end(4, 5, 0, 1), 5);
    require(!trafficLanesCross(we, offset) && !trafficLanesCross(offset, we),
            "0x1cad58: only same-index segments are tested; segment 3 of we crosses segment 2 of offset and does not conflict");
    assets::LevelPath shortLane = offset;
    shortLane.points.resize(3);
    bool refused = false;
    try {
        trafficLanesCross(we, shortLane);
    } catch (const std::runtime_error&) {
        refused = true;
    }
    require(refused, "STAND-IN TRAFFIC-S13: b with fewer points than a is refused");
    shortLane.points.resize(1);
    require(!trafficLanesCross(we, shortLane), "0x1cad54: b with fewer than 2 points is never tested");
}

void fill() {
    const assets::LevelPath south = path("South", {{0, 0, -20}, {0, 0, -5}});
    const assets::LevelPath west = path("West", {{-20, 0, 0}, {-5, 0, 0}});
    const assets::LevelPath north = path("North", {{0, 0, 5}, {0, 0, 20}});
    const assets::LevelPath east = path("East", {{5, 0, 0}, {20, 0, 0}});
    TrafficJunctionSource source;
    source.position = {0, 0, 0};
    source.inputs = {&south, &west};
    source.outputs = {&north, &east};
    source.lights = {{TrafficLight{{south.nameHash}}, std::nullopt}, {TrafficLight{{west.nameHash}}, TrafficLight{{west.nameHash}}}};
    const TrafficJunctionFill result = fillTrafficJunction(source, 500);
    const TrafficJunctionRecord& record = result.record;
    require(record.layout.inputs == std::vector<std::uint32_t>({south.nameHash, west.nameHash}) &&
                record.layout.outputs == std::vector<std::uint32_t>({north.nameHash, east.nameHash}),
            "0x1ca620, 0x1ca69c: inputs at +0, outputs at +0x140 in slot order");
    require(record.choices == std::vector<std::vector<std::size_t>>({{0, 1}, {0, 1}}), "0x1caad4: the choice table +0x418 / +0x310");
    require(record.layout.lanes == std::vector<std::uint32_t>({500, 501, 502, 503}) && result.lanePaths.size() == 4 &&
                result.lanePaths[2].nameHash == 502,
            "0x1cab00: one lane per choice, input by input");
    require(record.laneEnds[1].input == 0 && record.laneEnds[1].output == 1 && record.laneEnds[2].input == 1 && record.laneEnds[2].output == 0,
            "0x1cab94: lane +0 = the input entry, +4 = the output entry");
    // Lanes: 0 south-north, 1 south-east, 2 west-north, 3 west-east. Pairs with a shared end are skipped (0x1cad18, 0x1cad2c).
    require(record.layout.conflicts == std::vector<std::array<std::uint8_t, 2>>({{0, 3}}),
            "0x1cae8c: only the straight crossing lanes conflict; the turns stay on their own corner");
    require(record.lightPairs.size() == 2 && record.passPair == 0 && record.lightPhase == kTrafficLightGreen &&
                record.lightTimer == kTrafficLightGreenTime,
            "0x1ca898..0x1ca8d8: count +0x304, +0x300 = 0, +0x308 = 2, +0x30c = *0x37459c (6)");
    require(record.queuedCars.empty() && record.layout.listedLanes.empty(), "the queue starts empty");
}

void lights() {
    TrafficJunctionRecord junction;
    junction.lightPairs = {{TrafficLight{{1}}, std::nullopt}, {TrafficLight{{2}}, std::nullopt}, {TrafficLight{{3}}, std::nullopt}};
    junction.lightPhase = kTrafficLightGreen;
    junction.lightTimer = kTrafficLightGreenTime;
    require(!runTrafficJunctionLights(junction, 5.5f) && near(junction.lightTimer, 0.5f), "0x1d8948: the timer counts down");
    std::optional<TrafficLightChange> change = runTrafficJunctionLights(junction, 1.0f);
    require(change && change->pair == 0 && change->phase == kTrafficLightYellow && change->previous == kTrafficLightGreen &&
                junction.lightTimer == kTrafficLightYellowTime,
            "0x1d8954, 0x1d898c: the timer stops at 0 and green turns yellow for 2 s");
    change = runTrafficJunctionLights(junction, 2.0f);
    require(change && change->phase == kTrafficLightRed && change->pair == 0 && junction.lightTimer == kTrafficLightRedTime,
            "0x1d8980: yellow turns red for 3 s");
    junction.queuedCars = {9};
    junction.layout.listedLanes = {0};
    change = runTrafficJunctionLights(junction, 3.0f);
    require(change && change->phase == kTrafficLightRed && change->previous == kTrafficLightRed && change->pair == 0 && junction.lightTimer == 0.0f,
            "0x1d89a4: red waits while a car is queued, setting red again");
    change = runTrafficJunctionLights(junction, 0.0f);
    require(change && change->phase == kTrafficLightRed && junction.passPair == 0, "and checks again at every update");
    junction.queuedCars.clear();
    junction.layout.listedLanes.clear();
    change = runTrafficJunctionLights(junction, 0.0f);
    require(change && change->pair == 1 && change->phase == kTrafficLightGreen && junction.passPair == 1 && junction.lightTimer == kTrafficLightGreenTime,
            "0x1d89b8: an empty queue lets the next pair turn green for 6 s");
    junction.passPair = 2;
    junction.lightPhase = kTrafficLightRed;
    junction.lightTimer = 0.0f;
    change = runTrafficJunctionLights(junction, 0.0f);
    require(change && change->pair == 0, "0x25c1b8: the next pair wraps by the pair count");
    junction.lightPhase = 7;
    junction.lightTimer = 0.0f;
    change = runTrafficJunctionLights(junction, 0.0f);
    require(change && change->phase == kTrafficLightRed && junction.lightTimer == 0.0f, "another phase byte becomes red with no time");

    TrafficJunctionRecord single;
    single.lightPairs = {{TrafficLight{{1}}, std::nullopt}};
    single.lightPhase = kTrafficLightGreen;
    single.lightTimer = kTrafficLightGreenTime;
    require(!runTrafficJunctionLights(single, 10.0f) && single.lightTimer == kTrafficLightGreenTime,
            "0x1d893c: with one pair the lights stay green and the timer stands");
}

struct ShippedJunctions {
    const char* blvl;
    std::size_t junctions;
    std::size_t lanes;
    std::size_t conflicts;
    std::size_t pairs;
    std::size_t lights;
    std::size_t sidedEnds;
};

int shipped(const std::string& fib, const std::string& csv) {
    if (!std::ifstream(fib) || !std::ifstream(csv)) {
        std::cout << "game data not extracted; skipping\n";
        return 77;
    }
    // Reference: 20261103-large-A, 2026-11-03, trafficJunctionFills over the shipped traffic levels (a throwaway probe printed the same
    // counts); the rules are those of the disassembly in docs/re/traffic-junction.md. The conflict counts are re-pinned by 20261103-large-A,
    // 2026-11-07, after the same-index segment test of 0x1cad58 (a throwaway probe over trafficJunctionFills printed the same counts).
    const ShippedJunctions levels[] = {
        {"levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl", 16, 97, 25, 27, 38, 48},
        {"levels/levels/lvl_03_auburn/lvl_03_auburn_main.blvl", 7, 64, 49, 12, 20, 24},
        {"levels/levels/lvl_04_fortmeadows/lvl_04_fortmeadows_main.blvl", 2, 0, 0, 0, 0, 0},
        {"levels/levels/lvl_05_bluebellwood/lvl_05_bluebellwood_main.blvl", 4, 4, 0, 0, 0, 0},
        {"levels/levels/lvl_06_apolloisland/lvl_06_apolloisland_main.blvl", 4, 4, 0, 0, 0, 0},
        {"levels/levels/lvl_07_downtown/lvl_07_downtown_main.blvl", 14, 63, 11, 32, 32, 0},
        {"levels/levels/lvl_test_playground_traffic.blvl", 4, 16, 2, 4, 7, 16},
    };
    assets::FuseArchive archive(fib);
    archive.loadNames(csv);
    for (const ShippedJunctions& want : levels) {
        const assets::FuseEntry* entry = archive.find(want.blvl);
        require(entry != nullptr, std::string(want.blvl) + ": not found");
        const assets::Level level = assets::parseLevel(archive.read(*entry));
        const std::vector<TrafficJunctionFill> fills = trafficJunctionFills({&level});
        std::size_t lanes = 0, conflicts = 0, pairs = 0, lights = 0, sided = 0;
        for (const TrafficJunctionFill& one : fills) {
            lanes += one.lanePaths.size();
            conflicts += one.record.layout.conflicts.size();
            pairs += one.record.lightPairs.size();
            for (const auto& pair : one.record.lightPairs) lights += std::size_t(pair[0].has_value()) + std::size_t(pair[1].has_value());
            for (const TrafficJunctionEnd& end : one.inputEnds) sided += end.side != 0;
            for (const TrafficJunctionEnd& end : one.outputEnds) sided += end.side != 0;
            require(one.lanePaths.size() == one.record.layout.lanes.size() && one.record.laneEnds.size() == one.record.layout.lanes.size(),
                    std::string(want.blvl) + ": one lane path and one end pair per lane");
        }
        require(fills.size() == want.junctions && lanes == want.lanes && conflicts == want.conflicts && pairs == want.pairs && lights == want.lights &&
                    sided == want.sidedEnds,
                std::string(want.blvl) + ": junctions " + std::to_string(fills.size()) + " lanes " + std::to_string(lanes) + " conflicts " +
                    std::to_string(conflicts) + " pairs " + std::to_string(pairs) + " lights " + std::to_string(lights) + " sided " + std::to_string(sided));
    }

    // lvl_01 `Junction1` (second junction object): a T junction with inputs RoadPath02, RoadPath07, RoadPath57, outputs RoadPath01,
    // RoadPath08, RoadPath51 and lights Trafficlight01 (Path01 RoadPath02) and Trafficlight02 (Path01 RoadPath57, an empty Path02) with its
    // SlaveLight Trafficlight03 (Path01 RoadPath07). Same probe, 20261103-large-A.
    const assets::Level level = assets::parseLevel(archive.read(*archive.find(levels[0].blvl)));
    const TrafficJunctionFill one = trafficJunctionFills({&level}).at(1);
    const TrafficJunctionRecord& record = one.record;
    require(record.layout.inputs == std::vector<std::uint32_t>({assets::nameHash("RoadPath02"), assets::nameHash("RoadPath07"), assets::nameHash("RoadPath57")}) &&
                record.layout.outputs == std::vector<std::uint32_t>({assets::nameHash("RoadPath01"), assets::nameHash("RoadPath08"), assets::nameHash("RoadPath51")}),
            "lvl_01 Junction1: inputs and outputs");
    require(record.choices == std::vector<std::vector<std::size_t>>({{1, 2}, {0, 2}, {0, 1}}), "lvl_01 Junction1: each input leaves by the two other roads");
    require(record.layout.conflicts == std::vector<std::array<std::uint8_t, 2>>({{1, 2}}), "lvl_01 Junction1: conflict pairs (same probe, 2026-11-07)");
    const assets::LevelPath& lane = one.lanePaths.at(1);
    require(near(lane.points[0][0], 514.17419f) && near(lane.points[0][2], 795.94940f) && near(lane.points[1][0], 509.00198f, 2e-3f) &&
                near(lane.points[1][1], 26.68993f, 2e-3f) && near(lane.points[1][2], 794.09082f, 2e-3f) && near(lane.points[2][0], 504.85501f, 2e-3f) &&
                near(lane.points[2][2], 789.92853f, 2e-3f) && near(lane.points[3][0], 503.23480f) && near(lane.points[3][2], 784.77570f),
            "lvl_01 Junction1: lane 1 from RoadPath02 to RoadPath51");
    require(record.lightPairs.size() == 2 && record.lightPairs[0][0]->routes == std::vector<std::uint32_t>({assets::nameHash("RoadPath02")}) &&
                !record.lightPairs[0][1] && record.lightPairs[1][0]->routes == std::vector<std::uint32_t>({assets::nameHash("RoadPath57")}) &&
                record.lightPairs[1][1]->routes == std::vector<std::uint32_t>({assets::nameHash("RoadPath07")}),
            "lvl_01 Junction1: light pairs with the lights' Path01..04 routes, an empty name skipped");
    std::cout << "traffic-junction-fill: shipped levels ok\n";
    return 0;
}

}

int main(int argc, char** argv) {
    try {
        ends();
        sides();
        choices();
        lanes();
        fill();
        lights();
        if (argc == 3) return shipped(argv[1], argv[2]);
        std::cout << "traffic-junction-fill: ok\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "traffic-junction-fill-test: FAIL: " << error.what() << '\n';
        return 1;
    }
}
