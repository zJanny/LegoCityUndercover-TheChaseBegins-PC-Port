// The climbing bricks (src/runtime/brick_grab.cpp) on made-up bricks, no game data needed. Every expected value is a hand evaluation of the
// rules in docs/re/player-traversal.md (grab test 0x273fe4, enters 0x32c4b8 / 0x32c5b8 / 0x32c6fc, events 0x31a408, 0x31a49c, 0x31c430,
// 0x31c488, 0x3233e8, brick update 0x1a649c) on the brick A (0, 10, 0), B 2 above it and C 1 to its right, all facing +z (out of the wall).
#include "runtime/brick_grab.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b, float tolerance) { return std::fabs(a - b) <= tolerance; }

GrabBrick brick(const char* name, float x, float y, float z) {
    GrabBrick result;
    result.instance = name;
    result.matrix = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, x, y, z, 1};
    result.adjacent.fill(-1);
    result.hasGrabBound = true;
    // Class bound GrabBound of props/parkour_brickgrab_new: centre (-0.0115, 0, 0.3857), half (0.607, 0.2, 0.3) (class file dump).
    const float centre[3] = {-0.0115f, 0.0f, 0.3857f}, half[3] = {0.607f, 0.2f, 0.3f};
    for (int a = 0; a < 3; ++a) {
        result.grabCentre[a] = centre[a];
        result.grabHalf[a] = half[a];
    }
    return result;
}

std::vector<GrabBrick> wall() {
    std::vector<GrabBrick> bricks = {brick("a", 0, 10, 0), brick("b", 0, 12, 0), brick("c", 1, 10, 0)};
    bricks[0].adjacent[0] = 1;
    bricks[0].adjacent[1] = 2;
    return bricks;
}

// A jump clip whose root rises 1 between frames 1 and 2 (the brickgrab clips do not move in their first frame) and moves -1 in x
// (as brickgrab_up / brickgrab_left) and +1 in z.
const chase::assets::RootMotion& jumpMotion() {
    static const chase::assets::RootMotion motion = {{{0, 0, 0}, {0, 0, 0}, {-1, 1, 1}}, {-1, 0, 0}, {0, 1, 1}};
    return motion;
}

BrickInput input() {
    BrickInput result;
    for (const int role : {kAnimationBrickGrab, kAnimationBrickGrabUp, kAnimationBrickGrabLeft, kAnimationBrickGrabRight})
        result.clips[role] = {2, nullptr, 15.0f};
    for (const int role : {kAnimationBrickJumpUp, kAnimationBrickJumpLeft, kAnimationBrickJumpRight, kAnimationBrickTopJump})
        result.clips[role] = {4, &jumpMotion(), 15.0f};
    return result;
}

constexpr std::int16_t kFacingWall = -32768;  // heading pi: forward (0, 0, -1), into the wall

void grabTest() {
    std::vector<GrabBrick> bricks = wall();
    // Box centre = player + (0, 1.15 * 1.75, 0.08), half (0.6, 0.85 * 0.25, 0.52); the GrabBound centre is (-0.0115, 10, 0.3857).
    const float hanging[3] = {0.0f, 10.0f - 2.0125f, 0.3f};
    require(brickGrabTest(bricks, hanging, kFacingWall, {}) == 0, "box centre level with brick A: A is grabbed");
    const float low[3] = {0.0f, 10.0f - 2.0125f - 0.42f, 0.3f};
    require(brickGrabTest(bricks, low, kFacingWall, {}) == -1, "0.42 below: the y gap exceeds 0.2125 + 0.2");
    require(brickGrabTest(bricks, hanging, 0, {}) == -1, "facing along the brick forward (dot 1 > 0.75) fails");
    bricks[0].cooldown = 1;
    require(brickGrabTest(bricks, hanging, kFacingWall, {}) == 2, "a brick in its cooldown is skipped: C, 1 to the side, overlaps too");
    bricks[0].cooldown = 0;
    require(brickGrabTest(bricks, hanging, kFacingWall, [](const GrabBrick& b) { return b.instance == "b"; }) == -1, "disabled bricks are not grabbed");
}

