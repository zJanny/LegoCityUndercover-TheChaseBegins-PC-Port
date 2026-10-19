// chase_runtime replicas (scheduler 0x100f44, pad pipeline) against values traced from the disassembly
// and read from the running game.
#include "runtime/collision.hpp"
#include "runtime/follow_camera.hpp"
#include "runtime/object_visibility.hpp"
#include "runtime/pad.hpp"
#include "runtime/player_intent.hpp"
#include "runtime/player_locomotion.hpp"
#include "runtime/player_vertical.hpp"
#include "runtime/scheduler.hpp"
#include <array>
#include <cmath>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>
using namespace chase::runtime;

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
bool near(float a, float b) { return std::fabs(a - b) < 1e-6f; }
float floatOf(std::uint32_t bits) {
    float value;
    std::memcpy(&value, &bits, sizeof value);
    return value;
}
std::uint32_t bitsOf(float value) {
    std::uint32_t bits;
    std::memcpy(&bits, &value, sizeof bits);
    return bits;
}
StickIntentInput stickAt(float x, float y, std::uint16_t type = 0x100, float yaw = 0.0f) {
    StickIntentInput input;
    input.slot14 = {x, 0.1f};
    input.slot15 = {y, 0.1f};
    input.entityType = type;
    input.cameraYaw = yaw;
    return input;
}
// A collision item with a single leaf node: quads are four corners (counter-clockwise seen from the normal).
chase::assets::CollisionItem quadItem(const std::vector<std::array<float, 12>>& quads) {
    chase::assets::CollisionItem item;
    item.name = "Collision_Test";
    for (const auto& quad : quads) {
        const std::uint32_t base = std::uint32_t(item.vertices.size() / 3);
        item.vertices.insert(item.vertices.end(), quad.begin(), quad.end());
        for (const std::array<std::uint32_t, 3>& corner : {std::array<std::uint32_t, 3>{0, 1, 2}, {0, 2, 3}}) {
            chase::assets::CollisionTriangle triangle;
            float edgeA[3], edgeB[3], normal[3];
            for (int a = 0; a < 3; ++a) {
                edgeA[a] = quad[corner[1] * 3 + std::size_t(a)] - quad[corner[0] * 3 + std::size_t(a)];
                edgeB[a] = quad[corner[2] * 3 + std::size_t(a)] - quad[corner[0] * 3 + std::size_t(a)];
            }
            normal[0] = edgeA[1] * edgeB[2] - edgeA[2] * edgeB[1];
            normal[1] = edgeA[2] * edgeB[0] - edgeA[0] * edgeB[2];
            normal[2] = edgeA[0] * edgeB[1] - edgeA[1] * edgeB[0];
            const float length = std::sqrt(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
            for (int a = 0; a < 3; ++a) {
                triangle.normal[a] = normal[a] / length;
                triangle.vertex[a] = base + corner[std::size_t(a)];
            }
            triangle.d = triangle.normal[0] * quad[corner[0] * 3] + triangle.normal[1] * quad[corner[0] * 3 + 1] +
                         triangle.normal[2] * quad[corner[0] * 3 + 2];
            item.triangles.push_back(triangle);
        }
    }
    chase::assets::CollisionNode node;
    for (int a = 0; a < 3; ++a) node.half[a] = 100;
    node.info = 1 | (std::uint32_t(item.triangles.size()) << 1);
    item.nodes.push_back(node);
    return item;
}
}

