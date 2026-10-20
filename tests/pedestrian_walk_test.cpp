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
// +0x5d = 30, face the player for requests 3 / 4. The vehicle loop 0x2b9340 .. 0x2b93a4 passes the node position, the step
// vector (sp +0x88) and dt to 0x1ffcd4, and 0x2b93c0 adds that vector to the position after the loop. Avoidance 0x2edf08: xz length below 2.0 (0x2ee05c), side |dot| below
// 1.0 (0x2ee064), 2 for a positive side, else 1.
// Steady updates 0x277c24: states 3, 4, 7 the root motion 0x277afc (translation through the node rows 0x258a10, minus the
// position; wall test 0x2059ec with (x, 0, z) and the near radius +0x5c x 1.5 (0x205b54), probe 0x2058cc); state 5 0x277c94
// (0x1fd8c8 with wall and probe set on the zero vector 0x56d400, the node moved by it again on a touch). Probe 0x2058cc: the
// floats 0.25 / 0.25 of the static initialiser 0x347904. Entity box 0x242df4 (scale 0.5 at 0x242eac), radius 0x2536a8. Draw
// test 0x2930e0 (type *0x37674c, fade-in words 0x529e30 +0x3ac, distance 0x24ef70); vehicle box 0x278b04 (0x2536c0, +0x6c,
// +0x74). Type 1 path 0x1b33fc (floor 0x252c14, signed remainder 0x24e500, unit 0x23b998).
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
    scene.world.blocked = [](PedestrianBody&, const PedestrianVec3&, PedestrianVec3&, float) { return 4; };
    PedestrianBody body = walking({0, 0, 10}, {0, 0, 20});
    body.pathParameter = 1.5f;
    PedestrianWalk::update(body, scene.world, random);
    require(!body.forward, "direction flipped");
    require(near(body.pathParameter, 0.75f), "parameter - 0.75");
    require(nearVec(body.target, {0.0f, 0.0f, 7.5f}), "target at the new parameter");
}

