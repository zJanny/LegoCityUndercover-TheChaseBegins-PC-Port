// chase_runtime replicas (scheduler 0x100f44, pad pipeline) against values traced from the disassembly
// and read from the running game.
#include "runtime/pad.hpp"
#include "runtime/scheduler.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace chase::runtime;

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
bool near(float a, float b) { return std::fabs(a - b) < 1e-6f; }
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
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
    std::cout << "runtime-core: all checks passed\n";
    return 0;
}
