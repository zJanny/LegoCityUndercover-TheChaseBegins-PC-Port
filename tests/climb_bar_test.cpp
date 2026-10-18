// Climb bar rules (src/runtime/climb_bar.cpp) on a made-up bar and corner rail; hand values from the formulas read in the original
// (docs/re/player-traversal.md): oriented box overlap 0x202504, Catmull-Rom spline 0x1c17c0, hang update 0x2a1c54, arrival 0x320ae4.
#include "runtime/climb_bar.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool near(float a, float b, float tolerance = 1e-4f) { return std::fabs(a - b) <= tolerance; }

ClimbBarSet madeUpSet() {
    ClimbBarSet set;
    ClimbBar bar;
    bar.instance = "bar";
    bar.position[1] = 10.0f;
    bar.yawDegrees = 0.0f;
    bar.grabCentre[1] = 0.1f;
    bar.grabHalf[0] = 2.0f;
    bar.grabHalf[1] = 0.2f;
    bar.grabHalf[2] = 0.3f;
    bar.hasGrabBound = true;
    set.bars.push_back(bar);
    return set;
}

// The hang line of the made-up bar: x from -2 to 2 at y 10.1 + hang offset y, z hang offset z.
float hangY() { return 10.0f + 0.1f + kClimbBarHangOffsetY; }

ClimbBarInput stickInput(float x, float heading) {
    ClimbBarInput input;
    input.position[0] = x;
    input.position[1] = hangY();
    input.position[2] = kClimbBarHangOffsetZ;
    input.stickHeld = true;
    input.stickHeading = std::int16_t(std::lround(heading * 10430.378f));
    static const chase::assets::RootMotion left{{{0,0,0},{-0.09f*2,0,0},{-0.18f*2,0,0}}, {}, {}};
    input.leftMotion = &left;
    static const chase::assets::RootMotion right{{{0,0,0},{0.18f,0,0},{0.36f,0,0}}, {}, {}};
    input.rightMotion = &right;
    input.clipFramesPerSecond = 15.0f;
    return input;
}
}

