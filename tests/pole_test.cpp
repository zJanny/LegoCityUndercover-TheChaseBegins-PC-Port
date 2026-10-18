// Poles (src/runtime/pole.cpp) with a made-up pole: the rules of the update functions 0x28d908, 0x283f18, 0x1d579c and the gates 0x206510 /
// 0x221570, with hand-computed values (the recorded oracle replays are in pole-real-test).
#include "runtime/pole.hpp"
#include <algorithm>
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

PoleObject makePole(bool canDismount) {
    PoleObject pole;
    pole.instance = "test_pole";
    pole.forward[2] = 1;  // yaw 0: forward +z, right +x
    pole.heading = 0;
    pole.snapOffset = 0.1f;
    pole.canDismount = canDismount;
    pole.collisionCentre[1] = 5.0f;
    pole.collisionHalf[0] = pole.collisionHalf[2] = 0.15f;
    pole.collisionHalf[1] = 5.0f;
    pole.useCentre[1] = 4.0f;
    pole.useCentre[2] = 0.5f;
    pole.useHalf[0] = 1.0f;
    pole.useHalf[1] = 4.0f;
    pole.useHalf[2] = 1.0f;
    return pole;
}
}

int main() {
    try {
        std::vector<PoleObject> poles = {makePole(false)};
        // Zone: in front of the pole (z +0.5) and facing it (facing 0x8000 = -z); behind it or facing away fails (FaceTowards 1).
        const float inFront[3] = {0, 1, 0.8f};
        require(poleZoneHolds(poles[0], inFront, std::int16_t(0x8000)), "zone holds in front, facing the pole");
        require(!poleZoneHolds(poles[0], inFront, 0), "zone fails facing away");
        const float behind[3] = {0, 1, -0.8f};
        require(!poleZoneHolds(poles[0], behind, std::int16_t(0x8000)), "zone fails behind the pole (outside the UseBound)");
        // Touch: the player's box reaches 0.6 in front of the node (centre z 0.08 + half 0.52): at 0.7 it touches a pole of half depth 0.15.
        const float close[3] = {0, 1, 0.7f}, far[3] = {0, 1, 0.9f};
        require(poleTouched(poles[0], close, std::int16_t(0x8000)), "touch at 0.7");
        require(!poleTouched(poles[0], far, std::int16_t(0x8000)), "no touch at 0.9");
        const auto none = [](const PoleObject&) { return false; };
        require(poleWalkInTest(poles, close, std::int16_t(0x8000), true, std::int16_t(0x8000), none) == 0, "walk-in");
        require(poleWalkInTest(poles, close, std::int16_t(0x8000), true, 0, none) == -1, "a stick pointing away from the pole does not walk in (cone 0x4000)");
        require(poleWalkInTest(poles, close, std::int16_t(0x8000), false, 0, none) == 0, "no stick: no cone test");
        require(poleUseTest(poles, inFront, std::int16_t(0x8000), none) == 0, "use press in the zone");

        // Mount: 5 updates at dt 1/30 (blend 6.6666665 dt = 0.2222 per update), then idle at the snap point 0.1 in front of the pole.
        PoleClimber climber;
        PoleInput input;
        input.position[1] = 2.0f;
        input.position[2] = 0.7f;
        input.facing = std::int16_t(0x8000);
        PoleStep step = poleMount(climber, poles, 0, input, false);
        require(climber.phase == PolePhase::Mount && step.riding, "mounting");
        for (int i = 0; i < 4; ++i) {
            std::copy_n(step.position, 3, input.position);
            step = poleUpdate(climber, poles, input);
            require(climber.phase == PolePhase::Mount, "still mounting");
        }
        std::copy_n(step.position, 3, input.position);
        step = poleUpdate(climber, poles, input);
        require(climber.phase == PolePhase::Idle, "idle after the fifth update");
        require(near(step.position[2], 0.1f, 1e-6f) && near(step.position[1], 2.0f, 1e-6f), "snap point");
        require(near(climber.height, 2.0f, 1e-6f), "ctrl+0x24c = player y - pole y");

        // Climb without a root track: 0.05 per update; the stick must lie in the up quadrant of the camera yaw 0.
        std::copy_n(step.position, 3, input.position);
        input.stickActive = true;
        input.stickHeading = 0;
        step = poleUpdate(climber, poles, input);
        require(climber.phase == PolePhase::Up, "idle -> up");
        std::copy_n(step.position, 3, input.position);
        step = poleUpdate(climber, poles, input);
        require(near(step.position[1], 2.05f, 1e-6f) && near(climber.height, 2.05f, 1e-6f), "default step 0.05");
        // Stick released: idle. Stick pulled back: down with -0.05.
        input.stickActive = false;
        std::copy_n(step.position, 3, input.position);
        step = poleUpdate(climber, poles, input);
        require(climber.phase == PolePhase::Idle && near(step.position[1], 2.05f, 1e-6f), "no stick -> idle");
        input.stickActive = true;
        input.stickHeading = std::int16_t(0x8000);
        step = poleUpdate(climber, poles, input);
        require(climber.phase == PolePhase::Down, "idle -> down");
        std::copy_n(step.position, 3, input.position);
        step = poleUpdate(climber, poles, input);
        require(near(step.position[1], 2.0f, 1e-6f), "down step -0.05");

        // Top without CanDismount: the step is undone and the player goes idle (0x1d579c); the head reach decides: top 10 - 1.8325.
        climber.phase = PolePhase::Up;
        climber.height = 10.0f - kPoleReach - 0.04f;
        input.stickHeading = 0;
        input.position[1] = 8.0f;
        step = poleUpdate(climber, poles, input);
        require(climber.phase == PolePhase::Idle && near(climber.height, 10.0f - kPoleReach - 0.04f, 1e-5f), "pole without CanDismount stops at the top");

        // Top with CanDismount: the dismount starts in the same update and keeps the step in ctrl+0x24c.
        poles[0].canDismount = true;
        climber.phase = PolePhase::Up;
        climber.height = 10.0f - kPoleReach - 0.04f;
        step = poleUpdate(climber, poles, input);
        require(climber.phase == PolePhase::Dismount && near(climber.height, 10.0f - kPoleReach + 0.01f, 1e-5f), "dismount at the top");

        // Below the base the player falls, facing turned by pi in ctrl+0xa only; an A press above 5 drops him; B jumps.
        climber = PoleClimber();
        climber.phase = PolePhase::Down;
        climber.pole = 0;
        climber.height = 0.02f;
        step = poleUpdate(climber, poles, input);
        require(step.exit == PoleExit::Fall && !climber.active(), "fall below the base");
        climber = PoleClimber();
        climber.phase = PolePhase::Up;
        climber.pole = 0;
        climber.height = 5.5f;
        input.aPressed = true;
        step = poleUpdate(climber, poles, input);
        require(step.exit == PoleExit::Fall, "A press above 5.0 drops the player");
        climber = PoleClimber();
        climber.phase = PolePhase::Up;
        climber.pole = 0;
        climber.height = 4.5f;
        input.aPressed = false;
        input.bPressed = true;
        step = poleUpdate(climber, poles, input);
        require(step.exit == PoleExit::Jump, "B press jumps");

        // The looped step: root(end) - root(start) across the loop point equals the per-loop increment.
        chase::assets::RootMotion motion;
        for (int i = 0; i <= 4; ++i) motion.translation.push_back({0, 0.5f * float(i), 0});
        const float across = poleClimbStep(motion, 4, 7, 1.0f / 30.0f, 15.0f);  // frames 3.5 -> 4.0
        require(near(across, 0.25f, 1e-6f), "step before the loop point");
        const float after = poleClimbStep(motion, 4, 8, 1.0f / 30.0f, 15.0f);  // frames 4.0 -> 4.5 (loop frame 0 -> 0.5)
        require(near(after, 0.25f, 1e-6f), "step after the loop point");
        std::cout << "pole-test: ok\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "pole-test: " << error.what() << '\n';
        return 1;
    }
}
