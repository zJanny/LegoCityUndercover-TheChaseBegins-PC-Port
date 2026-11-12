// Tests of src/runtime/traffic_push_shapes.*: the world slide 0x23d434 (0x208d84 / 0x209a98 / 0x2092d8 and the corner pass), the mesh test
// 0x287ac4 and the oriented test 0x27f174 behind TrafficPushShapes. Every expectation follows from the disassembly (llvm-objdump
// --triple=armv6k --mattr=+vfp2 of artifacts/game/game.arm.elf, read by 20261112-trafficfeed-C on 2026-11-12, docs/re/traffic-car-push.md):
// the filter flags 88 / 92 / 93 / 80 (| 0x20), the modes 17 / 3 / 1, the literals 0x2096c4 (1e-4), 0x2092d4 (-1.0), 0x27f650 (0.9) and
// 0x27f654 (0.007). Walls, boxes and moves are made up; the expected floats repeat the instruction order. No emulator was used.
#include "runtime/traffic_push_shapes.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

namespace {

using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool same(float a, float b) {
    std::uint32_t x = 0, y = 0;
    std::memcpy(&x, &a, sizeof x);
    std::memcpy(&y, &b, sizeof y);
    return x == y || (a == 0.0f && b == 0.0f);
}

void requireMove(const AiVec3& move, const AiVec3& expected, const std::string& message) {
    for (int a = 0; a < 3; ++a)
        require(same(move[a], expected[a]), message + ": axis " + std::to_string(a) + " is " + std::to_string(move[a]) + ", expected " +
                                                std::to_string(expected[a]));
}

using Vertex = std::array<float, 3>;

chase::assets::CollisionItem triangleItem(const std::vector<std::array<Vertex, 3>>& triangles, const std::vector<Vertex>& normals) {
    chase::assets::CollisionItem item;
    for (std::size_t t = 0; t < triangles.size(); ++t) {
        chase::assets::CollisionTriangle record;
        for (int k = 0; k < 3; ++k) {
            for (int a = 0; a < 3; ++a) item.vertices.push_back(triangles[t][std::size_t(k)][std::size_t(a)]);
            record.vertex[k] = std::uint32_t(t * 3 + std::size_t(k));
        }
        for (int a = 0; a < 3; ++a) record.normal[a] = normals[t][std::size_t(a)];
        record.d = record.normal[0] * triangles[t][0][0];
        record.d += record.normal[1] * triangles[t][0][1];
        record.d += record.normal[2] * triangles[t][0][2];
        item.triangles.push_back(record);
    }
    chase::assets::CollisionNode root;
    float low[3] = {1e9f, 1e9f, 1e9f}, high[3] = {-1e9f, -1e9f, -1e9f};
    for (std::size_t v = 0; v < item.vertices.size(); ++v) {
        low[v % 3] = std::min(low[v % 3], item.vertices[v]);
        high[v % 3] = std::max(high[v % 3], item.vertices[v]);
    }
    for (int a = 0; a < 3; ++a) {
        root.centre[a] = (low[a] + high[a]) * 0.5f;
        root.half[a] = (high[a] - low[a]) * 0.5f;
    }
    root.info = (0u << 5) | (std::uint32_t(triangles.size()) << 1) | 1u;
    item.nodes.push_back(root);
    return item;
}

// A large triangle in the plane through `point` with the unit normal `normal`; `u` and `w` are unit vectors in the plane.
std::array<Vertex, 3> planeTriangle(const Vertex& point, const Vertex& u, const Vertex& w) {
    std::array<Vertex, 3> t{};
    for (int a = 0; a < 3; ++a) {
        t[0][std::size_t(a)] = point[std::size_t(a)] - 10.0f * u[std::size_t(a)] - 10.0f * w[std::size_t(a)];
        t[1][std::size_t(a)] = point[std::size_t(a)] + 10.0f * u[std::size_t(a)] - 10.0f * w[std::size_t(a)];
        t[2][std::size_t(a)] = point[std::size_t(a)] + 20.0f * w[std::size_t(a)];
    }
    return t;
}

const Vertex kUp = {0.0f, 1.0f, 0.0f};
const Vertex kWallMinusX = {-1.0f, 0.0f, 0.0f};

CollisionWorld wallWorld(const Vertex& point, const Vertex& normal, const Vertex& u, const Vertex& w) {
    CollisionWorld world;
    world.add(triangleItem({planeTriangle(point, u, w)}, {normal}));
    return world;
}

WorldBoxSlide pushedSlide() {
    WorldBoxSlide slide;
    slide.centre = {0.0f, 1.0f, 0.0f};
    slide.half = {0.5f, 0.5f, 0.5f};
    slide.filter = kPushSlidePushedFlags;
    slide.mode = kPushSlideMode;
    return slide;
}

void testWallSlide() {
    const CollisionWorld world = wallWorld({1.0f, 1.0f, 0.0f}, kWallMinusX, kUp, {0.0f, 0.0f, 1.0f});
    AiVec3 move = {1.0f, 0.0f, 1.0f};
    const std::uint8_t result = slideWorldBox(world, pushedSlide(), move);
    requireMove(move, {0.0f, 0.0f, 1.0f}, "0x22f928 removes the part of the move into the wall");
    require(result == 1, "one candidate after the corner pass found none: the first list's count");

    WorldBoxSlide near = pushedSlide();
    near.centre = {0.9f, 1.0f, 0.0f};
    AiVec3 away = {-0.2f, 0.0f, 0.0f};
    slideWorldBox(world, near, away);
    requireMove(away, {-0.2f, 0.0f, 0.0f}, "flag 8 drops a wall the move leaves");
}

void testSlopeFlags() {
    const Vertex normal = {-0.6f, 0.8f, 0.0f};
    const CollisionWorld world = wallWorld({1.0f, 1.0f, 0.0f}, normal, {0.8f, 0.6f, 0.0f}, {0.0f, 0.0f, 1.0f});
    WorldBoxSlide slide = pushedSlide();
    slide.half = {0.25f, 0.25f, 0.25f};
    AiVec3 pushed = {1.0f, 0.0f, 0.0f};
    slideWorldBox(world, slide, pushed);
    float along = normal[0] * 1.0f;
    along += normal[1] * 0.0f;
    along += normal[2] * 0.0f;
    requireMove(pushed, {1.0f - normal[0] * along, 0.0f - normal[1] * along, 0.0f},
                "mode 17 has no flatten: the slide along a slope (normal y 0.8) changes y too, and flag 88 keeps the slope");

    slide.filter = kPushSlideMoverFlags;
    slide.normalLimit = kFloorNormalMin;
    AiVec3 mover = {1.0f, 0.0f, 0.0f};
    slideWorldBox(world, slide, mover);
    requireMove(mover, {1.0f, 0.0f, 0.0f}, "flag 92 (with 4) drops a triangle whose normal y is above *0x376910");
}

void testCornerPass() {
    const Vertex wallB = {0.6f, 0.0f, -0.8f};
    CollisionWorld world;
    world.add(triangleItem({planeTriangle({0.5f, 1.0f, 1.4f}, kUp, {0.8f, 0.0f, 0.6f})}, {wallB}));
    world.add(triangleItem({planeTriangle({1.0f, 1.0f, 0.0f}, kUp, {0.0f, 0.0f, 1.0f})}, {kWallMinusX}));
    AiVec3 move = {1.0f, 0.0f, 1.0f};
    slideWorldBox(world, pushedSlide(), move);
    requireMove(move, {0.0f, 0.0f, 0.0f}, "mode 17 bit 0x10: the second pass hits wall B again, which meets wall A at a cosine in (-1, 0)");

    WorldBoxSlide single = pushedSlide();
    single.mode = kWorldSlide;
    AiVec3 once = {1.0f, 0.0f, 1.0f};
    slideWorldBox(world, single, once);
    require(once[0] == 0.0f && once[2] > 0.0f, "without bit 0x10 the move keeps its slide along both walls");
}

chase::assets::ObjectCollision placed(const std::shared_ptr<const chase::assets::CollisionItem>& mesh, float x) {
    chase::assets::ObjectCollision object;
    object.instance = "wall";
    object.mesh = mesh;
    object.matrix = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, x, 0, 0, 1};
    return object;
}

