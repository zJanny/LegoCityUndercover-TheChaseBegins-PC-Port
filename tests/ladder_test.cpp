// Ladders (src/runtime/ladder.cpp) with a made-up ladder: the handler 0x18b458, the zone test 0x221570 and the state updates 0x325380,
// 0x1e43c0, 0x32ca18, 0x32f83c and 0x302e94 with hand-computed values (the recorded oracle runs are in ladder-real-test).
#include "runtime/ladder.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
bool near(float a, float b, float tolerance) { return std::fabs(a - b) <= tolerance; }

LadderObject makeLadder() {
    LadderObject ladder;
    ladder.instance = "test_ladder";
    ladder.snapOffset = 0.45f;
    ladder.collisionCentre[1] = 3.0f;
    ladder.collisionHalf[0] = 0.8f;
    ladder.collisionHalf[1] = 3.0f;
    ladder.collisionHalf[2] = 0.25f;
    ladder.hasUseBound = true;
    ladder.useCentre[1] = 3.0f;
    ladder.useCentre[2] = 0.54f;
    ladder.useHalf[0] = ladder.useHalf[2] = 0.8f;
    ladder.useHalf[1] = 3.2f;
    return ladder;
}

LadderInput inputAt(float x, float y, float z) {
    LadderInput input;
    input.position[0] = x;
    input.position[1] = y;
    input.position[2] = z;
    input.clamberRise = 1.8325f;
    return input;
}

void follow(LadderInput& input, const LadderStep& step) {
    for (int a = 0; a < 3; ++a) input.position[a] = step.position[a];
    input.facing = step.facing;
}
}

