// The pedestrian character, kind 197 (docs/re/pedestrians.md). Provenance of the pinned values: llvm-objdump of
// artifacts/game/game.arm.elf. Clip names: the strings of the create 0x1913f8 (0x1916f4 .. 0x1917f4); IsMale 4 / 8 at 0x19169c.
// State step 0x1d7e88: entry 1 samples the path at parameter +/- 0.1 (0x1d81cc / 0x1d81d0), lateral offset along
// (-tangent z, tangent x), hold = random % 50 + 10, walk clip in mode 1; entries 3 / 4 the dodge clip (+0x5f == 1 picks 3 / 2,
// else 1 / 0) in mode 0 with 0x239568; entry 5 random % 2, mode 0, the heading to the player (0x23df44, 0x23deb4); entry 6 the
// splat clip in mode 1 and 3.0 s (0x1d81c0); entry 7 splat_up in mode 0; state 1 requests 2; the hold keeps at least 10 while
// 0x2930e0 and not 0x23de70 (0x1d8274). Walk 0x2b9138: turn rate 5.0 (0x2b945c), velocity row 2 x speed x dt, one ground
// probe per update (byte 0x375d5c) or one when tilted over 0.34906587 (0x2b9470), touch below 1.5 squared xz (0x1fd8c8) with
// the push 0.1 (0x1fd9c4) and the bump (state 5) for player states 2 and 3, ray end y -1.0 (0x2b9478), the rows rebuilt
// with up (0, 1, 0) (0x56d424), path step 0.3 (0x2b96b8), turn back 0.75 (0x2b96a8), sidestep 1.3 (0x2b96b0) with
// +0x5d = 30, face the player for requests 3 / 4. Avoidance 0x2edf08: xz length below 2.0 (0x2ee05c), side |dot| below
// 1.0 (0x2ee064), 2 for a positive side, else 1.
#include "assets/assets.hpp"
#include "runtime/game_random.hpp"
#include "runtime/pedestrian_walk.hpp"

#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b) { return std::fabs(a - b) < 1e-5f; }
bool nearVec(const PedestrianVec3& a, const PedestrianVec3& b) { return near(a[0], b[0]) && near(a[1], b[1]) && near(a[2], b[2]); }

using Node = std::array<PedestrianVec3, 4>;

Node identityAt(const PedestrianVec3& position) { return {PedestrianVec3{1, 0, 0}, PedestrianVec3{0, 1, 0}, PedestrianVec3{0, 0, 1}, position}; }

PedestrianPath straightPath(std::uint16_t points) {
    PedestrianPath path;
    path.pointCount = points;
    path.sample = [](float parameter, PedestrianVec3& position, PedestrianVec3& tangent) {
        position = {0.0f, 0.0f, parameter * 10.0f};
        tangent = {0.0f, 0.0f, 1.0f};
    };
    return path;
}

struct Scene {
    PedestrianPath path = straightPath(4);
    PedestrianWalkWorld world;
    bool token = false;

    Scene() {
        world.dt = 1.0f / 30.0f;
        world.path = &path;
        world.arrivalRadius = 0.5f;
        world.playerPosition = {50.0f, 0.0f, 50.0f};
        world.groundProbeTaken = &token;
        world.visible = true;
    }
};

PedestrianBody walking(const PedestrianVec3& position, const PedestrianVec3& target) {
    PedestrianBody body = pedestrianCreate(true, identityAt(position));
    body.state = body.requested = 2;
    body.forward = true;
    body.speed = 3.0f;
    body.target = target;
    return body;
}

