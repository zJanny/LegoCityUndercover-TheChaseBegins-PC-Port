#pragma once
// Frame scheduler 0x100f44 (docs/re/arm-executable.md, 2026-10-13T10:35:31). No SDL dependency.
#include <cmath>
#include <cstdint>
#include <functional>

namespace chase::runtime {

// Client fields C+0x10..+0x22 and process field P+0x70. Defaults are the values read
// from the running game (oracle, 2026-10-13T10:35:31): dt clamped to 0.03..0.07 s, one step per call.
struct StepConfig {
    float minDt = 0.03f;
    float maxDt = 0.07f;
    float fixedDt = 0.0f;
    float scale = 1.0f;
    bool useFixedDt = false;
    std::uint32_t maxSteps = 1;
};

// Clock unit of the scheduler timer (frequency word 0xbb8 at P+0x18+0xc).
constexpr std::uint32_t kClockFrequency = 3000;

// The step the scheduler passes to an update is a whole number of clock ticks divided by the frequency.
inline std::uint32_t stepTicks(float dt) { return static_cast<std::uint32_t>(std::lround(dt * static_cast<float>(kClockFrequency))); }

class Scheduler {
public:
    explicit Scheduler(StepConfig config = {}, std::uint32_t start = 0, std::uint32_t frequency = kClockFrequency);

    // One call of 0x100f44 at clock value `now`; runs update(dt) zero or more times and returns the count.
    std::uint32_t advance(std::uint32_t now, const std::function<void(float)>& update);

    std::uint32_t last() const { return last_; }
    std::uint32_t timerTicks() const { return timerTicks_; }
    float lastDt() const { return lastDt_; }

private:
    StepConfig config_;
    std::uint32_t frequency_;
    std::uint32_t last_;
    std::uint32_t timerTicks_ = 0;
    float lastDt_ = 0.0f;
};

}