void testBlockedSlide() {
    Scene scene;
    GameRandom random(1);
    PedestrianVec3 seenPosition{}, seenVelocity{};
    float seenDt = 0;
    scene.world.blocked = [&](PedestrianBody&, const PedestrianVec3& position, PedestrianVec3& velocity, float dt) {
        seenPosition = position;
        seenVelocity = velocity;
        seenDt = dt;
        velocity = {0.05f, 0.0f, 0.0f};
        return 0;
    };
    PedestrianBody body = walking({0, 0, 10}, {0, 0, 20});
    PedestrianWalk::update(body, scene.world, random);
    require(nearVec(seenPosition, {0, 0, 10}) && nearVec(seenVelocity, {0.0f, 0.0f, 0.1f}) && near(seenDt, 1.0f / 30.0f), "blocked inputs");
    require(nearVec(body.node[3], {0.05f, 0.0f, 10.0f}), "the walk adds the slid velocity (0x2b93c0)");
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
    scene.world.blocked = [](PedestrianBody& walker, const PedestrianVec3&, PedestrianVec3&, float) {
        walker.requested = 4;
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

    level.type = 1;
    level.points = {{0, 0, 0}, {10, 0, 0}, {10, 0, 10}};
    const PedestrianPath linear = pedestrianLevelPath(level);
    linear.sample(0.5f, position, tangent);
    require(nearVec(position, {5, 0, 0}) && nearVec(tangent, {1, 0, 0}), "type 1 sample");
    linear.sample(1.25f, position, tangent);
    require(nearVec(position, {10, 0, 2.5f}) && nearVec(tangent, {0, 0, 1}), "type 1 second segment");
    linear.sample(-1.0f, position, tangent);
    require(nearVec(position, {0, 0, 0}) && nearVec(tangent, {0, 0, 1}), "open path clamps at 0, fraction 0 gives (0, 0, 1)");
    linear.sample(5.0f, position, tangent);
    require(nearVec(position, {10, 0, 10}) && nearVec(tangent, {0, 0, 1}), "open path clamps at count - 1");
    level.closed = true;
    const float half = std::sqrt(0.5f);
    pedestrianLinearPathSample(level, 2.5f, position, tangent);
    require(nearVec(position, {5, 0, 5}) && nearVec(tangent, {-half, 0, -half}), "closed path wraps the next point");
    pedestrianLinearPathSample(level, -0.5f, position, tangent);
    require(nearVec(position, {5, 0, 5}), "closed path wraps a negative parameter");

    level.type = 2;
    bool rejected = false;
    try {
        pedestrianLevelPath(level);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected, "a path type without a sampler is rejected");
}

Node quarterTurnAt(const PedestrianVec3& position) {
    return {PedestrianVec3{0, 0, -1}, PedestrianVec3{0, 1, 0}, PedestrianVec3{1, 0, 0}, position};
}

void testRootMotion() {
    for (std::uint16_t state : {3, 4, 7}) {
        Scene scene;
        GameRandom random(1);
        PedestrianBody body = pedestrianCreate(true, quarterTurnAt({10, 0, 5}));
        body.state = body.requested = state;
        int probes = 0;
        scene.world.groundProbe = [&](PedestrianVec3& position) {
            ++probes;
            position[1] = 1.0f;
        };
        PedestrianWalk::update(body, scene.world, random);
        require(nearVec(body.node[3], {10, 0, 5}) && probes == 0, "no root delta: no move and no probe");

        scene.world.rootTranslation = PedestrianVec3{0.0f, 0.2f, 0.5f};
        PedestrianVec3 slidFrom{}, slidMove{};
        float slidRadius = 0;
        scene.world.wallSlide = [&](const PedestrianVec3& position, const PedestrianVec3& move, float nearRadius) {
            slidFrom = position;
            slidMove = move;
            slidRadius = nearRadius;
            return PedestrianVec3{0.25f, 0.1f, -0.1f};
        };
        const PedestrianWalkUpdate out = PedestrianWalk::update(body, scene.world, random);
        require(nearVec(slidFrom, {10, 0, 5}) && nearVec(slidMove, {0.5f, 0.0f, 0.0f}), "root translation through the node rows, y dropped");
        require(near(slidRadius, 0.75f), "near radius entity +0x5c x 1.5");
        require(nearVec(body.node[3], {10.25f, 1.0f, 4.9f}) && probes == 1 && out.groundProbed, "slid x and z, then the probe");
        require(body.state == state && body.requested == state, "state kept while the clip plays");

        scene.world.wallSlide = nullptr;
        PedestrianWalk::update(body, scene.world, random);
        require(nearVec(body.node[3], {10.75f, 1.0f, 4.9f}), "no walls: the move in x and z");
    }
}

void testBumpSteady() {
    Scene scene;
    GameRandom random(1);
    scene.world.playerPosition = {1.0f, 0.0f, 0.0f};
    scene.world.playerState = 2;
    scene.world.playerInVehicle = true;
    int slides = 0;
    scene.world.wallSlide = [&](const PedestrianVec3&, const PedestrianVec3& move, float) {
        ++slides;
        return PedestrianVec3{move[0], 0.3f, move[2]};
    };
    int probes = 0;
    scene.world.groundProbe = [&](PedestrianVec3& position) {
        ++probes;
        position[1] = 0.0f;
    };
    PedestrianBody body = pedestrianCreate(true, identityAt({0, 0, 0}));
    body.state = body.requested = 5;
    PedestrianWalk::update(body, scene.world, random);
    require(slides == 1 && probes == 1, "wall test and probe on a touch, also from a vehicle");
    require(nearVec(body.node[3], {-0.2f, 0.3f, 0.0f}), "push applied by the wall test and again after the probe");

    scene.world.playerState = 0;
    PedestrianBody still = pedestrianCreate(true, identityAt({0, 0, 0}));
    still.state = still.requested = 5;
    PedestrianWalk::update(still, scene.world, random);
    require(slides == 2 && probes == 2 && nearVec(still.node[3], {0.0f, 0.3f, 0.0f}), "touch without a push moves by the slide only");

    scene.world.playerPosition = {5.0f, 0.0f, 0.0f};
    PedestrianBody apart = pedestrianCreate(true, identityAt({0, 0, 0}));
    apart.state = apart.requested = 5;
    PedestrianWalk::update(apart, scene.world, random);
    require(slides == 2 && probes == 3 && nearVec(apart.node[3], {0, 0, 0}), "no touch: only the probe");
}

void testEntityBox() {
    const PedestrianEntityBox box = pedestrianEntityBox({0.0f, 0.8f, 0.0f}, {0.5f, 0.8f, 0.3f});
    require(nearVec(box.centre, {0.0f, 0.8f, 0.0f}) && nearVec(box.half, {0.5f, 0.8f, 0.3f}), "box from the node bounds");
    require(near(box.radius, std::sqrt(0.98f)), "radius = length of the half extents");
}

void testGroundProbe() {
    PedestrianVec3 from{}, to{};
    std::optional<float> hit = 1.9f;
    auto ray = [&](const PedestrianVec3& a, const PedestrianVec3& b) {
        from = a;
        to = b;
        return hit;
    };
    PedestrianVec3 position{1, 2, 3};
    pedestrianGroundProbe(position, ray);
    require(nearVec(from, {1, 2.25f, 3}) && nearVec(to, {1, 1.75f, 3}), "probe 0.25 up and 0.25 down");
    require(nearVec(position, {1, 1.9f, 3}), "hit height");
    hit.reset();
    pedestrianGroundProbe(position, ray);
    require(nearVec(position, {1, 1.9f, 3}), "no hit keeps the height");
}

void testDrawTest() {
    PedestrianNode node;
    node.word = 5 | 0x200;
    node.fadeFlags = 0x10;
    node.radius = 2.0f;
    const DistanceFadeTable fade;
    require(pedestrianNodeDrawn(node, 5, fade, 35.9f * 35.9f), "inside the tier 0 fade-in distance");
    require(!pedestrianNodeDrawn(node, 5, fade, 36.0f * 36.0f), "at the fade-in distance");
    require(pedestrianNodeDrawn(node, 4, fade, 1e6f), "another node type");
    node.radius = 4.0f;
    require(pedestrianNodeDrawn(node, 5, fade, 71.0f * 71.0f) && !pedestrianNodeDrawn(node, 5, fade, 73.0f * 73.0f), "tier 1");
    node.word = 5;
    require(pedestrianNodeDrawn(node, 5, fade, 1e6f), "no distance fade bit");
    require(near(pedestrianDistanceSquared({1, 2, 3}, {4, 6, 3}), 25.0f), "squared distance");

    Scene scene;
    GameRandom random(1);
    PedestrianDrawTest test;
    test.node.word = 5 | 0x200;
    test.node.fadeFlags = 0x10;
    test.node.radius = 2.0f;
    test.meshNodeType = 5;
    test.fadePoint = {0.0f, 0.0f, 100.0f};
    scene.world.drawTest = test;
    PedestrianBody body = walking({0, 0, 0}, {0, 0, 20});
    body.hold = 3;
    PedestrianWalk::update(body, scene.world, random);
    require(body.hold == 2, "beyond the fade-in distance counts down although `visible` is set");
    scene.world.drawTest->fadePoint = {0.0f, 0.0f, 10.0f};
    PedestrianWalk::update(body, scene.world, random);
    require(body.hold == 10, "drawn keeps at least 10");
    scene.world.drawTest->node.word |= 1u << 18;
    PedestrianWalk::update(body, scene.world, random);
    require(body.hold == 9, "a hidden node counts down");
}

void testUnderVehicle() {
    PedestrianVehicleBox vehicle;
    vehicle.node = quarterTurnAt({10, 0, 10});
    vehicle.halfX = 1.0f;
    vehicle.halfZ = 2.5f;
    require(pedestrianUnderVehicle(vehicle, {12.0f, 3.0f, 10.5f}), "inside the box in x and z of the vehicle frame");
    require(!pedestrianUnderVehicle(vehicle, {12.6f, 0.0f, 10.0f}), "beyond the half z");
    require(!pedestrianUnderVehicle(vehicle, {10.0f, 0.0f, 11.0f}), "at the half x");

    Scene scene;
    scene.world.dt = 1.0f;
    GameRandom random(1);
    PedestrianBody body = pedestrianCreate(true, identityAt({12.0f, 0.0f, 10.5f}));
    body.requested = 6;
    PedestrianWalk::update(body, scene.world, random);
    scene.world.splatRefresh = true;
    scene.world.playerVehicleBox = vehicle;
    PedestrianWalk::update(body, scene.world, random);
    require(near(body.splatTimer, 2.0f), "under the vehicle sets 3.0 again");
    body.node[3] = {20.0f, 0.0f, 10.0f};
    PedestrianWalk::update(body, scene.world, random);
    require(near(body.splatTimer, 1.0f), "the vehicle box replaces splatRefresh");
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
        testBlockedSlide();
        testAvoidance();
        testBump();
        testDodgeFacesPlayer();
        testSplat();
        testHold();
        testTilt();
        testLookAt();
        testLevelPath();
        testRootMotion();
        testBumpSteady();
        testEntityBox();
        testGroundProbe();
        testDrawTest();
        testUnderVehicle();
    } catch (const std::exception& error) {
        std::cerr << "pedestrian-walk: " << error.what() << "\n";
        return 1;
    }
    std::cout << "pedestrian-walk: ok\n";
    return 0;
}