void testDuplicateRecords() {
    const auto mesh = std::make_shared<const chase::assets::CollisionItem>(
        triangleItem({planeTriangle({0.0f, 1.0f, 0.0f}, kUp, {0.0f, 0.0f, 1.0f})}, {kWallMinusX}));
    WorldBoxSlide slide = pushedSlide();
    slide.mode = kWorldSlide;

    CollisionWorld shared;
    shared.addObject(placed(mesh, 1.0f));
    shared.addObject(placed(mesh, 1.0f));
    AiVec3 move = {1.0f, 0.0f, 0.0f};
    require(slideWorldBox(shared, slide, move) == 1, "flag 0x40: the second entry of one item repeats its records and is dropped");

    const auto copy = std::make_shared<const chase::assets::CollisionItem>(*mesh);
    CollisionWorld distinct;
    distinct.addObject(placed(mesh, 1.0f));
    distinct.addObject(placed(copy, 1.0f));
    AiVec3 again = {1.0f, 0.0f, 0.0f};
    require(slideWorldBox(distinct, slide, again) == 2, "records of two items differ in their vertex pointers and both stay");
}

void testBoxEntry() {
    chase::assets::ObjectCollision object;
    object.instance = "crate";
    object.matrix = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1.5f, 1.0f, 0.0f, 1};
    object.half[0] = object.half[1] = object.half[2] = 0.5f;
    CollisionWorld world;
    world.addObject(object);
    WorldBoxSlide slide = pushedSlide();
    slide.mode = kWorldSlide;
    slide.filter = kPushSlidePushedFlags | kWorldFilterNormalY;
    slide.normalLimit = -2.0f;
    AiVec3 move = {1.0f, 0.0f, 0.25f};
    require(slideWorldBox(world, slide, move) == 1, "a box entry counts once");
    requireMove(move, {0.0f, 0.0f, 0.25f}, "a box entry skips the filter (a null triangle) and slides on its -x face only");
}