void testCreate() {
    require(std::string(pedestrianClipName(PedestrianClip::Walk)) == "walk_minifig", "walk clip");
    require(std::string(pedestrianClipName(PedestrianClip::DodgeLeft02)) == "dodge_left02_minifig", "dodge clip");
    require(std::string(pedestrianClipName(PedestrianClip::SplatUp)) == "splat_up_minifig", "splat up clip");
    require(pedestrianCreate(true, identityAt({0, 0, 0})).voice == 4, "male voice");
    require(pedestrianCreate(false, identityAt({0, 0, 0})).voice == 8, "female voice");
    const PedestrianBody body = pedestrianCreate(true, identityAt({0, 0, 0}));
    require(body.state == 0 && body.requested == 0, "create states");
}

void testSpawnEntry() {
    Scene scene;
    PedestrianBody body = pedestrianCreate(true, identityAt({0, 0, 11}));
    body.requested = 1;
    body.forward = true;
    body.pathParameter = 1.0f;
    body.lateral = 0.25f;
    body.speed = 2.5f;
    GameRandom random(1234);
    GameRandom copy = random;
    const std::uint32_t hold = copy.next() % 50 + 10;

    const PedestrianWalkUpdate first = PedestrianWalk::update(body, scene.world, random);
    require(!first.walked, "no walk on entry");
    require(body.state == 1 && body.requested == 2, "state 1 requests 2");
    require(nearVec(body.target, {-0.25f, 0.0f, 11.0f}), "entry target at parameter + 0.1 with lateral offset");
    require(near(body.pathParameter, 1.0f), "entry keeps the parameter");
    require(body.hold == hold, "hold random % 50 + 10");
    require(body.walkStarted, "walk flag");
    require(first.animations.size() == 1, "one animation");
    const PedestrianAnimationRequest& walk = first.animations[0];
    require(walk.clip == PedestrianClip::Walk && walk.mode == 1 && walk.first == 0 && walk.end == 0xffff, "walk request");
    require(walk.speed == 1.0f && walk.blend == 0.0f && !walk.rootMotion, "walk request speed and blend");

    const PedestrianWalkUpdate second = PedestrianWalk::update(body, scene.world, random);
    require(!second.walked && second.animations.empty() && body.state == 2, "entry 2");
    const PedestrianWalkUpdate third = PedestrianWalk::update(body, scene.world, random);
    require(third.walked, "state 2 walks");

    PedestrianBody back = pedestrianCreate(true, identityAt({0, 0, 0}));
    back.requested = 1;
    back.forward = false;
    back.pathParameter = 1.0f;
    back.lateral = 0.25f;
    PedestrianWalk::update(back, scene.world, random);
    require(nearVec(back.target, {-0.25f, 0.0f, 9.0f}), "entry target at parameter - 0.1");
}

void testStraightWalk() {
    Scene scene;
    PedestrianBody body = walking({0, 0, 0}, {0, 0, 20});
    GameRandom random(1);
    const PedestrianWalkUpdate out = PedestrianWalk::update(body, scene.world, random);
    require(out.walked && out.groundProbed && scene.token, "first walker probes the ground");
    require(nearVec(body.node[3], {0.0f, 0.0f, 0.1f}), "moved by speed x dt along row 2");
    require(nearVec(body.target, {0, 0, 20}), "far target kept");

    PedestrianBody other = walking({5, 0, 0}, {5, 0, 20});
    const PedestrianWalkUpdate second = PedestrianWalk::update(other, scene.world, random);
    require(!second.groundProbed, "one probe per update");
}

void testTurn() {
    Scene scene;
    PedestrianBody body = walking({0, 0, 0}, {10, 0, 0});
    GameRandom random(1);
    PedestrianWalk::update(body, scene.world, random);
    const float inverse = 1.0f / std::sqrt(26.0f);
    require(nearVec(body.node[2], {inverse, 0.0f, 5.0f * inverse}), "row 2 turned by dt x 5 and rebuilt");
    require(nearVec(body.node[0], {5.0f * inverse, 0.0f, -inverse}), "row 0 = unit(up x row 2)");
    require(nearVec(body.node[1], {0, 1, 0}), "row 1 up");
    require(nearVec(body.node[3], {0.1f / 6.0f, 0.0f, 0.5f / 6.0f}), "velocity along the blended row 2");
}

