// Frame scheduler 0x100f44 replica against values traced from the disassembly and read from the running game.
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
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
    std::cout << "runtime-core: all checks passed\n";
    return 0;
}
