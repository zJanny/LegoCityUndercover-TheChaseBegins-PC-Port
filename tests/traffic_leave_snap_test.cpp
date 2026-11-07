// Tests of src/runtime/traffic_leave_snap.*: the leave call 0x21d6b8 and the ground colour probe 0x20ae2c with the triangle weights 0x2b51b8 and
// the colour of the hit 0x221064. Every expected value follows from the disassembly (llvm-objdump --triple=armv6k --mattr=+vfp2 of
// artifacts/game/game.arm.elf, read by 20261103-large-B on 2026-11-03, docs/re/traffic-ai-driver.md), evaluated by hand in single
// precision on triangles whose weights are powers of two; no emulator was used.
#include "runtime/traffic_leave_snap.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

template <typename Function>
bool throws(Function function) {
    try {
        function();
    } catch (const std::exception&) {
        return true;
    }
    return false;
}

TrafficGroundTriangle flatTriangle() {
    TrafficGroundTriangle triangle;
    triangle.normal = {0.0f, 1.0f, 0.0f};
    triangle.corners = {AiVec3{0, 0, 0}, AiVec3{8, 0, 0}, AiVec3{0, 0, 8}};
    triangle.colourIndex = {3, 5, 7};
    return triangle;
}

TrafficGroundMesh coloured() {
    TrafficGroundMesh mesh;
    mesh.hasColours = true;
    mesh.colours.assign(8, 0);
    mesh.colours[3] = 0x00204080u;
    mesh.colours[5] = 0x00fa6414u;
    mesh.colours[7] = 0x0000ffffu;
    return mesh;
}

void weights() {
    const auto result = trafficTriangleWeights(flatTriangle(), {2, 0, 2});
    require(result && (*result)[0] == 0.5f && (*result)[1] == 0.25f && (*result)[2] == 0.25f, "weights of a point in a triangle of the xz plane");

    TrafficGroundTriangle degenerate = flatTriangle();
    degenerate.corners = {AiVec3{1, 0, 1}, AiVec3{1, 0, 1}, AiVec3{1, 0, 1}};
    require(!trafficTriangleWeights(degenerate, {1, 0, 1}), "a zero determinant fails (0x2b52a8)");

    TrafficGroundTriangle slanted;
    slanted.normal = {0.05f, 0.1f, 1.0f};
    slanted.corners = {AiVec3{0, 0, 0}, AiVec3{8, 0, 0}, AiVec3{0, 8, 0}};
    const auto xy = trafficTriangleWeights(slanted, {2, 2, 0});
    require(xy && (*xy)[0] == 0.5f && (*xy)[1] == 0.25f && (*xy)[2] == 0.25f, "a dominant z normal with |y| above |x| projects on x and y");
    slanted.normal = {0.1f, 0.05f, 1.0f};
    require(!trafficTriangleWeights(slanted, {2, 2, 0}),
            "the original projects on x and z when |x| is above |y| and |z| is the largest (0x2b51dc, 0x2b51e8 .. 0x2b520c)");
}

void colour() {
    const TrafficGroundTriangle triangle = flatTriangle();
    const auto vertex = trafficGroundColour(coloured(), triangle, {2, 0, 2});
    require(vertex && *vertex == 0xff84784eu, "vertex colours: channel sums 132.75 / 120.75 / 78.5 truncated, byte 0 in bits 16 to 23, alpha 255");

    TrafficGroundMesh plain;
    TrafficGroundTriangle indices = triangle;
    indices.colourIndex = {10, 20, 30};
    const auto grey = trafficGroundColour(plain, indices, {2, 0, 2});
    require(grey && *grey == 0xff111111u, "without a colour table the shade is the weighted indices, 17.5 -> 17");
    indices.colourIndex = {255, 255, 255};
    require(*trafficGroundColour(plain, indices, {2, 0, 2}) == 0xffffffffu, "the shade is clamped to 255");

    TrafficGroundMesh bright;
    bright.hasColours = true;
    bright.colours.assign(8, 0xffffffffu);
    require(*trafficGroundColour(bright, triangle, {2, 0, 2}) == 0xffffffffu, "white stays white");

    TrafficGroundTriangle bad = triangle;
    bad.colourIndex = {3, 5, 9};
    require(throws([&] { trafficGroundColour(coloured(), bad, {2, 0, 2}); }), "a colour index beyond the table is refused");
}

void snap() {
    AiDriverRecord driver;
    driver.target = {10.0f, 4.0f, -3.0f};
    const TrafficGroundSegment segment = trafficGroundSegment(driver.target);
    require(segment.from == AiVec3({10.0f, 5.5f, -3.0f}) && segment.to == AiVec3({10.0f, 2.5f, -3.0f}), "the ray runs from 1.5 above to 1.5 below the target");

    TrafficGroundSegment cast;
    int sent = 0;
    std::uint32_t sentColour = 0;
    std::optional<TrafficGroundHit> hit;
    TrafficGroundWorld world;
    world.cast = [&](const TrafficGroundSegment& ray) {
        cast = ray;
        return hit;
    };
    world.sendColour = [&](std::uint32_t value) {
        ++sent;
        sentColour = value;
    };
    require(!snapTrafficTarget(driver, world) && sent == 0 && cast.from == segment.from, "no hit sends nothing");
    hit = TrafficGroundHit{flatTriangle(), coloured(), {2, 0, 2}};
    require(snapTrafficTarget(driver, world) && sent == 1 && sentColour == 0xff84784eu, "a hit sends the colour of the hit point");
    hit->triangle.corners = {AiVec3{1, 0, 1}, AiVec3{1, 0, 1}, AiVec3{1, 0, 1}};
    require(!snapTrafficTarget(driver, world) && sent == 1, "a hit whose weights fail sends nothing");

    TrafficGroundWorld missing;
    require(throws([&] { snapTrafficTarget(driver, missing); }), "a world without a ray cast is refused");
    missing.cast = world.cast;
    require(throws([&] { snapTrafficTarget(driver, missing); }), "a world without a colour receiver is refused");
}

void leave() {
    GameRandom random(0x77u);
    TrafficManager manager;
    manager.loadLevel(3, std::nullopt, random);
    manager.cars()[0].disabled = false;
    manager.cars()[1].disabled = false;
    manager.cars()[1].pinned = true;
    std::vector<TrafficEvent> events;
    require(leaveTraffic(manager, 0, &events) && manager.cars()[0].disabled && events.size() == 1 && events[0].kind == TrafficEventKind::Despawn &&
                events[0].car == 0 && manager.fades()[0] == kTrafficOpaque,
            "an enabled car is despawned and its fade byte set to 255 (0x21d6f8 .. 0x21d720)");
    require(leaveTraffic(manager, 0, &events) && events.size() == 1, "a disabled car is not despawned twice");
    require(leaveTraffic(manager, 1, &events) && !manager.cars()[1].disabled && events.size() == 1, "a pinned car (flag 0x20) is left alone (0x21d6f0)");
    require(!leaveTraffic(manager, 3, &events) && events.size() == 1, "a car outside the table is not found (0x21d738)");
}

}

int main() {
    try {
        weights();
        colour();
        snap();
        leave();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << "\n";
        return 1;
    }
    return 0;
}
