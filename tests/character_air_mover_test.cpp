// The mover of a level character and the throw fall of the grab and takedown states (runtime/character_air_mover.hpp). The rules and constants are
// transcribed from llvm-objdump of 0x27726c, 0x283b10, 0x215804, 0x20cd30, 0x20b48c, 0x18f0b0..0x18f274, 0x1d6a64..0x1d6b08, 0x28d3a8..0x28d404,
// 0x2b24bc..0x2b2558 and 0x28b46c (20261112-enemy-E, 2026-11-12, docs/re/scripts-missions-1-4-roof.md); the roof, the street and the moves are
// made up, the expected positions hand-computed from those rules.
#include "runtime/character_air_mover.hpp"

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

bool near(float a, float b) { return std::fabs(a - b) < 1e-4f; }

assets::CollisionItem floorItem(float height, float minX, float maxX) {
    assets::CollisionItem item;
    item.name = "Collision_Test";
    const float quad[12] = {minX, height, -100, minX, height, 100, maxX, height, 100, maxX, height, -100};
    item.vertices.assign(quad, quad + 12);
    for (const std::array<std::uint32_t, 3>& corner : {std::array<std::uint32_t, 3>{0, 1, 2}, {0, 2, 3}}) {
        assets::CollisionTriangle triangle;
        for (int a = 0; a < 3; ++a) triangle.vertex[a] = corner[std::size_t(a)];
        triangle.normal[0] = 0;
        triangle.normal[1] = 1;
        triangle.normal[2] = 0;
        triangle.d = height;
        item.triangles.push_back(triangle);
    }
    assets::CollisionNode node;
    node.centre[0] = (minX + maxX) * 0.5f;
    node.centre[1] = height;
    node.half[0] = (maxX - minX) * 0.5f;
    node.half[1] = 1;
    node.half[2] = 100;
    node.info = 1 | (std::uint32_t(item.triangles.size()) << 1);
    item.nodes.push_back(node);
    return item;
}

CollisionWorld roofAndStreet() {
    CollisionWorld world;
    world.add(floorItem(10.0f, -20.0f, 0.0f));
    world.add(floorItem(0.0f, 2.0f, 100.0f));
    return world;
}

constexpr float kDt = 1.0f / 30.0f;

void standingCharacterIsNotProbed() {
    const CollisionWorld world = roofAndStreet();
    CharacterAir air;
    AirVector position{-5, 10, 0};
    const CharacterMoveResult result = moveCharacter(air, position, 1, 0, CharacterMove{}, world, kDt);
    require(!result.probed && air.onGround && position[1] == 10.0f, "0x20d5a8: no movement on the ground runs no probe");
}

void stepOnTheRoofKeepsTheGround() {
    const CollisionWorld world = roofAndStreet();
    CharacterAir air;
    AirVector position{-5, 10, 0};
    CharacterMove move;
    move.root = {0.3f, 0, 0};
    const CharacterMoveResult result = moveCharacter(air, position, 1, 0, move, world, kDt);
    require(result.probed && !result.landed && air.onGround && near(position[0], -4.7f) && near(position[1], 10.0f), "a step on the roof finds it");
    require(air.groundBelow && near(*air.groundBelow, 10.0f) && near(air.lastMove[0], 0.3f), "ctrl+0x244 and ctrl+0x204");
}

