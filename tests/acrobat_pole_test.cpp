// Acrobat pole rules (src/runtime/acrobat_pole.cpp) on two made-up poles and made-up clips; hand values from the rules read in the original
// (docs/re/player-traversal-rails.md §2026-10-19T07:53:17): flip test 0x1e4744, idle end 0x31c2dc, Flip Done 0x334680, jump enter 0x31f3bc,
// Jump Done 0x334794.
#include "runtime/acrobat_pole.hpp"
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

// Two acrobat poles along +z (yaw 0), 4 apart and 1 higher, the first linked to the second.
ClimbBarSet madeUpPoles() {
    ClimbBarSet set;
    for (int k = 0; k < 2; ++k) {
        ClimbBar pole;
        pole.instance = "pole" + std::to_string(k);
        pole.position[1] = 10.0f + float(k);
        pole.position[2] = 4.0f * float(k);
        pole.grabHalf[0] = 1.0f;
        pole.hasGrabBound = true;
        pole.acrobatPole = true;
        pole.swingSound = 7;
        set.bars.push_back(pole);
    }
    ClimbBarLink& next = set.bars[0].links[0];
    next.present = true;
    next.bar = 1;
    next.objectType = kClimbBarObjectType;
    std::copy(set.bars[1].position, set.bars[1].position + 3, next.position);
    return set;
}

// Idle 4 frames at 30 per second (end at clip time 3), flip 2 frames at 15, jump 4 frames at 15 moving 2 along z with a hump of 1; the phase
// events at frames 1 and 3, the vertical speed span 2 frames.
AcrobatPoleClips madeUpClips() {
    AcrobatPoleClips clips;
    clips.idle = {4.0f, 30.0f};
    clips.flip = {2.0f, 15.0f};
    clips.jump = {4.0f, 15.0f};
    clips.jumpRoot = {{0, 0, 0}, {0, 0.5f, 0.5f}, {0, 1.0f, 1.0f}, {0, 0.5f, 1.5f}, {0, 0, 2.0f}};
    clips.jumpTravel = 2.0f;
    clips.jumpEventFrames = {1.0f, 3.0f};
    clips.jumpPhaseFrames = 2.0f;
    return clips;
}

struct Driver {
    ClimbBarSet set = madeUpPoles();
    AcrobatPoleClips clips = madeUpClips();
    AcrobatPoleRider rider;
    float position[3] = {0, 0, 0};
    std::int16_t facing = 0;
    AcrobatPoleStep step(bool b, bool stick = false, std::int16_t heading = 0) {
        AcrobatPoleInput input;
        input.dt = 1.0f / 30.0f;
        std::copy(position, position + 3, input.position);
        input.facing = facing;
        input.bHeld = b;
        input.stickHeld = stick;
        input.stickHeading = heading;
        const AcrobatPoleStep out = acrobatPoleUpdate(rider, set, clips, input);
        std::copy(out.position, out.position + 3, position);
        facing = out.facing;
        return out;
    }
};
}