void testArrival() {
    Scene scene;
    GameRandom random(1);
    PedestrianBody body = walking({0, 0, 38.9f}, {0, 0, 39});
    body.pathParameter = 3.9f;
    body.lateral = -0.5f;
    PedestrianWalk::update(body, scene.world, random);
    require(near(body.pathParameter, 0.2f), "parameter + 0.3 wraps at the point count");
    require(nearVec(body.target, {0.5f, 0.0f, 2.0f}), "next target with lateral offset");

    PedestrianBody backward = walking({0, 0, 1.0f}, {0, 0, 1.0f});
    backward.forward = false;
    backward.pathParameter = 0.1f;
    PedestrianWalk::update(backward, scene.world, random);
    require(near(backward.pathParameter, 3.8f), "parameter - 0.3 wraps below 0");
}

void testTurnBack() {
    Scene scene;
    GameRandom random(1);
    scene.world.blocked = [](const PedestrianVec3&, const PedestrianVec3&, float) { return 4; };
    PedestrianBody body = walking({0, 0, 10}, {0, 0, 20});
    body.pathParameter = 1.5f;
    PedestrianWalk::update(body, scene.world, random);
    require(!body.forward, "direction flipped");
    require(near(body.pathParameter, 0.75f), "parameter - 0.75");
    require(nearVec(body.target, {0.0f, 0.0f, 7.5f}), "target at the new parameter");
}

void testAvoidance() {
    Scene scene;
    GameRandom random(1);
    scene.world.playerPosition = {0.5f, 0.0f, 1.0f};
    PedestrianBody body = walking({0, 0, 0}, {0, 0, 20});
    require(PedestrianWalk::avoidance(body, scene.world) == 2, "player on the row 0 side");
    PedestrianWalk::update(body, scene.world, random);
    require(nearVec(body.target, {-1.3f, 0.0f, 1.4f}), "sidestep away from the player");
    require(body.avoidDelay == 29, "avoid delay 30, one counted down");
    require(body.requested == 2, "player state 0 does not bump");

    PedestrianBody next = walking({0, 0, 0}, {0, 0, 20});
    next.avoidDelay = 5;
    PedestrianWalk::update(next, scene.world, random);
    require(nearVec(next.target, {0, 0, 20}), "no avoidance while the delay runs");

    Scene crowd;
    crowd.world.otherPedestrians = {{-0.5f, 0.0f, 1.0f}};
    PedestrianBody left = walking({0, 0, 0}, {0, 0, 20});
    require(PedestrianWalk::avoidance(left, crowd.world) == 1, "pedestrian on the other side");
    PedestrianWalk::update(left, crowd.world, random);
    require(nearVec(left.target, {1.3f, 0.0f, 1.4f}), "sidestep to row 0");

    crowd.world.otherPedestrians = {{1.5f, 0.0f, 1.0f}, {0.0f, 0.0f, 2.5f}};
    require(PedestrianWalk::avoidance(left, crowd.world) == 0, "outside the side limit or the distance");
}