void jumpCounter() {
    BrickClimber climber;
    require(!brickJumpTick(climber, true, false), "take-off: counter 0");
    int first = 0;
    for (int update = 2; update <= 8 && first == 0; ++update)
        if (brickJumpTick(climber, false, true)) first = update;
    require(first == 7, "the grab test runs in the 7th jump update, got " + std::to_string(first));
    require(!brickJumpTick(climber, false, false) && climber.jumpCount == -1, "leaving state 4 resets the counter");
}

void grabAndIdle() {
    std::vector<GrabBrick> bricks = wall();
    BrickClimber climber;
    BrickInput in = input();
    BrickStep step = brickGrabEnter(climber, bricks, 0);
    require(climber.state == BrickState::Grab && step.entered == BrickState::Grab && bricks[0].held, "state 141, brick held");
    require(near(step.position[0], 0, 0) && near(step.position[1], 8.0f, 0) && near(step.position[2], 0, 0), "snap: 2 below the brick");
    require(std::abs(int(step.facing)) >= 32767, "facing turned by pi from the brick: into the wall");
    std::copy(step.position, step.position + 3, in.position);
    in.facing = step.facing;
    step = brickGrabUpdate(climber, bricks, in);
    require(climber.state == BrickState::Grab && step.animation == kAnimationBrickGrab, "the grab clip runs");
    step = brickGrabUpdate(climber, bricks, in);
    require(climber.state == BrickState::Idle && step.entered == BrickState::Idle, "clip done (2 updates): state 145");
    in.stickHeld = true;
    in.stickHeading = kFacingWall;
    step = brickGrabUpdate(climber, bricks, in);
    require(climber.state == BrickState::ReachUp && step.animation == kAnimationBrickReachUp, "stick into the wall: reach up");
    in.stickHeading = 16384;  // +x: brick right
    step = brickGrabUpdate(climber, bricks, in);
    require(climber.state == BrickState::ReachLeft, "stick along the brick's right row: reach left (0x31c488)");
    in.stickHeld = false;
    step = brickGrabUpdate(climber, bricks, in);
    require(climber.state == BrickState::Idle, "stick released: idle (0x32c6bc)");
    updateGrabBricks(bricks);
    require(bricks[0].cooldown == kBrickReleaseCooldown, "cooldown 15 while held");
}

void jumpUp() {
    std::vector<GrabBrick> bricks = wall();
    BrickClimber climber;
    BrickInput in = input();
    BrickStep step = brickGrabEnter(climber, bricks, 0);
    std::copy(step.position, step.position + 3, in.position);
    in.facing = step.facing;
    in.stickHeld = true;
    in.stickHeading = kFacingWall;
    in.jumpPressed = true;
    step = brickGrabUpdate(climber, bricks, in);
    require(climber.state == BrickState::JumpUp && climber.target == 1 && step.entered == BrickState::JumpUp, "B into the wall: jump up to B");
    require(!bricks[0].held, "the released event clears A");
    require(near(climber.scale[1], 2.0f, 1e-6f) && climber.scale[0] == 0.0f, "y scale = 2 (way) / 1 (extent)");
    require(near(step.position[1], 8.0f, 0), "no move in the entry update");
    in.jumpPressed = false;
    const float expected[3] = {8.0f, 8.0f, 9.0f};  // frames 0.5, 1, 1.5
    for (const float y : expected) {
        step = brickGrabUpdate(climber, bricks, in);
        require(near(step.position[1], y, 1e-5f), "jump height " + std::to_string(step.position[1]));
    }
    step = brickGrabUpdate(climber, bricks, in);
    require(climber.state == BrickState::GrabUp && climber.brick == 1 && bricks[1].held, "clip done: grab up on B");
    require(near(step.position[1], 10.0f, 0), "snap 2 below B");
}

