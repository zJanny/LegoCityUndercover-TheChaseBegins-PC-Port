// chase_runtime replicas (scheduler 0x100f44, pad pipeline) against values traced from the disassembly
// and read from the running game.
#include "runtime/pad.hpp"
#include "runtime/player_intent.hpp"
#include "runtime/player_locomotion.hpp"
#include "runtime/scheduler.hpp"
#include <cmath>
#include <cstring>
#include <iostream>
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
        {
            LocomotionState state;
            StickIntent forward;
            forward.active = true;
            forward.heldSet = kHeldMove;
            LocomotionStep step;
            for (int update = 1; update <= 31; ++update) {
                step = locomotionUpdate(state, 0.033f, forward);
                require(std::fabs(step.deltaZ - kMeasuredRootMotionSpeeds.run * 0.033f) < 1e-6f && step.deltaX == 0.0f, "run step");
            }
            require(near(state.runTimer, 1.023f) && state.sprinting && step.animation == kAnimationSprint && step.animationScale == 1.5f, "timer passes 1.0 in update 31");
            step = locomotionUpdate(state, 0.033f, forward);
            require(std::fabs(step.deltaZ - kMeasuredRootMotionSpeeds.run * 1.5f * 0.033f) < 1e-6f, "sprint step is 1.5 x run");
            // Release: no movement, scale back to 1, timer kept; a new push restarts the timer.
            StickIntent released;
            step = locomotionUpdate(state, 0.033f, released);
            require(step.deltaZ == 0.0f && step.animation == 0 && step.animationScale == 1.0f && near(state.runTimer, 1.056f), "release keeps the timer");
            step = locomotionUpdate(state, 0.033f, forward);
            require(near(state.runTimer, 0.033f) && !state.sprinting && std::fabs(step.deltaZ - kMeasuredRootMotionSpeeds.run * 0.033f) < 1e-6f, "run re-entry restarts the timer at full run speed");
            // Walk freezes the timer and runs at walk speed; run re-entry restarts it.
            StickIntent walk = forward;
            walk.heldSet |= kHeldSlow;
            locomotionUpdate(state, 0.033f, forward);
            const float held = state.runTimer;
            step = locomotionUpdate(state, 0.033f, walk);
            require(state.runTimer == held && step.animation == kAnimationWalk && std::fabs(step.deltaZ - kMeasuredRootMotionSpeeds.walk * 0.033f) < 1e-6f, "walk keeps the timer");
            locomotionUpdate(state, 0.033f, forward);
            require(near(state.runTimer, 0.033f), "walk to run restarts the timer");
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
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
    std::cout << "runtime-core: all checks passed\n";
    return 0;
}
