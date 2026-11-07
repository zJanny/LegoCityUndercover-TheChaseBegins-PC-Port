// Tests of src/runtime/traffic_light_parts.*: the part changes of 0x20b19c, the junction set-up 0x1ca5a4 with its slot index quirk and the
// light cycle 0x1d8924. Expectations follow from the disassembly read by 20261107-traffic-B, 2026-11-07 (docs/re/traffic-junction.md).
// With <fib> <csv> it also checks the shipped levels (traffic-light-parts-real-data).
#include "runtime/traffic_light_parts.hpp"

#include "assets/assets.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using namespace chase;
using namespace chase::runtime;
using Pairs = std::vector<std::array<std::optional<TrafficLight>, 2>>;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
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

Pairs pairsOf(std::initializer_list<std::pair<bool, bool>> list) {
    Pairs pairs;
    for (const auto& item : list) {
        std::array<std::optional<TrafficLight>, 2> pair;
        if (item.first) pair[0] = TrafficLight{};
        if (item.second) pair[1] = TrafficLight{};
        pairs.push_back(pair);
    }
    return pairs;
}

std::string look(const TrafficJunctionLightParts& parts, std::size_t pair, std::size_t member) {
    std::string text;
    for (std::uint8_t part = 0; part < kTrafficLightPartCount; ++part)
        if (parts.shown(pair, member, part)) text += std::string(text.empty() ? "" : ",") + kTrafficLightPartNames[part];
    return text.empty() ? "none" : text;
}

void phase() {
    const Pairs pairs = pairsOf({{true, true}, {true, false}});
    const std::vector<TrafficLightPartOp> ops = trafficLightPhaseOps(pairs, 1, kTrafficLightYellow, kTrafficLightGreen);
    require(ops.size() == 2 && ops[0].pair == 1 && ops[0].member == 0 && ops[0].part == kTrafficLightGreen && !ops[0].show &&
                ops[1].part == kTrafficLightYellow && ops[1].show,
            "0x20b19c: hide the part of the old phase, show the part of the new one, only for the lights that exist");
    require(trafficLightPhaseOps(pairs, 0, kTrafficLightGreen, kTrafficLightRed).size() == 4, "0x1d89c8: both lights of the pair");
    require(trafficLightPhaseOps(pairs, 5, kTrafficLightGreen, kTrafficLightRed).empty(), "a pair outside the list changes nothing");
    require(throws([&] { trafficLightPhaseOps(pairs, 0, 3, 0); }), "a phase outside 0..2 is refused (the original indexes part -1)");

    TrafficJunctionLightParts parts(2);
    require(look(parts, 0, 0) == "none" && look(parts, 1, 1) == "none", "0x1afae8: a new light hides red, yellow and green");
    parts.apply(trafficLightPhaseOps(pairs, 0, kTrafficLightRed, kTrafficLightRed));
    require(look(parts, 0, 0) == "red" && look(parts, 0, 1) == "red", "red hidden, then shown: red");
    parts.apply(trafficLightPhaseOps(pairs, 0, kTrafficLightGreen, kTrafficLightRed));
    require(look(parts, 0, 0) == "green", "red turns green");
    parts.apply(trafficLightPhaseOps(pairs, 0, kTrafficLightYellow, kTrafficLightGreen));
    require(look(parts, 0, 0) == "yellow", "green turns yellow");
    parts.apply(trafficLightPhaseOps(pairs, 0, kTrafficLightRed, kTrafficLightYellow));
    require(look(parts, 0, 0) == "red", "yellow turns red");
    parts.apply(trafficLightPhaseOps(pairs, 0, kTrafficLightRed, kTrafficLightRed));
    require(look(parts, 0, 0) == "red", "a red wait with cars queued (0x1d8924 calls 0x20b19c with equal phases) keeps red");
}