void jumpSideways() {
    std::vector<GrabBrick> bricks = wall();
    BrickClimber climber;
    BrickInput in = input();
    BrickStep step = brickGrabEnter(climber, bricks, 0);
    std::copy(step.position, step.position + 3, in.position);
    in.facing = step.facing;
    in.stickHeld = true;
    in.stickHeading = 16384;
    in.jumpPressed = true;
    step = brickGrabUpdate(climber, bricks, in);
    require(climber.state == BrickState::JumpLeft && climber.target == 2, "B towards +x: C lies along the target's right row, jump left");
    require(near(climber.scale[0], 1.0f, 1e-6f) && climber.scale[1] == 0.0f && climber.scale[2] == 0.0f, "x scale = 1 / 1");
    in.jumpPressed = false;
    for (int update = 0; update < 3; ++update) step = brickGrabUpdate(climber, bricks, in);
    require(near(step.position[0], 0.5f, 1e-5f), "half of the way at frame 1.5: -scale x * root x along the brick right");
}

void exits() {
    std::vector<GrabBrick> bricks = wall();
    BrickClimber climber;
    BrickInput in = input();
    BrickStep step = brickGrabEnter(climber, bricks, 0);
    std::copy(step.position, step.position + 3, in.position);
    in.usePressed = true;
    step = brickGrabUpdate(climber, bricks, in);
    require(!climber.active() && step.exit == BrickExit::Fall && step.fallSpeed == 0.0f && !bricks[0].held, "A: state 5, fall speed 0");

    step = brickGrabEnter(climber, bricks, 1);
    in.usePressed = false;
    in.jumpPressed = true;
    in.stickHeld = true;
    in.stickHeading = kFacingWall;
    step = brickGrabUpdate(climber, bricks, in);
    require(!climber.active() && step.exit == BrickExit::Jump, "B on a brick without adjacent bricks: state 4");

    step = brickGrabEnter(climber, bricks, 0);
    in.jumpPressed = false;
    step = brickGrabUpdate(climber, bricks, [&]() {
        BrickInput disabled = in;
        disabled.enabled = [](const GrabBrick& b) { return b.instance != "a"; };
        return disabled;
    }());
    require(!climber.active() && step.exit == BrickExit::Fall, "check event 3: a disabled brick drops the player");

    bricks[0].topBrick = true;
    step = brickGrabEnter(climber, bricks, 0);
    std::copy(step.position, step.position + 3, in.position);
    in.jumpPressed = true;
    step = brickGrabUpdate(climber, bricks, in);
    require(climber.state == BrickState::TopJump && step.animation == kAnimationBrickTopJump, "IsTopBrick: state 152");
    in.jumpPressed = false;
    for (int update = 0; update < 3; ++update) step = brickGrabUpdate(climber, bricks, in);
    require(near(step.position[1], 8.5f, 1e-5f), "state 152 moves by the unscaled root track");
    step = brickGrabUpdate(climber, bricks, in);
    require(!climber.active() && step.exit == BrickExit::Fall && near(step.fallSpeed, -0.5f, 1e-5f), "clip end: state 5 with the last height change");
}

void cooldown() {
    std::vector<GrabBrick> bricks = wall();
    bricks[0].held = true;
    updateGrabBricks(bricks);
    bricks[0].held = false;
    for (int update = 0; update < kBrickReleaseCooldown; ++update) updateGrabBricks(bricks);
    require(bricks[0].cooldown == 0, "15 updates after the release the brick is free again");
}

}

int main() {
    try {
        grabTest();
        jumpCounter();
        grabAndIdle();
        jumpUp();
        jumpSideways();
        exits();
        cooldown();
    } catch (const std::exception& error) {
        std::cerr << "brick-grab-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "brick-grab-test: ok\n";
    return 0;
}