void testBump() {
    Scene scene;
    scene.world.playerPosition = {1.0f, 0.0f, 0.0f};
    scene.world.playerState = 2;
    PedestrianVec3 rayTo{};
    scene.world.groundRay = [&](const PedestrianVec3&, const PedestrianVec3& to) -> std::optional<float> {
        rayTo = to;
        return 0.25f;
    };
    GameRandom random(77);
    GameRandom copy = random;
    const std::uint8_t clip = std::uint8_t(copy.next() % 2);
    PedestrianBody body = walking({0, 0, 0}, {0, 0, 20});
    const PedestrianWalkUpdate out = PedestrianWalk::update(body, scene.world, random);
    require(body.state == 5 && body.requested == 5, "touch with player state 2 bumps");
    require(near(rayTo[1], -1.0f) && near(body.node[3][1], 0.25f), "ground ray to y -1.0");
    require(nearVec(body.node[3], {-0.1f, 0.25f, 0.1f}), "pushed away by 0.1");
    require(out.animations.size() == 1 && out.animations[0].clip == (clip ? PedestrianClip::Bump02 : PedestrianClip::Bump01), "bump clip");
    require(out.animations[0].mode == 0 && !out.animations[0].rootMotion, "bump mode");
    const float facing = 1.0f / std::sqrt(1.1f * 1.1f + 0.1f * 0.1f);
    require(std::fabs(body.node[2][0] - 1.1f * facing) < 1e-3f && std::fabs(body.node[2][2] + 0.1f * facing) < 1e-3f, "faces the player");
    require(near(body.node[0][0], body.node[2][2]) && near(body.node[0][2], -body.node[2][0]) && near(body.node[1][1], 1.0f), "yaw rows");

    scene.world.clipFinished = [](PedestrianClip) { return true; };
    PedestrianWalk::update(body, scene.world, random);
    require(body.requested == 1, "bump ends into state 1");

    Scene driving;
    driving.token = true;
    driving.world.playerPosition = {1.0f, 0.0f, 0.0f};
    driving.world.playerState = 2;
    driving.world.playerInVehicle = true;
    PedestrianBody passed = walking({0, 0, 0}, {0, 0, 20});
    PedestrianWalk::update(passed, driving.world, random);
    require(passed.state == 2 && passed.requested == 2, "no touch test from a vehicle without a probe");
}

void testDodgeFacesPlayer() {
    Scene scene;
    scene.world.playerPosition = {0.0f, 0.0f, -10.0f};
    PedestrianBody body = walking({0, 0, 0}, {0, 0, 20});
    body.dodgeSide = 1;
    scene.world.blocked = [&](const PedestrianVec3&, const PedestrianVec3&, float) {
        body.requested = 4;
        return 0;
    };
    GameRandom random(1);
    const PedestrianWalkUpdate out = PedestrianWalk::update(body, scene.world, random);
    require(nearVec(body.node[2], {0.0f, 0.0f, -1.0f}), "turned to the player");
    require(body.state == 4 && body.dodgeClip == 2, "dodge 4 with side 1");
    require(out.animations.size() == 1 && out.animations[0].clip == PedestrianClip::DodgeRight02, "dodge clip");
    require(out.animations[0].mode == 0 && out.animations[0].rootMotion, "dodge root motion");

    PedestrianBody left = walking({0, 0, 0}, {0, 0, 20});
    left.state = 2;
    left.requested = 3;
    const PedestrianWalkUpdate three = PedestrianWalk::update(left, scene.world, random);
    require(left.dodgeClip == 1 && three.animations[0].clip == PedestrianClip::DodgeLeft01, "dodge 3 with side 0");
}

void testSplat() {
    Scene scene;
    scene.world.dt = 1.0f;
    GameRandom random(1);
    PedestrianBody body = walking({0, 0, 0}, {0, 0, 20});
    body.requested = 6;
    PedestrianWalkUpdate out = PedestrianWalk::update(body, scene.world, random);
    require(out.animations.size() == 1 && out.animations[0].clip == PedestrianClip::Splat && out.animations[0].mode == 1, "splat loop");
    require(near(body.splatTimer, 2.0f), "3.0 s minus the first update");
    PedestrianWalk::update(body, scene.world, random);
    require(body.requested == 6, "still lying");
    PedestrianWalk::update(body, scene.world, random);
    require(body.requested == 7, "gets up at 0");
    out = PedestrianWalk::update(body, scene.world, random);
    require(out.animations.size() == 1 && out.animations[0].clip == PedestrianClip::SplatUp && out.animations[0].rootMotion, "splat up");
    scene.world.clipFinished = [](PedestrianClip clip) { return clip == PedestrianClip::SplatUp; };
    PedestrianWalk::update(body, scene.world, random);
    require(body.requested == 1, "walks again");

    PedestrianBody held = walking({0, 0, 0}, {0, 0, 20});
    held.requested = 6;
    PedestrianWalk::update(held, scene.world, random);
    scene.world.splatRefresh = true;
    PedestrianWalk::update(held, scene.world, random);
    require(near(held.splatTimer, 2.0f), "refresh sets 3.0 again");
}