int main() {
    try {
        // 0x1e4744 with the camera yaw 0 (camera x row (1, 0, 0)): facing +x (side 1) flips for h in 0xa001..0xdfff, facing -x for 0x2001..0x5fff.
        require(!acrobatPoleFlipTest(0x4000, false, std::int16_t(0xc000), 0.0f), "flip test needs the stick");
        require(acrobatPoleFlipTest(0x4000, true, std::int16_t(0xc000), 0.0f), "facing +x, stick to -x flips");
        require(!acrobatPoleFlipTest(0x4000, true, 0x4000, 0.0f), "facing +x, stick to +x keeps");
        require(acrobatPoleFlipTest(std::int16_t(0xc000), true, 0x4000, 0.0f), "facing -x, stick to +x flips");
        require(!acrobatPoleFlipTest(0x4000, true, 0x2000, 0.0f) && !acrobatPoleFlipTest(0x4000, true, std::int16_t(0xe000), 0.0f), "range ends excluded");
        require(acrobatPoleFlipTest(0, true, 0x2001, 0.0f) && acrobatPoleFlipTest(0, true, std::int16_t(0xa001), 0.0f), "side 0 flips on both sides");

        // Arrival done: 93 is entered in the next update and the player stands at the middle of the hang line facing +z (yaw-0 pole z row).
        {
            Driver d;
            acrobatPoleStart(d.rider, 0);
            const AcrobatPoleStep first = d.step(false);
            require(first.state == kAcrobatPoleIdleState && near(d.position[1], 10.0f + kAcrobatPoleHangOffsetY) && near(d.position[2], kAcrobatPoleHangOffsetZ),
                    "idle placement");
            // Clip time 1, 2 at the next updates; at clip time 3 (length - 1) the swing sound and, with B held, the jump request.
            require(d.step(false).state == kAcrobatPoleIdleState && d.step(false).state == kAcrobatPoleIdleState, "idle runs to the clip end");
            const AcrobatPoleStep end = d.step(true);
            require(end.state == kAcrobatPoleIdleState && end.sounds.size() == 1 && end.sounds[0].id == 7 && d.rider.pending == kAcrobatPoleJumpState,
                    "idle end with B: jump requested");
            // Jump enter: hang offset of the jump clip, scale 4 / 2 = 2, vertical speed 1 / (2 / 15) = 7.5, facing +z.
            const AcrobatPoleStep entered = d.step(true);
            require(entered.state == kAcrobatPoleJumpState && near(d.rider.rootScale, 2.0f) && near(d.rider.verticalSpeed, 7.5f), "jump enter");
            require(near(d.position[1], 10.0f + kAcrobatPoleJumpHangOffsetY) && near(d.position[2], kAcrobatPoleJumpHangOffsetZ), "jump enter placement");
            // Update 1 (clip 0 -> 0.5): root y 0.25, z 0.25 x 2; the phase event at frame 1 comes after update 1; update 2 adds 7.5 x dt.
            const float y0 = d.position[1], z0 = d.position[2];
            d.step(false);
            require(near(d.position[1], y0 + 0.25f) && near(d.position[2], z0 + 0.5f) && d.rider.phase == 1.0f, "jump update 1");
            d.step(false);
            require(near(d.position[1], y0 + 0.5f + 0.25f) && near(d.position[2], z0 + 1.0f), "jump update 2 climbs");
            // Phase 2 after update 5 (clip 3); update 6 stands the player on pole 1; the clip ends after update 7 and Jump Done (B released) requests 93.
            for (int k = 0; k < 4; ++k) d.step(false);
            require(d.rider.pole == 1 && near(d.position[1], 11.0f + kAcrobatPoleJumpHangOffsetY + (0.0f - 0.5f) * 0.5f), "placed on pole 1");
            d.step(false);
            const AcrobatPoleStep done = d.step(false);
            require(done.state == kAcrobatPoleIdleState && d.rider.from == 1, "Jump Done to idle on pole 1");
        }

        // Idle facing -z on pole 0 with the stick to +x (camera yaw 0; the side is -sin(pi), just below 0): flip at the idle end, 4 updates of the flip, then
        // Flip Done (stick released) turns the player to +z and returns to 93.
        {
            Driver d;
            d.facing = std::int16_t(0x8000);
            acrobatPoleStart(d.rider, 0);
            for (int k = 0; k < 3; ++k) d.step(false, true, 0x4000);
            const AcrobatPoleStep end = d.step(false, true, 0x4000);
            require(end.state == kAcrobatPoleIdleState && d.rider.pending == kAcrobatPoleFlipState, "idle end: flip requested");
            for (int k = 0; k < 4; ++k) require(d.step(false).state == kAcrobatPoleFlipState, "flip runs 4 updates");
            const AcrobatPoleStep back = d.step(false);
            require(back.state == kAcrobatPoleIdleState && d.facing == 0, "Flip Done: idle facing +z");
        }
        std::cout << "acrobat-pole-test: ok\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "acrobat-pole-test: " << error.what() << '\n';
        return 1;
    }
}
