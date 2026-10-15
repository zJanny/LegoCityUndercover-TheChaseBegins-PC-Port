#include "runtime/frame_pacer.hpp"
#include "runtime/scheduler.hpp"
#include "runtime/vehicle_body.hpp"

#include <cstdio>
#include <vector>

namespace runtime = chase::runtime;

namespace {
int failures = 0;
void check(bool ok, const char* what) {
    if (!ok) {
        std::printf("FAIL %s\n", what);
        ++failures;
    }
}

struct Run {
    std::vector<float> dts;
    int zeroStepUpdates = 0;
};

Run sixtyHertz(bool paced) {
    Run run;
    runtime::Scheduler scheduler({}, 0);
    runtime::FramePacer pacer;
    float leftover = 0.0f;
    for (std::uint32_t frame = 1; frame <= 600; ++frame) {
        const std::uint32_t real = frame * runtime::kPresentUnitTicks;
        std::uint32_t clock = real;
        if (paced) {
            if (!pacer.due(real)) continue;
            clock = pacer.present(real);
        }
        scheduler.advance(clock, [&](float dt) {
            run.dts.push_back(dt);
            if (runtime::vehicleWorldSteps(leftover, dt) == 0) ++run.zeroStepUpdates;
        });
    }
    return run;
}
}

int main() {
    const Run before = sixtyHertz(false);
    check(before.zeroStepUpdates > 0, "unpaced 60 Hz calls give updates without a vehicle world step (EXEC-5 cause)");

    const Run after = sixtyHertz(true);
    check(after.dts.size() == 300, "paced run: one update per two vblanks");
    check(after.zeroStepUpdates == 0, "paced run: no update without a vehicle world step");
    bool exact = true;
    for (float dt : after.dts) exact = exact && dt == 100.0f / 3000.0f;
    check(exact, "paced run: every update has dt 100/3000 s (oracle-rpc.md 2026-10-15T16:17: 100 ticks per world step)");

    runtime::FramePacer jitter;
    const std::uint32_t real = 1000;
    check(jitter.due(real), "first frame is due");
    jitter.present(real);
    check(!jitter.due(real + 50), "one vblank later is not due");
    check(jitter.due(real + 75), "within the slack of the second vblank is due");
    jitter.present(real + 120);
    check(!jitter.due(real + 150) && jitter.due(real + 175), "a late frame does not shift the schedule");
    jitter.present(real + 200);
    jitter.present(real + 900);
    check(!jitter.due(real + 900) && jitter.due(real + 1000), "after a long stall the schedule restarts at the stalled frame");

    runtime::FramePacer single(1);
    single.present(0);
    check(single.present(50) == 100 && !single.due(50) && single.due(75), "interval 1 presents every vblank unit");

    if (failures) return 1;
    std::printf("frame-pacer ok: unpaced zero-step updates %d of %zu\n", before.zeroStepUpdates, before.dts.size());
    return 0;
}