void setup() {
    {
        TrafficJunctionLightParts parts(1);
        parts.apply(trafficLightSetupOps(pairsOf({{true, false}}), {0}));
        require(look(parts, 0, 0) == "green", "one light: red at its slot, then pair 0 turns green");
    }
    {
        const Pairs pairs = pairsOf({{true, false}, {true, true}});
        TrafficJunctionLightParts parts(2);
        parts.apply(trafficLightSetupOps(pairs, {0, 1}));
        require(look(parts, 0, 0) == "green", "pair 0 starts green");
        require(look(parts, 1, 0) == "none", "slot index quirk: pair 1 (slot 1) reaches word 3, so its own light stays hidden");
        require(look(parts, 1, 1) == "red", "slot index quirk: the slave of pair 1 is the light at word 3 and shows red");
    }
    {
        const Pairs pairs = pairsOf({{true, true}, {true, true}});
        TrafficJunctionLightParts parts(2);
        parts.apply(trafficLightSetupOps(pairs, {0, 2}));
        require(look(parts, 0, 0) == "green" && look(parts, 0, 1) == "green", "both lights of pair 0 turn green");
        require(look(parts, 1, 0) == "none" && look(parts, 1, 1) == "none", "pair 1 at slot 2 reaches word 4, not written yet: skipped");
    }
    {
        const Pairs pairs = pairsOf({{true, true}});
        TrafficJunctionLightParts parts(1);
        parts.apply(trafficLightSetupOps(pairs, {1}));
        require(look(parts, 0, 0) == "green" && look(parts, 0, 1) == "green", "a pair at slot 1 reaches its slave first, then both turn green");
    }
    {
        const Pairs pairs = pairsOf({{true, false}, {true, false}, {true, true}});
        TrafficJunctionLightParts parts(3);
        parts.apply(trafficLightSetupOps(pairs, {0, 1, 2}));
        require(look(parts, 1, 0) == "none" && look(parts, 2, 0) == "none" && look(parts, 2, 1) == "none",
                "slots 0, 1, 2: word 0, word 3 (slave of pair 1, absent) and word 6 (not written)");
    }
    {
        const std::vector<TrafficLightPartOp> ops = trafficLightSetupOps(pairsOf({{true, false}}), {0});
        require(ops.size() == 6 && !ops[0].show && ops[0].part == kTrafficLightRed && ops[1].show && ops[1].part == kTrafficLightRed &&
                    ops[3].show && ops[4].part == kTrafficLightRed && !ops[4].show && ops[5].part == kTrafficLightGreen && ops[5].show,
                "the red hide and show run twice (0x1ca860 loop), then hide red and show green (0x1ca89c)");
    }
    require(trafficLightSetupOps({}, {}).empty(), "no pair: nothing");
    require(throws([] { trafficLightSetupOps(pairsOf({{true, false}}), {}); }), "slots and pairs of different size are refused");
    require(throws([] { trafficLightSetupOps(pairsOf({{true, false}}), {8}); }), "slot 8 does not exist");
}

void names() {
    const std::vector<std::string> nodes = {"base", "Red", "YELLOW", "green", "green"};
    require(trafficLightPartNode(nodes, kTrafficLightRed) == 1 && trafficLightPartNode(nodes, kTrafficLightYellow) == 2 &&
                trafficLightPartNode(nodes, kTrafficLightGreen) == 3,
            "0x25ad90 compares names without case; the first match is taken");
    require(trafficLightPartNode({"base"}, kTrafficLightRed) == -1 && trafficLightPartNode(nodes, 3) == -1, "no such node: -1");
}

int shipped(const std::string& fib, const std::string& csv) {
    if (!std::ifstream(fib) || !std::ifstream(csv)) {
        std::cout << "game data not extracted; skipping\n";
        return 77;
    }
    assets::FuseArchive archive(fib);
    archive.loadNames(csv);
    const assets::ClassLookup classOf = assets::archiveClassLookup(archive);
    // Reference: 20261107-traffic-B, 2026-11-07, trafficJunctionFills over lvl_01 and the node names of the model of props/prop_trafficlight
    // (a throwaway probe printed the same values).
    const assets::Level level = assets::parseLevel(archive.read(*archive.find("levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl")));
    const std::vector<TrafficJunctionFill> fills = trafficJunctionFills({&level});
    const TrafficJunctionFill& one = fills.at(1);
    require(one.lightSlots == std::vector<std::size_t>({0, 1}) && one.lightInstances.size() == 2 && one.lightInstances[0][1].empty() &&
                !one.lightInstances[1][1].empty(),
            "lvl_01 Junction1: light slots 0 and 1, the second pair with a slave");
    std::size_t pairCount = 0;
    for (const TrafficJunctionFill& fill : fills) {
        require(fill.lightSlots.size() == fill.record.lightPairs.size() && fill.lightInstances.size() == fill.record.lightPairs.size(),
                "one slot and one instance pair per light pair");
        for (std::size_t n = 1; n < fill.lightSlots.size(); ++n) require(fill.lightSlots[n] > fill.lightSlots[n - 1], "slots grow");
        TrafficJunctionLightParts parts(fill.record.lightPairs.size());
        parts.apply(trafficLightSetupOps(fill.record.lightPairs, fill.lightSlots));
        pairCount += fill.record.lightPairs.size();
    }
    require(pairCount == 27, "lvl_01: 27 light pairs");
    std::string model;
    for (const assets::Placement& placement : assets::objectPlacements(level, classOf))
        if (placement.className == "props/prop_trafficlight") {
            model = assets::meshModelPath(placement.mesh);
            break;
        }
    require(!model.empty() && archive.find(model) != nullptr, "a props/prop_trafficlight placement with a model");
    const assets::Model parsed = assets::parseModel(archive.read(*archive.find(model)));
    for (std::uint8_t part = 0; part < kTrafficLightPartCount; ++part) {
        const std::ptrdiff_t node = trafficLightPartNode(parsed.nodeNames, part);
        require(node >= 0 && parsed.nodeMesh[std::size_t(node)] >= 0, std::string("the model has a drawn node ") + kTrafficLightPartNames[part]);
    }
    std::cout << "traffic-light-parts: shipped levels ok\n";
    return 0;
}

}

int main(int argc, char** argv) {
    try {
        phase();
        setup();
        names();
        if (argc == 3) return shipped(argv[1], argv[2]);
        std::cout << "traffic-light-parts: ok\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "traffic-light-parts-test: FAIL: " << error.what() << '\n';
        return 1;
    }
}
