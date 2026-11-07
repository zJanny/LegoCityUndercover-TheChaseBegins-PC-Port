// Tests of src/runtime/traffic_ground_world.*: the ground ray cast of the traffic probe 0x20ae2c (0x22b108 -> 0x266834 -> 0x22f978 -> 0x293188).
// Expected values of the synthetic cases follow from the disassembly (llvm-objdump --triple=armv6k --mattr=+vfp2 of artifacts/game/game.arm.elf,
// read by 20261107-traffic-D on 2026-11-07, docs/re/traffic-ground-cast.md), evaluated by hand on triangles whose weights are exact in
// single precision. The real-data case (FIB and names as arguments, label needs-game-data) casts the probe at points of the shipped
// traffic paths of lvl_01 with the port's own collision world; no emulator value is involved.
#include "assets/assets.hpp"
#include "runtime/ai_driver_input.hpp"
#include "runtime/traffic_ground_world.hpp"

#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

namespace {

using namespace chase::runtime;
namespace assets = chase::assets;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

assets::CollisionItem floor(bool facingUp, std::uint8_t a, std::uint8_t b, std::uint8_t c) {
    assets::CollisionItem item;
    item.vertices = {0, 0, 0, 0, 0, 8, 8, 0, 0};
    assets::CollisionTriangle triangle;
    triangle.normal[1] = facingUp ? 1.0f : -1.0f;
    triangle.vertex[0] = 0;
    triangle.vertex[1] = facingUp ? 1 : 2;
    triangle.vertex[2] = facingUp ? 2 : 1;
    triangle.unknown[0] = a;
    triangle.unknown[1] = b;
    triangle.unknown[2] = c;
    item.triangles.push_back(triangle);
    assets::CollisionNode node;
    node.centre[0] = 4;
    node.centre[2] = 4;
    node.half[0] = 4;
    node.half[1] = 1;
    node.half[2] = 4;
    node.info = 1u | (1u << 1);
    item.nodes.push_back(node);
    return item;
}

AiDriverRecord driverAt(float x, float y, float z) {
    AiDriverRecord driver;
    driver.target = {x, y, z};
    return driver;
}

std::optional<std::uint32_t> probe(const CollisionWorld& world, const AiVec3& target) {
    std::optional<std::uint32_t> sent;
    const TrafficGroundWorld ground = trafficGroundWorld(world, [&](std::uint32_t colour) { sent = colour; });
    snapTrafficTarget(driverAt(target[0], target[1], target[2]), ground);
    return sent;
}

void syntheticCases() {
    CollisionWorld world;
    world.add(floor(true, 100, 200, 50));
    const TrafficGroundSegment segment = trafficGroundSegment({2, 0, 2});
    const auto hit = castTrafficGround(world, segment);
    require(hit.has_value(), "a vertical ray over a floor triangle hits it");
    require(hit->point[0] == 2 && hit->point[1] == 0 && hit->point[2] == 2, "the hit point is the start moved down by t (t = 1.5)");
    require(hit->triangle.colourIndex[0] == 100 && hit->triangle.colourIndex[1] == 200 && hit->triangle.colourIndex[2] == 50, "triangle bytes +0x1d..+0x1f");
    require(!hit->mesh.hasColours, "the shipped items have a zero flag word (item +8)");

    const std::optional<std::uint32_t> colour = probe(world, {2, 0, 2});
    require(colour.has_value(), "the probe sends the colour of the hit");
    require(*colour >> 24 == 255, "alpha 255");
    const std::uint32_t grey = *colour & 0xffu;
    require(grey == 112 && *colour == 0xff707070u, "weights 0.5 / 0.25 / 0.25 of the corners: 100 x 0.5 + 200 x 0.25 + 50 x 0.25 = 112.5, truncated");
}

void colourMessage() {
    TrafficNodeTint node;
    applyTrafficColourMessage(node, 0xff112233u);
    require(node.bytes[0] == 0x33 && node.bytes[1] == 0x22 && node.bytes[2] == 0x11, "bytes +0x118 / +0x119 / +0x11a take the low, middle and high byte (0x106a34..0x106a40)");
}

void missCases() {
    CollisionWorld world;
    world.add(floor(true, 10, 20, 30));
    require(!castTrafficGround(world, trafficGroundSegment({7, 0, 7})), "a point outside the triangle misses");
    require(!castTrafficGround(world, trafficGroundSegment({2, 2, 2})), "a floor 2 m below the target is out of reach (1.5)");
    require(!castTrafficGround(world, trafficGroundSegment({2, -2, 2})), "a floor above the segment is out of reach");
    require(castTrafficGround(world, trafficGroundSegment({2, 1.4f, 2})).has_value(), "a floor 1.4 below the target is in reach");

    CollisionWorld back;
    back.add(floor(false, 10, 20, 30));
    require(!castTrafficGround(back, trafficGroundSegment({2, 0, 2})), "a triangle seen from behind is missed (determinant below 1e-5)");

    CollisionWorld masked;
    masked.add(floor(true, 10, 20, 30), 4);
    require(!castTrafficGround(masked, trafficGroundSegment({2, 0, 2})), "an entry with mask 4 (prop) is not accepted by include 1");

    bool threw = false;
    try {
        castTrafficGround(world, TrafficGroundSegment{{0, 1, 0}, {1, 0, 0}});
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    require(threw, "a segment that is not vertical is refused");
}

void firstEntryEnds() {
    CollisionWorld world;
    assets::CollisionItem lower = floor(true, 1, 2, 3);
    for (std::size_t i = 1; i < lower.vertices.size(); i += 3) lower.vertices[i] = -1.0f;
    world.add(lower);
    world.add(floor(true, 9, 9, 9));
    const auto hit = castTrafficGround(world, trafficGroundSegment({2, 0, 2}));
    require(hit && hit->point[1] == -1 && hit->triangle.colourIndex[0] == 1, "the first accepted entry that is hit ends the cast, though a later one is nearer");

    CollisionWorld boxFirst;
    assets::CollisionItem boxItem = floor(true, 0, 0, 0);
    boxFirst.add(boxItem, 17);
    CollisionWorld::Entry& box = boxFirst.mutableEntries().back();
    box.box = true;
    box.matrix = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    box.centre[0] = 2;
    box.centre[1] = 0.5f;
    box.centre[2] = 2;
    box.half[0] = 1;
    box.half[1] = 0.25f;
    box.half[2] = 1;
    boxFirst.add(floor(true, 9, 9, 9));
    require(!castTrafficGround(boxFirst, trafficGroundSegment({2, 0, 2})), "a vehicle box (mask 17) that the segment meets ends the cast without a triangle");
    require(castTrafficGround(boxFirst, trafficGroundSegment({6, 0, 1})).has_value(), "a segment beside the box reaches the next entry");
}

void realData(const char* fib, const char* names) {
    assets::FuseArchive archive(fib);
    archive.loadNames(names);
    CollisionWorld world;
    for (const auto& item : assets::parseModel(archive.read(*archive.find("models/lvl_01_cherrytreehills/lvl_01_cherrytreehills.bfnmdl"))).collision) world.add(item);
    const assets::Level level = assets::parseLevel(archive.read(*archive.find("levels/levels/lvl_01_cherrytreehills/lvl_01_cherrytreehills_main.blvl")));
    std::size_t points = 0, hits = 0, coloured = 0;
    for (const assets::LevelPath& path : level.paths) {
        if (path.points.empty()) continue;
        for (const auto& point : path.points) {
            ++points;
            const auto hit = castTrafficGround(world, trafficGroundSegment({point[0], point[1], point[2]}));
            if (!hit) continue;
            ++hits;
            if (trafficGroundColour(hit->mesh, hit->triangle, hit->point)) ++coloured;
        }
    }
    std::cout << "lvl_01 path points " << points << " hits " << hits << " coloured " << coloured << "\n";
    require(points == 4383 && hits == 3884 && coloured == 3884, "path points, hits and coloured hits of lvl_01 (port output of 20261107-traffic-D, a regression pin, not an original value)");
}

}

int main(int argc, char** argv) {
    try {
        syntheticCases();
        colourMessage();
        missCases();
        firstEntryEnds();
        if (argc == 3) {
            if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
                std::cout << "traffic-ground-world-test: data not available, skipped\n";
                return 77;
            }
            realData(argv[1], argv[2]);
        }
    } catch (const std::exception& error) {
        std::cerr << "traffic-ground-world-test: " << error.what() << "\n";
        return 1;
    }
    return 0;
}
