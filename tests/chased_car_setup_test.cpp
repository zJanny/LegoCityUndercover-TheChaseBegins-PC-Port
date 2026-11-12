// The path length table and the node test of VehicleSetChased (src/runtime/chased_car_setup.hpp). The rules are transcribed from 0x1afd38,
// 0x22a6c0 and 0x1afe04..0x1afe30 (llvm-objdump of game.arm.elf, 20261112-enemy-D, 2026-11-12, docs/re/traffic-driver-states.md); the paths are made up.
#include "runtime/chased_car_setup.hpp"
#include "runtime/vehicle_ai_natives.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

assets::LevelPath straightPath(unsigned points, bool closed) {
    assets::LevelPath path;
    path.type = 0;
    path.closed = closed;
    for (unsigned i = 0; i < points; ++i) path.points.push_back({10.0f * float(i), 0.0f, 0.0f});
    return path;
}

void run() {
    require(kChasedPathLengthSteps == 10, "VehicleSetChased passes 10 steps to 0x22a6c0 (mov r1, #10 at 0x1afd84)");

    const ChasedPathTable none = chasedPathTable(assets::LevelPath{});
    require(none.lengths.empty() && none.total == 0.0f, "a path without points has no table and the total 0.0");

    const ChasedPathTable open = chasedPathTable(straightPath(5, false));
    require(open.lengths.size() == 5 && open.lengths.front() == 0.0f, "an open path has one entry per point, entry 0 is 0.0");
    require(open.total == open.lengths.back(), "the total at path +0x18 is the last entry");
    require(std::fabs((open.lengths[2] - open.lengths[1]) - 9.0f) < 1e-3f, "the chord sum of an inner segment of points 10 apart takes 9 of its 10 steps (0x293540 sums the chords up to t = 0.9; run of the ported sum, not measured in the original)");
    for (std::size_t i = 1; i < open.lengths.size(); ++i) require(open.lengths[i] > open.lengths[i - 1], "the entries grow along the path");

    const ChasedPathTable closed = chasedPathTable(straightPath(5, true));
    require(closed.lengths.size() == 6, "a closed path has count + 1 entries");

    ChasedNodeBytes node;
    node.alpha = 7;
    node.flags = 0xff;
    chasedNodeTest(false, node);
    require(node.alpha == 7 && node.flags == 0xff, "a node of another type is left alone");
    chasedNodeTest(true, node);
    require(node.alpha == 255 && node.flags == 0xcf, "a mesh node gets alpha 255 and loses the distance fade bits 4 and 5");

    VehicleAiNatives natives;
    const assets::LevelPath route = straightPath(5, false);
    natives.setPathSource([&route](std::uint32_t hash) { return hash == 77 ? &route : nullptr; });
    const AiMatrix origin{{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {0, 0, 0}}};
    natives.setChased("car", origin, 99, 29.0f);
    require(natives.pathTable(99) == nullptr && natives.pathTable(77) == nullptr, "a route that is no level path builds no table");
    natives.setChased("car", origin, 77, 29.0f);
    const ChasedPathTable* table = natives.pathTable(77);
    require(table && table->lengths.size() == 5 && table->total == chasedPathTable(route).total, "VehicleSetChased builds the table of the path with 10 steps");
    const VehicleAiRecord* record = natives.find("car");
    require(record && record->node.alpha == 255 && (record->node.flags & kNodeDistanceFadeBits) == 0, "VehicleSetChased runs the node test (STAND-IN SCRIPT-S344: mesh node assumed)");
}

}

int main() {
    try {
        run();
    } catch (const std::exception& error) {
        std::cerr << "chased-car-setup: " << error.what() << '\n';
        return 1;
    }
    std::cout << "chased-car-setup: ok\n";
    return 0;
}