void testMeshPush() {
    const Vertex normal = {-0.8f, 0.6f, 0.0f};
    CollisionWorld world = wallWorld({1.0f, 1.0f, 0.0f}, normal, {0.6f, 0.8f, 0.0f}, {0.0f, 0.0f, 1.0f});
    const CollisionWorld::Entry& wall = world.entries().front();
    const TrafficWorldBox box = {{0.0f, 1.0f, 0.0f}, {0.25f, 0.25f, 0.25f}};
    AiVec3 move = {1.0f, 0.0f, 0.5f};
    require(meshEntryPush(wall, box, &move, kMeshPushFilter, kMeshPushMode, kFloorNormalMin), "the moved box overlaps the wall");
    require(std::fabs(move[0]) < 1e-6f && move[1] == 0.0f && move[2] == 0.5f, "mode 3 flattens the normal to (-1, y, 0) and keeps the move's y");

    AiVec3 still{};
    require(!meshEntryPush(wall, box, &still, kMeshPushFilter, kMeshPushMode, kFloorNormalMin), "the box at rest does not reach the wall");

    CollisionWorld floorWorld = wallWorld({0.0f, 0.8f, 0.0f}, kUp, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f});
    const CollisionWorld::Entry& floor = floorWorld.entries().front();
    AiVec3 down = {0.0f, -0.25f, 0.0f};
    require(!meshEntryPush(floor, box, &down, kMeshPushFilter, kMeshPushMode, kFloorNormalMin), "flag 4 of 93 drops a floor");
    require(meshEntryPush(floor, box, nullptr, 80, 0, kFloorNormalMin), "the pure turn call (80, mode 0) has no normal filter");
}

