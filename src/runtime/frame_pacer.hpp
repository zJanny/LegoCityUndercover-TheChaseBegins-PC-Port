#pragma once
#include <cstdint>

namespace chase::runtime {

constexpr std::uint32_t kPresentUnitTicks = 50;
constexpr std::uint32_t kGameplayPresentInterval = 2;
constexpr std::uint32_t kPresentSlackTicks = 25;

class FramePacer {
public:
    explicit FramePacer(std::uint32_t intervalUnits = kGameplayPresentInterval) : interval_(intervalUnits) {}

    bool due(std::uint32_t realTicks) const { return !started_ || static_cast<std::int32_t>(realTicks + kPresentSlackTicks - next_) >= 0; }

    std::uint32_t present(std::uint32_t realTicks) {
        const std::uint32_t span = interval_ * kPresentUnitTicks;
        if (!started_ || static_cast<std::int32_t>(realTicks - next_) > static_cast<std::int32_t>(span)) next_ = realTicks;
        next_ += span;
        started_ = true;
        clock_ += interval_ * kPresentUnitTicks;
        return clock_;
    }

    std::uint32_t clock() const { return clock_; }

private:
    std::uint32_t interval_;
    std::uint32_t next_ = 0;
    std::uint32_t clock_ = 0;
    bool started_ = false;
};

}