int main() {
    try {
        std::vector<LadderObject> ladders = {makeLadder()};
        const auto none = [](const LadderObject&) { return false; };
        // Zone: the UseBound reaches 1.34 in front of the ladder (centre z 0.54 + half 0.8); FaceTowards 0 has no facing test.
        const float inFront[3] = {0, 0, 1.2f}, tooFar[3] = {0, 0, 1.4f};
        require(ladderZoneHolds(ladders[0], inFront, 0), "zone holds 1.2 in front, facing away (FaceTowards 0)");
        require(!ladderZoneHolds(ladders[0], tooFar, 0), "zone fails 1.4 in front");
        ladders[0].faceTowards = 1;
        require(!ladderZoneHolds(ladders[0], inFront, 0) && ladderZoneHolds(ladders[0], inFront, std::int16_t(0x8000)), "FaceTowards 1: face the ladder");
        ladders[0].faceTowards = 0;
        // Handler: a player above top (6) - clamber rise (1.8325) is refused; a furled ladder answers 0.
        const float high[3] = {0, 4.2f, 1.0f}, low[3] = {0, 4.1f, 1.0f};
        require(!ladderAccepts(ladders[0], high, 1.8325f) && ladderAccepts(ladders[0], low, 1.8325f), "top minus the clamber rise");
        require(ladderUseTest(ladders, inFront, 0, 1.8325f, none) == 0, "use press in the zone");
        require(ladderUseTest(ladders, inFront, 0, 1.8325f, [](const LadderObject&) { return true; }) == -1, "a disabled ladder is not used");
        ladders[0].furled = true;
        require(ladderUseTest(ladders, inFront, 0, 1.8325f, none) == -1, "a furled ladder answers 0");
        ladders[0].furled = false;
        // Walk-in: the player's box reaches 0.6 in front of the node, the collision box 0.25: touch at 0.8, not at 0.9.
        const float close[3] = {0, 0, 0.8f}, apart[3] = {0, 0, 0.9f};
        require(ladderWalkInTest(ladders, close, std::int16_t(0x8000), 1.8325f, none) == 0, "walk-in at 0.8");
        require(ladderWalkInTest(ladders, apart, std::int16_t(0x8000), 1.8325f, none) == -1, "no walk-in at 0.9");

        // Mount by the A press: 4 updates of 0.25 (dt x 30 x 0.25 at dt 1/30) to the snap point 0.45 in front, facing kept until the end.
        LadderClimber climber;
        LadderInput input = inputAt(0.0f, 2.0f, 1.2f);
        input.facing = 1000;
        LadderStep step = ladderMount(climber, ladders, 0, input, true);
        require(step.riding && near(step.position[2], 1.2f - 0.25f * 0.75f, 1e-5f) && step.facing == 1000, "mount update 1 in the press update");
        follow(input, step);
        for (int i = 0; i < 3; ++i) {
            step = ladderUpdate(climber, ladders, input);
            follow(input, step);
        }
        require(climber.phase == LadderPhase::Idle && near(step.position[2], 0.45f, 1e-6f) && near(climber.height, 2.0f, 1e-6f), "idle at the snap point");
        require(step.facing == std::int16_t(0x8000), "facing the ladder (heading + 0x8000)");
        // Walk-in mount: the facing turns to the ladder at once.
        LadderClimber walker;
        LadderInput walking = inputAt(0.0f, 2.0f, 0.8f);
        require(ladderMount(walker, ladders, 0, walking, false).facing == std::int16_t(0x8000), "walk-in faces the ladder");

        // Idle: the stick up (quadrant 0 with camera yaw 0) asks for the climb in this update, the step comes in the next: 0.05 per update.
        input.stickActive = true;
        input.stickHeading = 0;
        step = ladderUpdate(climber, ladders, input);
        require(climber.phase == LadderPhase::Up && near(step.position[1], 2.0f, 1e-6f), "climb asked, no move");
        follow(input, step);
        step = ladderUpdate(climber, ladders, input);
        require(near(step.position[1], 2.05f, 1e-5f) && step.animation == kAnimationLadderUp, "0.05 up");
        follow(input, step);
        // Sideways stick (quadrant 1) keeps the current direction.
        input.stickHeading = 0x4000;
        step = ladderUpdate(climber, ladders, input);
        require(climber.phase == LadderPhase::Up && near(step.position[1], 2.1f, 1e-5f), "sideways keeps climbing");
        follow(input, step);
        input.stickHeading = 0;
        // Top: 6 <= height + 1.8325 at height 4.1675: the clamber starts in that update without the step being applied.
        while (climber.phase == LadderPhase::Up) {
            step = ladderUpdate(climber, ladders, input);
            follow(input, step);
        }
        require(climber.phase == LadderPhase::Clamber && climber.height + 1.8325f >= 6.0f && climber.height + 1.8325f < 6.05f, "clamber at the top");
        require(near(input.position[1], climber.height - 0.05f, 1e-4f), "the top update does not move");
        // Clamber with a made-up root track (1 up per frame at 15 frames per second, 4 frames): no move in its first update, then 0.5 per update.
        chase::assets::RootMotion rise;
        for (int f = 0; f <= 4; ++f) rise.translation.push_back({0.0f, float(f), 0.0f});
        input.clamber = &rise;
        input.clamberFrames = 4;
        const float before = input.position[1];
        step = ladderUpdate(climber, ladders, input);
        require(near(step.position[1], before, 1e-6f), "clamber update 1 does not move");
        follow(input, step);
        step = ladderUpdate(climber, ladders, input);
        require(near(step.position[1], before + 0.5f, 1e-5f), "clamber update 2 moves half a frame");
        follow(input, step);
        int updates = 2;
        while (step.exit == LadderExit::None) {
            step = ladderUpdate(climber, ladders, input);
            follow(input, step);
            ++updates;
        }
        require(step.exit == LadderExit::Ground && updates == 9 && near(step.position[1], before + 4.0f, 1e-5f), "clip end after 9 updates");
        require(!climber.active(), "off the ladder");

        // Without CanDismount the top blocks: the step is undone and Idle (Blocked) follows; up stays blocked, down climbs.
        ladders[0].canDismount = false;
        LadderClimber blocked;
        LadderInput onTop = inputAt(0.0f, 4.15f, 0.45f);
        onTop.stickActive = true;
        blocked.phase = LadderPhase::Up;
        blocked.ladder = 0;
        blocked.height = 4.15f;
        step = ladderUpdate(blocked, ladders, onTop);
        require(blocked.phase == LadderPhase::Blocked && near(step.position[1], 4.15f, 1e-6f) && near(blocked.height, 4.15f, 1e-6f), "blocked at the top");
        step = ladderUpdate(blocked, ladders, onTop);
        require(blocked.phase == LadderPhase::Blocked, "up stays blocked");
        onTop.stickHeading = std::int16_t(0x8000);
        ladderUpdate(blocked, ladders, onTop);
        require(blocked.phase == LadderPhase::Down, "down leaves the blocked idle");
        ladders[0].canDismount = true;

        // Climb down: below -0.85 (the player's half height) the player falls; a floor within the step below him ends it first.
        LadderClimber down;
        down.phase = LadderPhase::Down;
        down.ladder = 0;
        down.height = -0.79f;
        LadderInput below = inputAt(0.0f, -0.79f, 0.45f);
        below.stickActive = true;
        below.stickHeading = std::int16_t(0x8000);
        step = ladderUpdate(down, ladders, below);
        require(step.exit == LadderExit::None && near(step.position[1], -0.84f, 1e-5f), "down to -0.84");
        follow(below, step);
        step = ladderUpdate(down, ladders, below);
        require(step.exit == LadderExit::Fall && near(step.position[1], -0.84f, 1e-5f), "fall below -0.85 without moving");
        LadderClimber floored;
        floored.phase = LadderPhase::Down;
        floored.ladder = 0;
        floored.height = 1.0f;
        LadderInput onFloor = inputAt(0.0f, 1.0f, 0.45f);
        onFloor.stickActive = true;
        onFloor.stickHeading = std::int16_t(0x8000);
        onFloor.floorBelow = [](const float*, float length) { return length > 0.0f; };
        require(ladderUpdate(floored, ladders, onFloor).exit == LadderExit::Fall, "floor below ends the climb down");
        // 0x32f83c casts the ray also with the stick released (step 0): it reaches 0.1 below the node.
        LadderClimber resting;
        resting.phase = LadderPhase::Down;
        resting.ladder = 0;
        resting.height = 1.0f;
        LadderInput released = inputAt(0.0f, 1.0f, 0.45f);
        released.floorBelow = [](const float position[3], float length) { return position[1] - length <= 0.95f; };
        require(ladderUpdate(resting, ladders, released).exit == LadderExit::Fall, "a floor within 0.1 ends the climb down at step 0");

        // A and B leave from the idle and the climb (events 7 and 6), not from the mount.
        LadderClimber idle;
        idle.phase = LadderPhase::Idle;
        idle.ladder = 0;
        LadderInput press = inputAt(0.0f, 2.0f, 0.45f);
        press.aPressed = true;
        const LadderStep drop = ladderUpdate(idle, ladders, press);
        require(drop.exit == LadderExit::Fall && drop.facing == std::int16_t(-0x3000), "A drops, turned by -0x3000");
        require(drop.targetHeading == std::int16_t(0x8000), "the target heading turns to the ladder heading (ctrl+0xa + 0x8000)");
        idle.phase = LadderPhase::Up;
        idle.ladder = 0;
        press.aPressed = false;
        press.bPressed = true;
        require(ladderUpdate(idle, ladders, press).exit == LadderExit::Jump, "B jumps");
        LadderClimber mounting;
        mounting.phase = LadderPhase::Mount;
        mounting.ladder = 0;
        require(ladderUpdate(mounting, ladders, press).exit == LadderExit::None, "no B during the mount");
    } catch (const std::exception& error) {
        std::cerr << "ladder-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "ladder-test: ok\n";
    return 0;
}