AiMatrix identityAt(float x, float y, float z) {
    AiMatrix m{};
    m[0] = {1.0f, 0.0f, 0.0f};
    m[1] = {0.0f, 1.0f, 0.0f};
    m[2] = {0.0f, 0.0f, 1.0f};
    m[3] = {x, y, z};
    return m;
}

void testOrientedPush() {
    const CollisionWorld empty;
    const AiVec3 moverHalf = {0.5f, 0.5f, 0.5f};
    const AiVec3 boxCentre = {0.0f, 0.0f, 0.0f};
    const AiVec3 boxHalf = {1.0f, 1.0f, 1.0f};
    const AiMatrix matrix = identityAt(0.0f, 0.0f, 0.0f);

    AiVec3 test = {1.0f, 0.0f, 0.0f};
    require(orientedBoxPush(empty, moverHalf, test, boxCentre, boxHalf, matrix, {-2.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, false, kFloorNormalMin),
            "the moved centre inside the grown box is a hit");
    require(!orientedBoxPush(empty, moverHalf, test, boxCentre, boxHalf, matrix, {-3.0f, 0.0f, 0.0f}, {-1.5f, 0.0f, 0.0f}, false, kFloorNormalMin),
            "the bounds are open");

    AiVec3 outside = {1.0f, 0.0f, 0.0f};
    orientedBoxPush(empty, moverHalf, outside, boxCentre, boxHalf, matrix, {-2.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, true, kFloorNormalMin);
    requireMove(outside, {1.0f + ((-1.5f - -1.0f) - kOrientedPushOut), 0.0f, 0.0f}, "a start outside on -x stops 0.007 before the face");

    AiVec3 inside = {0.1f, 0.0f, 0.0f};
    orientedBoxPush(empty, moverHalf, inside, boxCentre, boxHalf, matrix, {0.2f, 0.0f, 0.0f}, {0.3f, 0.0f, 0.0f}, true, kFloorNormalMin);
    requireMove(inside, {0.1f + (1.5f - 0.3f), 0.0f, 0.0f}, "a start inside leaves on the axis of the smallest depth of x and z");

    AiMatrix tilted = identityAt(0.0f, 0.0f, 0.0f);
    tilted[1] = {0.0f, 0.8f, 0.6f};
    tilted[2] = {0.0f, -0.6f, 0.8f};
    AiVec3 vertical = {0.0f, 0.01f, 0.0f};
    const AiVec3 moved = {0.0f, 0.96f, 0.72f};
    require(orientedBoxPush(empty, moverHalf, vertical, boxCentre, boxHalf, tilted, {0.0f, 0.95f, 0.72f}, moved, true, kFloorNormalMin),
            "a tilted box is hit");
    requireMove(vertical, {0.0f, 0.01f, 0.0f}, "|row 1 y| 0.8 is below 0.9, so y can be the axis; a push mostly along y is dropped");
}

constexpr std::uint32_t kMover = 1;
constexpr std::uint32_t kOwner = 2;

struct Scene {
    std::map<std::uint32_t, std::uint16_t> flags = {{kMover, 0x200}, {kOwner, 0x200}};
    std::map<std::uint32_t, AiMatrix> matrices = {{kMover, identityAt(0.0f, 0.0f, 0.0f)}, {kOwner, identityAt(0.0f, 0.0f, 1.0f)}};
    std::map<std::uint32_t, TrafficPushBox> boxes = {{kMover, {{0.0f, 0.5f, 0.0f}, {0.5f, 0.5f, 0.5f}}},
                                                     {kOwner, {{0.0f, 1.0f, 0.0f}, {0.25f, 1.0f, 0.25f}}}};
    std::vector<std::pair<std::uint32_t, AiVec3>> moves;
    std::vector<std::uint32_t> lastHits;
    std::optional<std::vector<TrafficPushPart>> parts;
    bool mesh = false;
};

TrafficPushShapes shapesOf(Scene& s) {
    TrafficPushShapes shapes;
    shapes.flags = [&s](std::uint32_t e) { return s.flags[e]; };
    shapes.kind = [](std::uint32_t) { return std::uint8_t(27); };
    shapes.nodeMatrix = [&s](std::uint32_t e) { return s.matrices[e]; };
    shapes.box = [&s](std::uint32_t e) { return s.boxes[e]; };
    shapes.entryMatrix = [&s](const TrafficContactEntry& entry) { return s.matrices[entry.owner]; };
    shapes.meshEntry = [&s](const TrafficContactEntry&) { return s.mesh; };
    shapes.moveNode = [&s](std::uint32_t entity, const AiVec3& delta) { s.moves.push_back({entity, delta}); };
    shapes.lastHit = [&s](std::uint32_t owner) { s.lastHits.push_back(owner); };
    shapes.floorNormalMin = kFloorNormalMin;
    return shapes;
}

void testConnectedPush() {
    Scene scene;
    TrafficPushShapes shapes = shapesOf(scene);
    const CollisionWorld world = wallWorld({0.0f, 1.0f, 1.3f}, {0.0f, 0.0f, -1.0f}, kUp, {1.0f, 0.0f, 0.0f});
    TrafficPushShapeWorld shapeWorld;
    shapeWorld.world = &world;
    shapeWorld.meshOf = [&world](const TrafficContactEntry&) { return &world.entries().front(); };
    shapeWorld.parts = [&scene](std::uint32_t) { return scene.parts; };
    connectTrafficPushShapes(shapes, shapeWorld);

    TrafficContactEntry entry;
    entry.owner = kOwner;
    AiVec3 vector{};
    require(trafficPushVector(shapes, kMover, {0.0f, 0.0f, 0.6f}, entry, &vector, true), "the car's moved box reaches the character");
    requireMove(vector, {0.0f, 0.0f, 0.0f}, "the wall behind the character takes its whole half of the push (slide 88, mode 17)");
    require(scene.moves.size() == 1 && scene.moves[0].first == kMover, "the car moves back");
    requireMove(scene.moves[0].second, {0.0f, 0.0f, -0.3f + (0.0f - 0.3f)}, "the car takes back its share and what the character could not take");

    scene.flags[kOwner] = 0x200 | kEntityFlagPushParts;
    scene.parts = std::nullopt;
    require(!trafficPushVector(shapes, kMover, {0.0f, 0.0f, 0.6f}, entry, &vector, false), "an owner with 0x800 without a part record: no test");
    TrafficPushPart part;
    part.matrix = identityAt(0.0f, 0.0f, 0.0f);
    part.box = {{0.0f, 1.0f, 0.0f}, {0.25f, 1.0f, 0.25f}};
    scene.parts = std::vector<TrafficPushPart>{part};
    scene.lastHits.clear();
    require(trafficPushVector(shapes, kMover, {0.0f, 0.0f, 0.6f}, entry, &vector, false), "the part's box times the entry matrix is hit");
    require(scene.lastHits == std::vector<std::uint32_t>{kOwner}, "an oriented hit stores the owner in *0x377134");

    scene.flags[kOwner] = 0x200;
    scene.mesh = true;
    scene.lastHits.clear();
    require(!trafficPushVector(shapes, kMover, {0.0f, 0.0f, 0.1f}, entry, &vector, true), "the mesh test misses the wall at z 1.3");
    require(trafficPushVector(shapes, kMover, {0.0f, 0.0f, 0.9f}, entry, &vector, true), "the mesh test reaches the wall");
    require(scene.lastHits == std::vector<std::uint32_t>{kOwner}, "a mesh hit stores the owner in *0x377134");
}

}

int main() {
    try {
        testWallSlide();
        testSlopeFlags();
        testCornerPass();
        testDuplicateRecords();
        testBoxEntry();
        testMeshPush();
        testOrientedPush();
        testConnectedPush();
    } catch (const std::exception& error) {
        std::cerr << "traffic-push-shapes: " << error.what() << '\n';
        return 1;
    }
    std::cout << "traffic-push-shapes: ok\n";
    return 0;
}