// The pair motion (flags 65: no gravity) carries the goon past the roof edge: the probe finds nothing, the node stays at its height, ctrl+0x314 bit 6 is
// clear; with bit 4 set the tail of 0x27726c starts the fall with the last move as the flight vector.
void throwOffTheRoofFallsAndLands() {
    const CollisionWorld world = roofAndStreet();
    CharacterAir air;
    air.fallAllowed = true;
    AirVector position{-0.5f, 10, 0};
    CharacterMove move;
    move.root = {1.0f, 0, 0};
    const CharacterMoveResult edge = moveCharacter(air, position, 1, 0, move, world, kDt);
    require(edge.fallEvent && !edge.fallStateRequest && !air.onGround && near(position[0], 0.5f) && position[1] == 10.0f,
            "off the edge: event 0x26 without the gravity mode leaves the height");
    require(startThrownFall(air, position[1]) && air.thrown && near(air.velocity[0], 1.0f) && air.velocity[1] == 0.0f, "the fall starts");
    require(!startThrownFall(air, position[1]), "bit 5 set: no second start");
    int updates = 0;
    float expectedY = 10.0f;
    while (updates < 40) {
        const std::optional<ThrownFall> flight = thrownFlight(air, position, 1, 0, world, kDt);
        require(flight && flight->step == ThrownFallStep::Flying, "in the air every update is a flight step");
        ++updates;
        if (flight->move.landed) break;
        expectedY -= kThrownFallStep * float(updates);
        require(near(position[1], expectedY) && near(position[0], 0.5f + float(updates)) && !air.onGround, "the flight vector, y down by 0.05 more each update");
        require(near(air.fallSpeed, kThrownFallStep * float(updates)), "ctrl+0x2b8 = -flight y");
    }
    // Flight update n starts at y = 10 - 0.025 (n - 1) n with ctrl+0x2b8 = 0.05 n, and the probe ends 0.1 + max(0.05 n, 1 x 0.7933 - 0.1) below the
    // node: update 19 starts at 1.45 and ends at 0.4, update 20 starts at 0.5 and reaches the street.
    require(updates == 20 && air.onGround && std::fabs(position[1]) < kBisectionStop && air.fallSpeed == 0.0f,
            "the street stops the fall in update 20 (the probe bisects to within 0x3ba3d70a of it)");
    const std::optional<ThrownFall> after = thrownFlight(air, position, 1, 0, world, kDt);
    require(after && after->step == ThrownFallStep::Landed && !air.thrown, "the next update clears bit 5 and resumes the clip");
    require(!thrownFlight(air, position, 1, 0, world, kDt), "bit 5 clear: the head does nothing");
}

void fallNeedsTheTrackAndADrop() {
    CharacterAir air;
    air.onGround = false;
    require(!startThrownFall(air, 10.0f), "bit 4 clear: no fall");
    air.fallAllowed = true;
    air.groundBelow = 9.97f;
    require(!startThrownFall(air, 10.0f), "|y - ctrl+0x244| at most 0.05: no fall");
    air.groundBelow = 9.9f;
    require(startThrownFall(air, 10.0f), "a drop above 0.05 starts the fall");
    CharacterAir standing;
    standing.fallAllowed = true;
    require(!startThrownFall(standing, 10.0f), "on the ground: no fall");
}

// 0x283b10 without bit 5 moves with flags 64: entity +0xc bit 5 set, so a missing ground asks FALL (5) and runs the gravity step 0x18f1dc.
void gravityModeFalls() {
    const CollisionWorld world = roofAndStreet();
    CharacterAir air;
    AirVector position{-0.5f, 10, 0};
    CharacterMove move;
    move.root = {1.0f, 0, 0};
    move.gravity = true;
    const CharacterMoveResult result = moveCharacter(air, position, 1, 0, move, world, kDt);
    require(result.fallEvent && result.fallStateRequest && near(air.fallSpeed, 0.05f) && near(position[1], 9.95f) && near(air.fallDistance, 0.05f),
            "gravity 0.05 x 30 x dt, then the node drops by ctrl+0x2b8");
    move.root = {};
    moveCharacter(air, position, 1, 0, move, world, kDt);
    require(near(air.fallSpeed, 0.1f) && near(position[1], 9.85f), "the next update grows the speed again");
}