int main() {
    try {
        std::vector<float> dts;
        const auto record = [&](float dt) { dts.push_back(dt); };

        // Oracle 2026-10-13T10:35:31: 151 ticks since the last update gave dt 0.050333 and last = now.
        Scheduler game({}, 1000);
        require(game.advance(1151, record) == 1 && dts.size() == 1, "one step per call");
        require(near(dts[0], 151.0f / 3000.0f) && game.last() == 1151 && game.timerTicks() == 151, "measured dt");

        // Below the minimum: one step of 0.03 (90 ticks); last runs ahead of the clock.
        dts.clear();
        require(game.advance(1201, record) == 1 && near(dts[0], 0.03f), "minimum dt");
        require(game.last() == 1241, "last advances by the minimum period");
        require(game.advance(1231, record) == 0 && dts.size() == 1, "no update while last is ahead");

        // Above the maximum with cap 1: dt 0.07 (210 ticks) and the backlog is dropped.
        dts.clear();
        require(game.advance(2000, record) == 1 && near(dts[0], 0.07f) && game.last() == 2000, "maximum dt");

        // Calls every vblank (50 ticks): each update takes the 90-tick minimum, so updates skip calls
        // while last is ahead; game time follows the clock at about 33 updates per second.
        Scheduler vsync({}, 0);
        dts.clear();
        for (std::uint32_t now = 50; now <= 3000; now += 50) vsync.advance(now, record);
        require(dts.size() == 34 && vsync.timerTicks() == 34 * 90 && vsync.last() == 3060, "vblank-rate calls");

        // Catch-up path: cap 4 splits the elapsed time evenly, then the minimum applies per step.
        StepConfig catchUp;
        catchUp.maxSteps = 4;
        Scheduler split(catchUp, 0);
        dts.clear();
        require(split.advance(600, record) == 3, "ceil(600 / 210) steps");
        require(near(dts[0], 0.0666667f) && split.last() == 600, "even split");

        // Cap reached below 5 with last behind the clock: backlog dropped.
        dts.clear();
        require(split.advance(1600, record) == 4 && near(dts[0], 0.07f) && split.last() == 1600, "cap 4 drop");

        // Fixed dt flag C+0x21 replaces the period.
        StepConfig fixed;
        fixed.useFixedDt = true;
        fixed.fixedDt = 1.0f / 60.0f;
        Scheduler steady(fixed, 0);
        dts.clear();
        require(steady.advance(100, record) == 1 && near(dts[0], 50.0f / 3000.0f) && steady.last() == 100, "fixed dt");

        // Scale C+0x1c applies to the ticks given to the client, not to the clock.
        StepConfig slow;
        slow.scale = 0.5f;
        Scheduler half(slow, 0);
        dts.clear();
        require(half.advance(150, record) == 1 && near(dts[0], 75.0f / 3000.0f) && half.last() == 150, "scale");

        // Circle clamp 0x119724 with the default min 40 / max 145 (values worked through the integer code).
        require(circleClamp(0, 30, 40, 145) == std::make_pair<std::int16_t, std::int16_t>(0, 0), "inside min");
        require(circleClamp(100, 0, 40, 145) == std::make_pair<std::int16_t, std::int16_t>(60, 0), "length - min");
        require(circleClamp(200, 0, 40, 145) == std::make_pair<std::int16_t, std::int16_t>(105, 0), "capped at max - min");
        require(circleClamp(100, 100, 40, 145) == std::make_pair<std::int16_t, std::int16_t>(71, 71), "isqrt 18101");
        require(circleClamp(-100, 0, 40, 145) == std::make_pair<std::int16_t, std::int16_t>(-60, 0), "negative axis");

        // 0x10fcb0 and 0x1035d0.
        require(axisValue(0, 105) == 0.0f && axisValue(105, 105) == 1.0f && near(axisValue(60, 105), 60.0f / 105.0f), "axis");
        require(axisValue(-105, 105) == -1.0f && near(axisValue(-140, 105), -140.0f / 105.0f), "negative side not capped");
        require(deadzoneAxis(0.2f, 0.25f) == 0.0f && deadzoneAxis(-0.2f, 0.25f) == 0.0f, "inside dead zone");
        require(near(deadzoneAxis(0.625f, 0.25f), 0.5f) && near(deadzoneAxis(-0.625f, 0.25f), -0.5f), "remap");
        require(deadzoneAxis(1.5f, 0.25f) == 1.0f && deadzoneAxis(-1.5f, 0.25f) == -1.0f, "clamp");

        // Oracle 2026-10-13 (language screen): raw stick (100, 50) gave ch0 0.5779, ch1 0.2889 with the
        // game's defaults (dead zone 0.1, threshold 0.75 on every channel).
        PadDevice live;
        PadState pushed;
        pushed.stickX = 100;
        pushed.stickY = 50;
        live.poll(pushed);
        require(std::fabs(live.slot(14).value - 0.5779f) < 5e-5f && std::fabs(live.slot(15).value - 0.2889f) < 5e-5f,
                "matches the running game");
        require(live.channel(0).threshold == 0.75f && live.channel(16).deadzone == 0.1f, "defaults");

        // Poll: buttons, stick, radial dead zone and edges 0x25b4b4.
        PadDevice pad;
        pad.channel(0).deadzone = 0.25f;
        for (std::size_t i = 0; i < kChannelCount; ++i) pad.channel(i).threshold = 0.5f;
        PadState state;
        state.held = PadA | PadUp | 0x2000;
        state.stickX = 145;
        pad.poll(state);
        require(pad.slot(6).value == 1.0f && pad.slot(6).press == 1 && pad.slot(4).press == 1, "A on slots 4 and 6");
        require(pad.slot(0).value == 1.0f && pad.channel(12).press == 1, "Up on slot 0");
        require(pad.slot(14).value == 1.0f && pad.slot(15).value == 0.0f && pad.slot(14).press == 1, "stick x full");
        pad.poll(state);
        require(pad.slot(6).press == 0 && pad.slot(6).release == 0, "no repeat while held");
        state.held = 0;
        state.stickX = -145;
        pad.poll(state);
        require(pad.slot(6).value == 0.0f && pad.slot(6).release == 1, "A release edge");
        require(pad.slot(14).value == -1.0f && pad.slot(14).release == 1 && pad.slot(14).press == -1, "stick reversal");
        state.stickX = -100;
        pad.poll(state);
        const float expected = (60.0f / 105.0f - 0.25f) / 0.75f;
        require(near(pad.slot(14).value, -expected) && pad.slot(14).release == -1 && pad.slot(14).press == 0,
                "back inside the negative threshold");
        // Library atan2f behind 0x24765c (code at 0x2473ec..0x247f00 extracted from the image and run
        // under qemu-arm, 2026-10-13): {first, second, result} bit patterns. Not libm: the 1e30 row is one ulp
        // below atan2f(1e30, 1e30).
        const std::uint32_t atanRows[][3] = {
            {0x00000000, 0x00000000, 0x00000000}, {0x80000000, 0x80000000, 0xc0490fdb},
            {0x00000000, 0x80000000, 0x40490fdb}, {0x3f800000, 0x00000000, 0x3fc90fdb},
            {0xbf800000, 0x80000000, 0xbfc90fdb}, {0x7f800000, 0x7f800000, 0x3f490fdb},
            {0xff800000, 0x7f800000, 0xbf490fdb}, {0x7f800000, 0xff800000, 0x4016cbe4},
            {0x3f800000, 0x7f800000, 0x00000000}, {0x0da24260, 0x3f800000, 0x0da24260},
            {0x0da24260, 0xbf800000, 0x40490fdb}, {0x8da24260, 0xbf800000, 0xc0490fdb},
            {0x40400000, 0x3f800000, 0x3f9fe0bb}, {0x3f800000, 0x40400000, 0x3ea4bc7d},
            {0xc0400000, 0xbf800000, 0xbff23efa}, {0x3f800000, 0xbfc00000, 0x40236e05},
            {0x3f19999a, 0x3f000000, 0x3f604557}, {0x3f000000, 0x3f19999a, 0x3f31da5e},
            {0x7149f2ca, 0x7149f2ca, 0x3f490fda}, {0x0da24260, 0x0da24260, 0x3f490fdb},
            {0x3e99999a, 0xbf666666, 0x4034784b}, {0x40000000, 0x40400000, 0x3f168757},
            {0xbe99999a, 0x3f333333, 0xbecf4dfb}, {0x00000001, 0x3f800000, 0x00000001},
        };
        for (const auto& row : atanRows)
            require(bitsOf(gameAtan2(floatOf(row[0]), floatOf(row[1]))) == row[2], "gameAtan2 bit pattern");
        require(std::isnan(gameAtan2(NAN, 1.0f)) && std::isnan(gameAtan2(1.0f, NAN)), "gameAtan2 NaN");

        // Stick to intent, 0x2007a0..0x200864 (heading = (s16)(atan2(x, y) * 10430.378) + (s16)(yaw * 10430.378),
        // flag 8 when x^2 + y^2 < 0.5 on the raw values, 0x200820). Expected values worked through the
        // float arithmetic: sqrt(|v|) * v per axis unless type 0x14b / 0x189 (0x2007b0..0x2007c4).
        StickIntent up = stickIntent(stickAt(0.0f, 1.0f));
        require(up.active && up.heading == 0 && up.heldSet == kHeldMove, "stick up");
        StickIntent right = stickIntent(stickAt(1.0f, 0.0f));
        require(right.heading == 16384 && right.heldSet == kHeldMove, "stick right is a quarter turn");
        require(stickIntent(stickAt(-1.0f, 0.0f)).heading == -16384, "stick left");
        require(stickIntent(stickAt(0.0f, -1.0f)).heading == -32768, "stick down wraps to s16 min");
        require(stickIntent(stickAt(1.0f, 1.0f)).heading == 8192, "diagonal");
        require(stickIntent(stickAt(0.4f, 0.8f)).heading == 3544, "square-root shaping");
        require(stickIntent(stickAt(0.4f, 0.8f, 0x189)).heading == 4836, "type 0x189 skips shaping");
        require(stickIntent(stickAt(0.4f, 0.8f, 0x14b)).heading == 4836, "type 0x14b skips shaping");
        require(stickIntent(stickAt(-0.6f, -0.3f)).heading == -19928, "negative quadrant");

        // Held flag 8 compares the bits of x^2 + y^2 with 0x3f000000 (0.5): exactly 0.5 does not set it.
        require(stickIntent(stickAt(0.5f, 0.5f)).heldSet == kHeldMove, "sum 0.5 is not slow");
        require(stickIntent(stickAt(0.5f, 0.4999f)).heldSet == (kHeldMove | kHeldSlow), "sum below 0.5 is slow");
        require(stickIntent(stickAt(0.5f, 0.0f, 0x14b)).heldSet == (kHeldMove | kHeldSlow), "slow uses the raw values");
        require(stickIntent(stickAt(1.0f, 0.0f)).heldSet == kHeldMove, "sum 1");

        // Camera yaw: (s16)(yaw * 10430.378) is added modulo 2^16 (0x200848..0x200864).
        require(stickIntent(stickAt(0.0f, 1.0f, 0x100, -1.0f)).heading == -10430, "yaw truncates toward zero");
        require(stickIntent(stickAt(1.0f, 0.0f, 0x100, 3.14159265f)).heading == -16384, "pi yaw plus quarter turn wraps");
        require(stickIntent(stickAt(0.5f, 0.5f, 0x100, 3.0f)).heading == -26053, "31291 + 8192 wraps");
        require(stickIntent(stickAt(1.0f, 0.0f, 0x100, 4.0f)).heading == -7431, "41721 as u16 + 16384");

        // Active test 0x200588..0x2005d0: |x| > dead zone of slot 14, or |y| > dead zone of slot 15 (strict).
        StickIntent idle = stickIntent(stickAt(0.1f, -0.1f));
        require(!idle.active && idle.heading == 0 && idle.heldSet == 0, "on the dead zone is idle");
        require(stickIntent(stickAt(0.0f, 0.11f)).active && stickIntent(stickAt(-0.11f, 0.0f)).active, "either axis activates");
        StickIntentInput separate = stickAt(0.2f, 0.0f);
        separate.slot14.deadzone = 0.3f;
        require(!stickIntent(separate).active, "slot 14 uses its own dead zone");
        separate.slot15 = {0.2f, 0.1f};
        require(stickIntent(separate).active, "slot 15 uses its own dead zone");
        require(!stickIntent(stickAt(NAN, 0.0f)).active, "NaN is not active");

        // 0x1aa8ac (signed byte at 0x372d44) nonzero: bits 0 and 1 of +0xc and +0xe cleared, stick ignored.
        StickIntentInput blocked = stickAt(1.0f, 0.0f);
        blocked.byte372d44 = -1;
        StickIntent cleared = stickIntent(blocked);
        require(!cleared.active && cleared.heading == 0 && cleared.heldSet == 0 && cleared.heldClear == 3 &&
                    cleared.pressClear == 3,
                "blocked stick");

        // Locomotion regimes from the raw stick through the pad pipeline (oracle 2026-10-13, loco-threshold*.txt:
        // raw 59 idle, 60 walk, 117 walk, 118 run; stick (0, M), camera yaw 0).
        const auto regimeAt = [&](int rawY) {
            PadDevice device;
            PadState raw;
            raw.stickY = std::int16_t(rawY);
            device.poll(raw);
            StickIntentInput input = stickAt(device.slot(14).value, device.slot(15).value);
            LocomotionState state;
            return locomotionUpdate(state, 0.033f, stickIntent(input)).regime;
        };
        require(regimeAt(50) == Regime::Idle && regimeAt(59) == Regime::Idle, "raw 59 is idle");
        require(regimeAt(60) == Regime::Walk && regimeAt(90) == Regime::Walk && regimeAt(117) == Regime::Walk, "raw 60 and 117 walk");
        require(regimeAt(118) == Regime::Run && regimeAt(145) == Regime::Run && regimeAt(156) == Regime::Run, "raw 118 runs");

        // Speed, timer and sprint: run 6.9 u/s for 30 updates at dt 0.033, the timer passes 1.0 in the 31st
        // update (1.023), whose own movement is still run speed; sprint (x1.5) moves from the 32nd.
        // Speeds derived from the code constants (0.23 run step, one third for walk, 30 steps per second); oracle: ctrl+0x8c 0.23,
        // ctrl+0x90 0.076667 in every locomotion trace, measured 2.3005..2.3010 and 6.9000..6.9004 u/s.
        require(std::fabs(kCharacterSpeeds.run - 6.9f) < 1e-5f && std::fabs(kCharacterSpeeds.walk - 2.3f) < 1e-5f && kCharacterSpeeds.sprintScale == 1.5f,
                "walk 2.3 and run 6.9 units per second");
        {
            LocomotionState state;
            StickIntent forward;
            forward.active = true;
            forward.heldSet = kHeldMove;
            LocomotionStep step;
            for (int update = 1; update <= 31; ++update) {
                step = locomotionUpdate(state, 0.033f, forward);
                require(std::fabs(step.deltaZ - kCharacterSpeeds.run * 0.033f) < 1e-6f && step.deltaX == 0.0f, "run step");
            }
            require(near(state.runTimer, 1.023f) && state.sprinting && step.animation == kAnimationSprint && step.animationScale == 1.5f, "timer passes 1.0 in update 31");
            step = locomotionUpdate(state, 0.033f, forward);
            require(std::fabs(step.deltaZ - kCharacterSpeeds.run * 1.5f * 0.033f) < 1e-6f, "sprint step is 1.5 x run");
            // Release: no movement, scale back to 1, timer kept; a new push restarts the timer.
            StickIntent released;
            step = locomotionUpdate(state, 0.033f, released);
            require(step.deltaZ == 0.0f && step.animation == 0 && step.animationScale == 1.0f && near(state.runTimer, 1.056f), "release keeps the timer");
            step = locomotionUpdate(state, 0.033f, forward);
            require(near(state.runTimer, 0.033f) && !state.sprinting && std::fabs(step.deltaZ - kCharacterSpeeds.run * 0.033f) < 1e-6f, "run re-entry restarts the timer at full run speed");
            // Walk freezes the timer and runs at walk speed; run re-entry restarts it.
            StickIntent walk = forward;
            walk.heldSet |= kHeldSlow;
            locomotionUpdate(state, 0.033f, forward);
            const float held = state.runTimer;
            step = locomotionUpdate(state, 0.033f, walk);
            require(state.runTimer == held && step.animation == kAnimationWalk && std::fabs(step.deltaZ - kCharacterSpeeds.walk * 0.033f) < 1e-6f, "walk keeps the timer");
            locomotionUpdate(state, 0.033f, forward);
            require(near(state.runTimer, 0.033f), "walk to run restarts the timer");
        }
        // Carrying a big object (0x203d18): the walk step times 2.5 (0x374368, oracle heap read of m11-8-donuts-in-hand) in the walk and the run regime.
        {
            LocomotionState state;
            state.carryingBig = true;
            StickIntent forward;
            forward.active = true;
            forward.heldSet = kHeldMove;
            StickIntent walk = forward;
            walk.heldSet |= kHeldSlow;
            require(std::fabs(locomotionUpdate(state, 0.033f, walk).deltaZ - kCharacterSpeeds.walk * kCarryWalkStepScale * 0.033f) < 1e-6f, "walking with a big object");
            require(std::fabs(locomotionUpdate(state, 0.033f, forward).deltaZ - kCharacterSpeeds.walk * kCarryWalkStepScale * 0.033f) < 1e-6f, "running with a big object");
        }
        // Carrying never sprints (0x1abe60): the run role is the carried variant of record 2 (2, 111, 131, 151, 162, 552, 272;
        // heap read in docs/re/player-locomotion.md), 552 for a big and 151 for a small object, so the run timer and the sprint
        // are skipped. Two seconds of full stick stay at the carried speed with the run role.
        {
            require(runRoleSprints(kAnimationRun) && runRoleSprints(111) && runRoleSprints(131) && !runRoleSprints(151) && !runRoleSprints(552),
                    "run roles that sprint");
            LocomotionState state;
            state.carryingBig = true;
            state.runRole = 552;
            StickIntent forward;
            forward.active = true;
            forward.heldSet = kHeldMove;
            LocomotionStep step;
            for (int update = 0; update < 60; ++update) step = locomotionUpdate(state, 0.033f, forward);
            require(!state.sprinting && state.runTimer == 0.0f && step.animation == kAnimationRun && step.animationScale == 1.0f &&
                        std::fabs(step.deltaZ - kCharacterSpeeds.walk * kCarryWalkStepScale * 0.033f) < 1e-6f,
                    "no sprint while carrying a big object");
            state.carryingBig = false;
            state.runRole = 151;
            for (int update = 0; update < 60; ++update) step = locomotionUpdate(state, 0.033f, forward);
            require(!state.sprinting && std::fabs(step.deltaZ - kCharacterSpeeds.run * 0.033f) < 1e-6f, "no sprint while carrying a small object");
        }

        // Movement follows the last intent, forward = (sin h, cos h): heading 16384 moves along +x.
        {
            LocomotionState state;
            StickIntent right;
            right.active = true;
            right.heldSet = kHeldMove;
            right.heading = 16384;
            LocomotionStep step = locomotionUpdate(state, 0.033f, right);
            require(step.deltaX > 0.22f && std::fabs(step.deltaZ) < 1e-3f, "heading 16384 is +x");
            StickIntent released;
            step = locomotionUpdate(state, 0.033f, released);
            require(state.lastIntent == 16384 && step.deltaX == 0.0f, "intent kept after release");
        }

        // Facing rule: sign(d) * min(6400, ceil(f32(0.3) * |d|)), d = wrap(target - facing). Values from
        // loco-turn.txt (turn_rest_r90: -19596 -> -16076 -> -13519 toward -8128; turn_rest_180 reaches
        // 8258..8260) and the cap of two consecutive 6400 steps in the 180 degree case.
        require(turnFacing(-19596, -7863) == -16076 && turnFacing(-16076, -7553) == -13519, "turn right");
        require(turnFacing(0, 100) == 31 && turnFacing(0, 1) == 1 && turnFacing(5, 5) == 5, "ceil (f32(0.3) * 100 = 30.000002) and rest");
        require(turnFacing(0, -100) == -31, "turn left");
        require(turnFacing(0, 20000) == 6000 && turnFacing(0, 5536) == 1661 && turnFacing(0, 30000) == 6400, "cap 6400");
        require(turnFacing(0, -32768) == -6400 && turnFacing(-32768, 0) == 26368, "exactly 180 degrees is d = -32768, a negative step");
        require(turnFacing(30000, -30000) == wrapHeading(30000 + 1661), "shortest way across the wrap (d = 5536)");

        // Collision (docs/re/collision.md): ground height by bisection of the swept box, kept within 0.01,
        // wall sliding with the box at the destination.
        {
            // Floor at y = 0 (normal up), a raised step at y = 0.2 for x > 1, a wall at x = 3 (normal -x).
            CollisionWorld world;
            world.add(quadItem({{-5, 0, -5, -5, 0, 5, 1, 0, 5, 1, 0, -5}}));
            world.add(quadItem({{1, 0.2f, -5, 1, 0.2f, 5, 5, 0.2f, 5, 5, 0.2f, -5}}));
            world.add(quadItem({{3, 0, 5, 3, 4, 5, 3, 4, -5, 3, 0, -5}}));
            GroundProbe probe;
            probe.forwardZ = 1;
            probe.dt = 0.033f;
            probe.rootDelta = 0.2f;
            probe.y = 0.003f;
            require(groundHeight(world, probe) && *groundHeight(world, probe) == 0.003f, "height within 0.01 of the floor is kept");
            probe.y = 0.1f;
            const auto lowered = groundHeight(world, probe);
            require(lowered && std::fabs(*lowered) < 0.01f, "floor in reach pulls the height down to the bisected floor");
            probe.y = 0.5f;
            require(!groundHeight(world, probe), "no floor in reach");
            probe.x = 1.3f;
            probe.y = 0.0f;
            const auto raised = groundHeight(world, probe);
            require(raised && std::fabs(*raised - 0.2f) < 0.01f, "step reached by the footprint raises the height");
            probe.x = -1;
            probe.y = 0.0f;
            probe.dt = 0.0f;
            probe.rootDelta = 0.0f;
            require(groundHeight(world, probe) && *groundHeight(world, probe) == 0.0f, "flat floor at rest");

            float dx = 0.5f, dz = 0.2f;
            require(slideAlongWalls(world, 2.0f, 0.0f, 0.0f, 0.0f, 1.0f, dx, dz) == 2 && std::fabs(dx) < 1e-6f && near(dz, 0.2f),
                    "a wall at the destination removes the movement into it");
            dx = 0.5f;
            dz = 0.2f;
            require(slideAlongWalls(world, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, dx, dz) == 0 && near(dx, 0.5f), "no wall within reach");
            dx = -0.5f;
            dz = 0.0f;
            require(slideAlongWalls(world, 2.9f, 0.0f, 0.0f, 0.0f, 1.0f, dx, dz) == 0 && near(dx, -0.5f), "movement away from the wall");
        }

        // Placed objects (docs/re/collision.md): an oriented box becomes the 12 triangles of 0x21ba50 in world coordinates, a
        // collision item is moved by the node matrix, and the layers of the queries pick entries by mask (player {3, 0x90}, camera {all, 0x30}).
        {
            require(kPlayerLayer.accepts(1) && kPlayerLayer.accepts(6) && kPlayerLayer.accepts(38) && !kPlayerLayer.accepts(4) &&
                        !kPlayerLayer.accepts(17),
                    "player layer: bits 0 or 1 without bit 4 or 7");
            require(kCameraLayer.accepts(1) && kCameraLayer.accepts(4) && kCameraLayer.accepts(6) && !kCameraLayer.accepts(17) &&
                        !kCameraLayer.accepts(38),
                    "camera layer: everything without bit 4 or 5");
            // Box centre (0, 1, 0), half (1, 1, 0.5), yaw 90 degrees (row vectors: x axis -> (0, 0, -1), z axis -> (1, 0, 0)), at x = 10.
            chase::assets::ObjectCollision box;
            box.matrix = {0, 0, -1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 10, 0, 0, 1};
            box.centre[1] = 1.0f;
            box.half[0] = 1.0f;
            box.half[1] = 1.0f;
            box.half[2] = 0.5f;
            CollisionWorld world;
            world.addObject(box);
            require(world.entries().size() == 1 && world.entries()[0].box && world.triangleCount() == 12 && world.entries()[0].mask == 6,
                    "a box registers 12 triangles with mask 6");
            const auto& root = world.entries()[0].item.nodes[0];
            require(near(root.centre[0], 10.0f) && near(root.centre[1], 1.0f) && near(root.centre[2], 0.0f) && near(root.half[0], 0.5f) &&
                        near(root.half[1], 1.0f) && near(root.half[2], 1.0f),
                    "the box turned by 90 degrees spans 0.5 in x and 1 in z");
            const auto& top = world.entries()[0].item.triangles[10];
            require(near(top.normal[1], 1.0f) && near(top.d, 2.0f), "face 10 of the table is the top, plane y = 2");
            float dx = -0.5f, dz = 0.0f;
            require(slideAlongWalls(world, 11.0f, 0.0f, 0.0f, 0.0f, 1.0f, dx, dz) > 0 && near(dx, 0.0f), "the player slides along the box");
            CollisionWorld vehicleLayer;
            box.mask = 17;
            vehicleLayer.addObject(box);
            dx = -0.5f;
            dz = 0.0f;
            require(slideAlongWalls(vehicleLayer, 11.0f, 0.0f, 0.0f, 0.0f, 1.0f, dx, dz) == 0 && near(dx, -0.5f), "mask 17 is not part of the player query");

            // A collision item (a floor quad at y = 0 over x, z in [-1, 1]) turned by 180 degrees and lifted by 3 becomes a ceiling at y = 3.
            auto item = std::make_shared<chase::assets::CollisionItem>(quadItem({{-1, 0, -1, -1, 0, 1, 1, 0, 1, 1, 0, -1}}));
            item->nodes[0].half[0] = item->nodes[0].half[1] = item->nodes[0].half[2] = 1.0f;
            chase::assets::ObjectCollision mesh;
            mesh.mesh = item;
            mesh.matrix = {-1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 0, 0, 3, 0, 1};
            CollisionWorld placed;
            placed.addObject(mesh);
            const auto& moved = placed.entries()[0].item;
            require(!placed.entries()[0].box && moved.triangles.size() == 2 && near(moved.triangles[0].normal[1], -1.0f) &&
                        near(moved.triangles[0].d, -3.0f) && near(moved.vertices[1], 3.0f),
                    "an item is moved by the node matrix: vertices, normals and plane offsets");
        }

        // Follow camera (docs/re/camera.md). Spline 0x1e3730 through (0, a), (0.5, b), (1, c); stick and shoulder
        // turn increments of 0x200ab0..0x200b18; reference pose and rates read from the running game at the
        // gameplay-start checkpoint (tools/oracle/camera-trace.txt, k_still / k_R, dt 0.033).
        require(near(quadraticSpline(3.0f, 10.0f, 15.0f, 0.5f), 10.0f) && near(quadraticSpline(3.0f, 10.0f, 15.0f, 0.0f), 3.0f) &&
                    near(quadraticSpline(3.0f, 10.0f, 15.0f, 1.0f), 15.0f),
                "camera spline passes through its three values");
        require(stickTurnsCamera(1) && stickTurnsCamera(5) && stickTurnsCamera(0x11c) && !stickTurnsCamera(0) && !stickTurnsCamera(6) &&
                    !stickTurnsCamera(0x100),
                "stick turns the camera for types 1..5 and 0x11c");
        require(near(stickTurnInput(1.0f, 3), 1.0f) && near(stickTurnInput(-1.0f, 3), -1.0f) && stickTurnInput(0.25f, 3) == 0.0f &&
                    stickTurnInput(-0.25f, 3) == 0.0f && near(stickTurnInput(0.5f, 3), 0.25f * 1.3333334f) && stickTurnInput(1.0f, 0x100) == 0.0f,
                "stick turn increment: dead zone 0.25, gain 4/3");
        require(shoulderTurnInput(0, 1, false) == 2.0f && shoulderTurnInput(1, 0, false) == -2.0f && shoulderTurnInput(1, 1, false) == 0.0f &&
                    shoulderTurnInput(0, 1, true) == -2.0f,
                "shoulder turn increment and invert option");
        {
            const float target[3] = {886.764f, 16.589f, 1111.166f};
            const float yaw = -2.3501f;
            FollowCamera camera;
            camera.reset(target, yaw, yaw);
            const float expectedEye[3] = {893.878f, 20.489f, 1118.194f}, expectedLook[3] = {886.764f, 18.789f, 1111.166f};
            for (int i = 0; i < 3; ++i)
                require(std::fabs(camera.eye()[i] - expectedEye[i]) < 2e-3f && std::fabs(camera.lookAt()[i] - expectedLook[i]) < 2e-3f,
                        "reset pose behind the player: 10 away, 1.7 above the look point, look point 2.2 above the feet");
            require(std::fabs(camera.yaw() - yaw) < 1e-4f, "reset yaw");

            FollowCameraInput still;
            still.dt = 0.033f;
            for (int i = 0; i < 3; ++i) still.target[i] = target[i];
            still.targetFacing = yaw;
            camera.update(still);
            for (int i = 0; i < 3; ++i) require(std::fabs(camera.eye()[i] - expectedEye[i]) < 2e-3f, "an idle player leaves the camera at rest");

            FollowCameraInput turn = still;
            turn.shoulderTurn = shoulderTurnInput(0, 1, false);
            camera.update(turn);
            require(std::fabs(camera.yaw() - (yaw + 0.0396f)) < 1e-3f, "R turns the view by 2 * dt * 0.6 per update (k_R: -2.3501 -> -2.3105)");

            camera.reset(target, yaw, yaw);
            FollowCameraInput step = still;
            step.target[0] += 1.0f;
            camera.update(step);
            require(std::fabs(camera.lookAt()[0] - (expectedLook[0] + (1.0f - std::pow(0.75f, 0.99f)))) < 1e-4f,
                    "look point follows the player by 1 - 0.75^(30 dt) = 0.2478 per update");
            for (int i = 0; i < 400; ++i) camera.update(step);
            const float dx = step.target[0] - camera.eye()[0], dz = step.target[2] - camera.eye()[2];
            require(std::fabs(std::sqrt(dx * dx + dz * dz) - 10.0f) < 1e-3f && std::fabs(camera.eye()[1] - (camera.lookAt()[1] + 1.7f)) < 1e-3f,
                    "settled camera is 10 away horizontally and 1.7 above the look point");
        }

        // Falling and landing (docs/re/player.md falling entry): gravity 0.05 * 30 * dt per update up to 0.8,
        // probe distance max(|speed|, 0.05), hard landing from a fall distance of 12, stun by elapsed time.
        {
            CollisionWorld world;
            world.add(quadItem({{-5, 0, -5, -5, 0, 5, 50, 0, 5, 50, 0, -5}}));
            VerticalState state;
            state.landingAnimations = {{13.0f, 15.0f}, {18.0f, 15.0f}};
            state.y = 40;
            VerticalInput input;
            input.x = -20;
            input.rootDelta = 0.2f;
            input.dt = 0.033f;
            input.ticks = 99;
            input.forwardZ = 1;
            verticalUpdate(state, world, input);
            require(state.posture == Posture::Falling && near(state.fallSpeed, 0.0495f) && near(state.y, 40 - 0.0495f) && near(state.fallDistance, 0.0495f),
                    "first airborne update: speed 0.0495, y -= speed");
            verticalUpdate(state, world, input);
            require(near(state.fallSpeed, 0.099f) && near(state.fallDistance, 0.1485f), "speed grows by 1.5 * dt per update");
            for (int i = 0; i < 30; ++i) verticalUpdate(state, world, input);
            require(state.fallSpeed == kTerminalFallSpeed, "terminal speed 0.8 per update");

            input.x = 0;
            state.y = 0.3f;
            state.fallSpeed = 0.2f;
            state.fallDistance = 5;
            VerticalStep step = verticalUpdate(state, world, input);
            require(step.landed && !step.hard && state.posture == Posture::Landing && state.fallSpeed == 0 && std::fabs(state.y) < 0.01f,
                    "ground within max(|speed|, 0.05) + 0.1 below lands; no stick: soft landing stun");
            int updates = 1;
            while (!verticalUpdate(state, world, input).stunEnded) ++updates;
            require(updates == 27 && state.posture == Posture::Ground,
                    "soft stun ends when ticks * 15 / 3000 >= 13: 27 updates of 99 ticks (2673 ticks = 13.365 frames, 2574 = 12.87)");
            require(stunFinished({13.0f, 15.0f}, 2600) && !stunFinished({13.0f, 15.0f}, 2599), "13 frames at 15 per second need 2600 ticks");

            state.posture = Posture::Falling;
            state.y = 0.3f;
            state.fallSpeed = 0.2f;
            state.fallDistance = 5;
            input.moveRequested = true;
            step = verticalUpdate(state, world, input);
            require(step.landed && state.posture == Posture::Ground, "soft landing with the stick held has no stun");

            state.posture = Posture::Falling;
            state.y = 0.3f;
            state.fallSpeed = 0.2f;
            state.fallDistance = 12.0f;
            step = verticalUpdate(state, world, input);
            require(step.hard && state.posture == Posture::Landing && state.landingAnimation.lastFrame == state.landingAnimations.hard.lastFrame, "fall distance 12 lands hard, also with the stick held");

            state = VerticalState();
            state.y = 0;
            input.x = 0;
            input.rootDelta = 0;
            verticalUpdate(state, world, input);
            require(state.posture == Posture::Ground && state.y == 0, "a standing character on the ground is not probed");
        }

        // Jump root motion (docs/re/player.md): the profile samples the animation at its rate, linear between
        // frames, held after the last frame; B starts it from the ground.
        {
            JumpProfile profile;
            for (int frame = 0; frame < 12; ++frame) profile.offsets.push_back(0.5f * float(frame));
            require(near(profile.duration(), 11.0f / 15.0f) && near(profile.offsetAt(0.1f), 0.75f) && near(profile.offsetAt(5.0f), 5.5f),
                    "linear between frames at 15 frames per second, held after the last frame");

            CollisionWorld world;
            world.add(quadItem({{-50, 0, -50, -50, 0, 50, 50, 0, 50, 50, 0, -50}}));
            VerticalState state;
            VerticalInput input;
            input.dt = 0.1f;
            input.startJump = &profile;
            VerticalStep step = verticalUpdate(state, world, input);
            require(step.jumpStarted && state.posture == Posture::Jumping && state.y == 0, "the press update starts the jump without moving");
            input.startJump = nullptr;
            verticalUpdate(state, world, input);
            require(near(state.y, 0.75f) && state.posture == Posture::Jumping, "the next update moves along the profile");
            for (int i = 0; i < 7; ++i) step = verticalUpdate(state, world, input);
            require(step.jumpEnded && state.posture == Posture::Falling && near(state.y, 5.5f), "a jump that runs out in the air starts to fall");

            const RootMotionSpeeds jump = jumpSpeeds(kCharacterSpeeds);
            require(near(jump.walk, 0.8f * kCharacterSpeeds.walk) && near(jump.run, 0.8f * kCharacterSpeeds.run) &&
                        jump.sprintScale == kCharacterSpeeds.sprintScale,
                    "horizontal speed after take-off is 0.8 of the ground speed");
        }
        {
            chase::assets::Placement shown, hidden;
            shown.instance = "Police_Plaque";
            hidden.instance = "Police_Plaque_Floor";
            chase::assets::PlacedObjectState state;
            state.disabled.insert("police_plaque_floor");
            const std::vector<bool> drawn = drawnPlacements({shown, hidden}, state);
            require(drawn.size() == 2 && drawn[0] && !drawn[1], "a disabled object is not drawn, an enabled one is");
            state.disabled.erase("police_plaque_floor");
            require(drawnPlacements({hidden}, state)[0], "EnableObject 1 draws the object again");
            chase::assets::CharacterObject goon, guard;
            goon.placement.instance = "1-5_Balcony_Goon_01";
            guard.placement.instance = "1-4_AI_Goon_01";
            state.disabled.insert("1-5_balcony_goon_01");
            const std::vector<bool> drawnCharacterFlags = drawnCharacters({goon, guard}, state);
            require(drawnCharacterFlags.size() == 2 && !drawnCharacterFlags[0] && drawnCharacterFlags[1], "a disabled character is not drawn");
            require(nestedObjectDrawn(true) && !nestedObjectDrawn(false), "a nested object is drawn while its owner is");
        }
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
    std::cout << "runtime-core: all checks passed\n";
    return 0;
}
