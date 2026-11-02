#include "runtime/scheduler.hpp"

namespace chase::runtime {
namespace {
std::uint32_t toTicks(std::uint32_t frequency, float seconds) {
    return static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<float>(frequency) * seconds));
}
}

float scaledStepDt(float realDt, float scale) {
    const std::uint32_t scaled = toTicks(stepTicks(realDt), scale);
    const std::uint32_t ticks = scaled <= 1 ? 1 : scaled;
    return static_cast<float>(ticks) / static_cast<float>(kClockFrequency);
}

Scheduler::Scheduler(StepConfig config, std::uint32_t start, std::uint32_t frequency)
    : config_(config), frequency_(frequency), last_(start) {}

std::uint32_t Scheduler::advance(std::uint32_t now, const std::function<void(float)>& update) {
    if (last_ >= now) return 0;
    const std::uint32_t minTicks = toTicks(frequency_, config_.minDt);
    const std::uint32_t maxTicks = toTicks(frequency_, config_.maxDt);
    const std::uint32_t fixedTicks = toTicks(frequency_, config_.fixedDt);
    const std::uint32_t elapsed = now - last_;
    std::uint32_t steps = maxTicks ? (elapsed + maxTicks - 1) / maxTicks : 0;
    std::uint32_t period;
    if (steps > config_.maxSteps) {
        steps = config_.maxSteps;
        period = config_.useFixedDt ? fixedTicks : maxTicks;
    } else {
        period = steps ? elapsed / steps : 0;
        if (config_.useFixedDt) period = fixedTicks;
        else if (period < minTicks) period = minTicks;
    }
    const std::uint32_t scaled = toTicks(period, config_.scale);
    const std::uint32_t ticks = scaled <= 1 ? 1 : scaled;
    const float dt = static_cast<float>(ticks) / static_cast<float>(frequency_);
    if (steps) {
        lastDt_ = dt;
        lastRealDt_ = static_cast<float>(period) / static_cast<float>(frequency_);
    }
    for (std::uint32_t i = 0; i < steps; ++i) {
        update(dt);
        timerTicks_ += ticks;
        last_ += period;
    }
    if (steps == config_.maxSteps && (config_.maxSteps >= 5 || last_ < now)) last_ = now;
    return steps;
}

}