void riseLeavesTheGround() {
    const CollisionWorld world = roofAndStreet();
    CharacterAir air;
    AirVector position{-5, 10, 0};
    CharacterMove move;
    move.vector = {0, 0.2f, 0};
    const CharacterMoveResult result = moveCharacter(air, position, 1, 0, move, world, kDt);
    require(!result.probed && !air.onGround && near(position[1], 10.2f) && near(air.fallSpeed, -0.2f) && !air.groundBelow, "0x20d464: a rise clears bit 6");
    CharacterAir gravity;
    AirVector second{-5, 10, 0};
    move.gravity = true;
    moveCharacter(gravity, second, 1, 0, move, world, kDt);
    require(near(gravity.fallSpeed, -0.15f), "a rise in the gravity mode adds 0x21df6c to ctrl+0x2b8");
}

assets::Animation fallTrackClip(std::uint32_t aspect) {
    assets::Animation clip;
    clip.frames = 47;
    clip.subAspects = {aspect};
    assets::AnimTrack track;
    track.hash = kFallAllowedTrack;
    for (const auto& [time, value] : {std::pair<float, float>{0.0f, 0.0f}, {0.282526f, 1.0f}, {1.0f, 0.0f}}) {
        assets::CurveKey key;
        key.time = time;
        key.value = value;
        track.keys.push_back(key);
    }
    clip.subAnimations.push_back({track});
    return clip;
}

void trackSetsAndClearsBitFour() {
    const assets::Animation clip = fallTrackClip(kFallAllowedAspect);
    CharacterAir air;
    sampleFallAllowed(air, clip, 0.2f);
    require(!air.fallAllowed, "value 0 before the key");
    sampleFallAllowed(air, clip, 0.3f);
    require(air.fallAllowed, "value 1.0 sets bit 4");
    sampleFallAllowed(air, clip, 1.0f);
    require(!air.fallAllowed, "value 0 at the end clears it");
    air.fallAllowed = true;
    sampleFallAllowed(air, fallTrackClip(0x607ed1b7), 0.1f);
    require(air.fallAllowed, "the track under another event kind is not read by 0x1d6a64");
}

void enterClearsTheBits() {
    CharacterAir air;
    air.fallAllowed = air.thrown = true;
    require(!takedownEnterAir(air, false) && !air.thrown && !air.fallAllowed, "0x28d3b4 / 0x28d3fc clear bits 5 and 4");
    air.onGround = false;
    air.lastMove = {0.1f, -0.2f, 0.3f};
    require(takedownEnterAir(air, true) && air.thrown && near(air.velocity[1], -0.2f) && !air.fallAllowed, "a gun HIT state in the air starts the fall");
}

void landingWithTheMark() {
    require(fallArrestHitOnLanding(true, 0x135), "PUTDOWN STUN with the mark requests 310");
    require(!fallArrestHitOnLanding(false, 0x135), "no mark, no request");
    for (const std::uint16_t state : {227, 228, 229, 230, 231, 307, 325, 409})
        require(!fallArrestHitOnLanding(true, state), "0x28b46c holds for " + std::to_string(state));
}

void statesAndTheirUpdates() {
    require(takedownMover(227) == TakedownMover::Putdown, "227 runs 0x27726c");
    for (const std::uint16_t state : {0x122, 0x12f, 0x133, 0x135, 0x136, 0x139, 0x13e}) require(takedownMover(state) == TakedownMover::Takedown, "0x283b10");
    for (const std::uint16_t state : {1, 0x130, 0x132, 0x137, 0x138, 0x13f}) require(takedownMover(state) == TakedownMover::None, "not registered with 0x283b10");
}
}

int main() {
    try {
        standingCharacterIsNotProbed();
        stepOnTheRoofKeepsTheGround();
        throwOffTheRoofFallsAndLands();
        fallNeedsTheTrackAndADrop();
        gravityModeFalls();
        riseLeavesTheGround();
        trackSetsAndClearsBitFour();
        enterClearsTheBits();
        landingWithTheMark();
        statesAndTheirUpdates();
    } catch (const std::exception& error) {
        std::cerr << "character-air-mover-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "character-air-mover-test ok\n";
    return 0;
}
