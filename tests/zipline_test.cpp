// The zipline (src/runtime/zipline.cpp) on a made-up line, no game data needed. Every expected value is a hand evaluation of the rules in
// docs/re/player-traversal.md (0x19b074 link, 0x2a9d94 grab test, 0x221570 / 0x1b6664 use test, 0x275000 attach, 0x2c4114 slide,
// state 44 root motion, the jump counter ctrl+0x2cf of state 4) on the two anchors A (0, 10, 0) and B (20, 5, 0) that point at each other.
#include "runtime/zipline.hpp"
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

ZiplineAnchor anchor(const char* name, std::size_t order, float x, float y, float z, int target) {
    ZiplineAnchor result;
    result.instance = name;
    result.order = order;
    result.position[0] = x;
    result.position[1] = y;
    result.position[2] = z;
    result.target = target;
    result.lineType = kGrappleLineZipline;
    result.faceTowards = 1;
    // Class bounds of props/parkour_zipline: WalkBound centre (0, -1.5, 0.4) half (0.5, 1.5, 0.8), UseBound centre
    // (0.028, 0.1505, 0.2108) half (0.5, 1, 0.5) (class file dump, docs/re/player-traversal.md).
    const float walkCentre[3] = {0, -1.5f, 0.4f}, walkHalf[3] = {0.5f, 1.5f, 0.8f};
    const float useCentre[3] = {0.028f, 0.1505f, 0.2108f}, useHalf[3] = {0.5f, 1.0f, 0.5f};
    for (int a = 0; a < 3; ++a) {
        result.classWalkCentre[a] = walkCentre[a];
        result.classWalkHalf[a] = walkHalf[a];
        result.useCentre[a] = useCentre[a];
        result.useHalf[a] = useHalf[a];
    }
    result.hasUseBound = true;
    return result;
}

std::vector<ZiplineAnchor> line() {
    std::vector<ZiplineAnchor> anchors = {anchor("a", 0, 0, 10, 0, 1), anchor("b", 1, 20, 5, 0, 0)};
    linkZiplineAnchors(anchors);
    return anchors;
}

void link() {
    const std::vector<ZiplineAnchor> anchors = line();
    const ZiplineAnchor& a = anchors[0];
    require(!a.listed && anchors[1].listed, "0x1a9bb4 lists the anchor whose id is not below its target's (b)");
    require(near(a.forward[0], 1, 1e-6f) && near(a.forward[1], 0, 1e-6f), "forward is horizontal towards the target");
    require(near(a.right[2], -1, 1e-6f), "right = up x forward");
    require(near(a.endOffset, -0.4f, 1e-6f), "+0x110 = 0.4 - 0.8");
    require(near(a.walkCentre[2], 10, 1e-5f) && near(a.walkHalf[2], 10.4f, 1e-5f), "WalkBound z: d/2 and 0.4 + d/2 for the horizontal distance 20");
    require(near(a.walkCentre[1], -1.5f, 1e-6f) && near(a.walkHalf[0], 0.5f, 1e-6f), "WalkBound x and y stay the class values");
}

void grab() {
    const std::vector<ZiplineAnchor> anchors = line();
    const float below[3] = {10, 6, 0};
    require(ziplineGrabTest(anchors, below, {}) == 1, "1.5 under the middle of the line: y' = 1 - 5 * 10 / 20 = -1.5 in [-3, 0]");
    const float above[3] = {10, 8, 0};
    require(ziplineGrabTest(anchors, above, {}) == -1, "0.5 above the line");
    const float aside[3] = {10, 6, 0.6f};
    require(ziplineGrabTest(anchors, aside, {}) == -1, "0.6 to the side, half width 0.5");
    const float beyond[3] = {-0.5f, 8.5f, 0};
    require(ziplineGrabTest(anchors, beyond, {}) == -1, "0.5 behind the high anchor, the bound ends 0.4 behind it");
    require(ziplineGrabTest(anchors, below, [](const ZiplineAnchor& anchor) { return anchor.instance != "a"; }) == -1,
            "a disabled target anchor turns the line off");
}

void use() {
    const std::vector<ZiplineAnchor> anchors = line();
    const float player[3] = {0.2f, 10, 0};
    require(ziplineUseTest(anchors, player, -16384, {}) == 0, "inside UseBound of the high anchor (local (0, 0, 0.2)) and facing it");
    require(ziplineUseTest(anchors, player, 16384, {}) == -1, "FaceTowards 1: facing away fails");
    const float low[3] = {19.8f, 5, 0};
    require(ziplineUseTest(anchors, low, 16384, {}) == -1, "0x1b6664 returns 0xff at the low anchor (target higher)");
}

void jumpCounter() {
    ZiplineRider rider;
    require(!ziplineJumpTick(rider, true, true), "the take-off update: ctrl+0x2cf = -1 + 1");
    for (int update = 2; update <= 6; ++update) require(!ziplineJumpTick(rider, false, true), "counter 0..4 before the update: no test");
    require(ziplineJumpTick(rider, false, true), "update 7 after the press: counter 5 > 4 (m12-9: grab in the 7th jump update)");
    require(!ziplineJumpTick(rider, false, false) && rider.jumpCount == -1, "the counter is gone outside the jump");
}