void testHold() {
    Scene scene;
    GameRandom random(1);
    PedestrianBody body = walking({0, 0, 0}, {0, 0, 20});
    body.hold = 3;
    PedestrianWalk::update(body, scene.world, random);
    require(body.hold == 10, "visible keeps at least 10");
    scene.world.visible = false;
    PedestrianWalk::update(body, scene.world, random);
    require(body.hold == 9, "counts down out of view");
    scene.world.visible = true;
    scene.world.hidden = true;
    PedestrianWalk::update(body, scene.world, random);
    require(body.hold == 8, "counts down while hidden");
    PedestrianBody idle = pedestrianCreate(true, identityAt({0, 0, 0}));
    idle.hold = 0;
    scene.world.hidden = false;
    PedestrianWalk::update(idle, scene.world, random);
    require(idle.hold == 0, "state 0 is not kept");
}

void testTilt() {
    const float s = std::sin(0.5f), c = std::cos(0.5f);
    const Node pitched{PedestrianVec3{1, 0, 0}, PedestrianVec3{0, c, -s}, PedestrianVec3{0, s, c}, PedestrianVec3{0, 0, 0}};
    require(near(pedestrianTiltAngle(pitched), -0.5f), "pitch angle");
    require(near(pedestrianTiltAngle(identityAt({0, 0, 0})), 0.0f), "upright");

    Scene scene;
    scene.token = true;
    scene.world.dt = 0.2f;
    GameRandom random(1);
    PedestrianBody body = walking({0, 0, 0}, {0, 10, 10});
    body.avoidDelay = 5;
    const PedestrianWalkUpdate out = PedestrianWalk::update(body, scene.world, random);
    require(out.groundProbed, "a tilted walker probes the ground again");
    PedestrianBody flat = walking({0, 0, 0}, {0, 0, 10});
    flat.avoidDelay = 5;
    require(!PedestrianWalk::update(flat, scene.world, random).groundProbed, "an upright one does not");
}

void testLookAt() {
    Node node = identityAt({1, 2, 3});
    pedestrianLookAt(node, {0, 0, 0}, {0, 5, 0});
    require(nearVec(node[2], {0, 1, 0}) && nearVec(node[0], {0, 0, 1}) && nearVec(node[1], {1, 0, 0}), "steep look-at rows");
    require(nearVec(node[3], {1, 2, 3}), "position kept");
}

void testLevelPath() {
    chase::assets::LevelPath level;
    level.points = {{0, 0, 0}, {0, 0, 10}, {0, 0, 20}, {0, 0, 30}};
    const PedestrianPath path = pedestrianLevelPath(level);
    PedestrianVec3 position{}, tangent{};
    path.sample(1.5f, position, tangent);
    require(path.pointCount == 4, "point count");
    require(nearVec(position, {0, 0, 15}) && nearVec(tangent, {0, 0, 1}), "type 0 sample");
}

}

int main() {
    try {
        testCreate();
        testSpawnEntry();
        testStraightWalk();
        testTurn();
        testArrival();
        testTurnBack();
        testAvoidance();
        testBump();
        testDodgeFacesPlayer();
        testSplat();
        testHold();
        testTilt();
        testLookAt();
        testLevelPath();
    } catch (const std::exception& error) {
        std::cerr << "pedestrian-walk: " << error.what() << "\n";
        return 1;
    }
    std::cout << "pedestrian-walk: ok\n";
    return 0;
}