int main() {
    try {
        {
            const float a[3] = {1, 1, 1}, b[3] = {1, 1, 1};
            const float identity[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
            const float near1[3] = {1.9f, 0, 0}, far1[3] = {2.1f, 0, 0};
            require(obbOverlap(a, near1, b, identity), "boxes 1.9 apart overlap");
            require(!obbOverlap(a, far1, b, identity), "boxes 2.1 apart do not overlap");
            const float c = std::cos(0.785398f), s = std::sin(0.785398f);
            const float turned[3][3] = {{c, 0, -s}, {0, 1, 0}, {s, 0, c}};
            const float diagonal[3] = {2.3f, 0, 0};
            require(obbOverlap(a, diagonal, b, turned), "a box turned by 45 degrees reaches 2.414");
            const float beyond[3] = {2.5f, 0, 0};
            require(!obbOverlap(a, beyond, b, turned), "and not 2.5");
        }
        {
            const std::vector<std::array<float, 3>> points = {{0, 0, 0}, {1, 0, 1}, {3, 0, 1}};
            const std::array<float, 3> mid = splinePoint(points, 1.0f), end = splinePoint(points, 5.0f), start = splinePoint(points, -1.0f);
            require(near(mid[0], 1.0f) && near(mid[2], 1.0f), "spline passes through the second point");
            require(near(end[0], 3.0f) && near(start[0], 0.0f), "spline is held at both ends");
        }
        {
            const ClimbBarSet set = madeUpSet();
            ClimbBarRider rider;
            rider.phase = ClimbPhase::Idle;
            rider.bar = 0;
            // Stick along +x of the bar (heading pi/2 points to +x): left. The first update has no root motion: 0.065 along +x.
            ClimbBarInput input = stickInput(0.0f, 1.5707964f);
            ClimbBarStep step = climbBarUpdate(rider, set, input);
            require(step.riding && rider.phase == ClimbPhase::Left, "stick along +x starts LEFT");
            require(near(step.position[0], 0.065f) && near(step.position[1], hangY()), "first update moves 0.065 onto the hang line");
            input.position[0] = step.position[0];
            step = climbBarUpdate(rider, set, input);
            require(near(step.position[0], 0.065f + 0.09f), "then the clip step");
            // Stick the other way switches to RIGHT, no stick back to IDLE.
            input = stickInput(step.position[0], -1.5707964f);
            step = climbBarUpdate(rider, set, input);
            require(rider.phase == ClimbPhase::Right, "stick along -x switches to RIGHT");
            input.stickHeld = false;
            input.position[0] = step.position[0];
            step = climbBarUpdate(rider, set, input);
            require(rider.phase == ClimbPhase::Idle, "no stick gives IDLE");
            // At the +x end the move is blocked and the state goes back to IDLE.
            rider.phase = ClimbPhase::Left;
            input = stickInput(2.0f, 1.5707964f);
            step = climbBarUpdate(rider, set, input);
            require(rider.phase == ClimbPhase::Idle && near(step.position[0], 2.0f), "end of the bar blocks");
            // Stick across the bar (inside +-0.3) does nothing.
            rider.phase = ClimbPhase::Idle;
            input = stickInput(0.0f, 0.0f);
            step = climbBarUpdate(rider, set, input);
            require(rider.phase == ClimbPhase::Idle, "stick across the bar is ignored");
        }
        {
            const ClimbBarSet set = madeUpSet();
            ClimbBarRider rider;
            rider.phase = ClimbPhase::Idle;
            rider.bar = 0;
            ClimbBarInput input = stickInput(0.0f, 0.0f);
            input.stickHeld = false;
            // B held at once: the press is used to jump off once; the latch holds until B is released.
            input.bHeld = true;
            ClimbBarStep step = climbBarUpdate(rider, set, input);
            require(!step.riding && step.release == ClimbBarRelease::Jump && rider.ignoreBar == 0 && rider.jumpLatch, "B jumps off");
            rider = ClimbBarRider{};
            rider.phase = ClimbPhase::Idle;
            rider.bar = 0;
            rider.jumpLatch = true;
            step = climbBarUpdate(rider, set, input);
            require(step.riding, "B held with the latch set does nothing");
            input.bHeld = false;
            step = climbBarUpdate(rider, set, input);
            require(!rider.jumpLatch, "releasing B clears the latch");
            input.aPressed = true;
            step = climbBarUpdate(rider, set, input);
            require(!step.riding && step.release == ClimbBarRelease::Fall, "A drops");
        }
        {
            // Arrival: 8 * dt per update towards the hang point, then the snap and the placement update.
            const ClimbBarSet set = madeUpSet();
            ClimbBarRider rider;
            ClimbBarInput input = stickInput(0.0f, 0.0f);
            input.stickHeld = false;
            input.position[1] = hangY() + 1.0f;
            climbBarGrab(rider, set, 0, input);
            require(rider.phase == ClimbPhase::Arrive, "grab enters the arrival");
            ClimbBarStep step = climbBarUpdate(rider, set, input);
            require(near(step.position[1], hangY() + 1.0f - 8.0f / 30.0f, 1e-4f), "arrival moves 8 dt");
            input.position[1] = step.position[1];
            for (int i = 0; i < 4; ++i) {
                step = climbBarUpdate(rider, set, input);
                input.position[1] = step.position[1];
            }
            require(rider.phase == ClimbPhase::Idle && near(step.position[1], hangY()), "arrival snaps");
        }
        {
            // The jump counter: tests from the 7th update, as the zipline.
            ClimbBarRider rider;
            require(!climbBarJumpTick(rider, true, true), "no test in the take-off update");
            int first = -1;
            for (int k = 1; k < 10 && first < 0; ++k)
                if (climbBarJumpTick(rider, false, true)) first = k;
            require(first == 6, "first test in update 6, found " + std::to_string(first));
        }
    } catch (const std::exception& error) {
        std::cerr << "climb-bar-test FAILED: " << error.what() << '\n';
        return 1;
    }
    std::cout << "climb-bar-test: ok\n";
    return 0;
}