void ride() {
    const std::vector<ZiplineAnchor> anchors = line();
    ZiplineRider rider;
    ZiplineInput input;
    input.dt = 1.0f / 30.0f;
    input.position[0] = 1.6f;
    input.position[1] = 8.0f;
    input.position[2] = 0.0f;
    input.facing = 0;
    input.grabUpdates = 3;
    input.dismountUpdates = 4;
    ZiplineStep step = ziplineEnter(rider, anchors, 1, input);
    // Start (-0.4, 10, 0), end (20.4, 5, 0): span sqrt(20.8^2 + 5^2) = 21.392522; rate = 30 / (21.392522 / (7 / 30)) = 0.3272173.
    require(rider.line == 0, "the attach takes the higher anchor (a)");
    require(near(rider.rate, 0.3272173f, 1e-6f), "rate " + std::to_string(rider.rate));
    require(rider.heading == 16383 || rider.heading == 16384, "heading atan2(20.8, 0) * 10430.378 = 16384: " + std::to_string(rider.heading));
    // t0 = |(1.6, 8, 0) - (-0.4, 10, 0)| / 21.392522 = 2.8284271 / 21.392522 = 0.1322157, then + 0.3272173 / 30 = 0.1431229.
    require(near(rider.t, 0.1431229f, 1e-6f), "t " + std::to_string(rider.t));
    // Point at t: (20 t, 10 - 5 t, 0) = (2.862458, 9.284386, 0); the player moves 0.2 of the way: (1.852492, 8.256877, 0).
    require(near(step.position[0], 1.852492f, 1e-5f) && near(step.position[1], 8.256877f, 1e-5f), "first slide update");
    require(step.facing == 6400, "the facing turns by at most 0x1900 per update");
    require(step.animation == kAnimationZiplineGrab && step.sounds.size() == 1 && step.sounds[0].action == ZiplineSoundAction::Play,
            "Get On plays zipline_grab and starts the loop sound 107");

    for (int update = 1; update <= 3; ++update) {
        std::copy(step.position, step.position + 3, input.position);
        input.facing = step.facing;
        step = ziplineUpdate(rider, anchors, input);
        require(step.riding && step.animation == (update < 3 ? kAnimationZiplineGrab : kAnimationZiplineSlide),
                "Get On lasts grabUpdates updates, the next one is the slide: " + std::to_string(update));
    }
    require(step.facing == 16384 || step.facing == 16383, "facing reached the heading");

    std::copy(step.position, step.position + 3, input.position);
    input.jumpPressed = true;
    const float before[3] = {step.position[0], step.position[1], step.position[2]};
    step = ziplineUpdate(rider, anchors, input);
    input.jumpPressed = false;
    require(rider.phase == ZiplinePhase::Dismount && step.animation == kAnimationZiplineDismount, "B during the slide: dismount in the same update");
    require(step.position[0] == before[0] && step.position[1] == before[1], "no slide move and root motion frame 0 in that update");
    require(step.sounds.size() == 1 && step.sounds[0].action == ZiplineSoundAction::Stop, "leaving the slide stops the loop");

    chase::assets::RootMotion motion;
    motion.translation = {{0, 0, 0}, {0, 1, 1}, {0, 0, 2}};
    input.dismountMotion = &motion;
    std::copy(step.position, step.position + 3, input.position);
    input.facing = 16384;
    for (int update = 1; update <= 3; ++update) {
        step = ziplineUpdate(rider, anchors, input);
        std::copy(step.position, step.position + 3, input.position);
    }
    // Frame 1.5 at the third update: root (0, 0.5, 1.5) along the facing +x.
    require(near(step.position[0], before[0] + 1.5f, 1e-5f) && near(step.position[1], before[1] + 0.5f, 1e-5f), "dismount root motion");
    require(near(rider.fallSpeed, 0.5f, 1e-6f) && near(rider.fallDistance, 1.0f, 1e-6f), "height lost: 0.5 this update, 1 over the key 1 -> 2");
    step = ziplineUpdate(rider, anchors, input);
    require(step.falling && !rider.active() && near(step.fallSpeed, 0.5f, 1e-6f) && near(step.fallDistance, 1.0f, 1e-6f),
            "the clip end starts state 5 with the last values");
}

void lineEnd() {
    const std::vector<ZiplineAnchor> anchors = line();
    ZiplineRider rider;
    ZiplineInput input;
    input.position[0] = 19.9f;
    input.position[1] = 5.2f;
    input.facing = 16384;
    ZiplineStep step = ziplineEnter(rider, anchors, 0, input);
    for (int update = 0; update < 30 && !rider.endReached; ++update) {
        std::copy(step.position, step.position + 3, input.position);
        input.facing = step.facing;
        step = ziplineUpdate(rider, anchors, input);
    }
    require(rider.endReached && rider.t > 1.0f, "past t = 1 with the target behind: event 6");
    const float x = step.position[0];
    std::copy(step.position, step.position + 3, input.position);
    step = ziplineUpdate(rider, anchors, input);
    require(rider.phase == ZiplinePhase::Dismount && step.position[0] == x, "the dismount starts in the next update without a move");
}

}

int main() {
    try {
        link();
        grab();
        use();
        jumpCounter();
        ride();
        lineEnd();
    } catch (const std::exception& error) {
        std::cerr << "zipline-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "zipline-test: ok\n";
    return 0;
}
