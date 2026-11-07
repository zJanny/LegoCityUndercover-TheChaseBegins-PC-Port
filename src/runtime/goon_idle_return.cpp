#include "runtime/goon_idle_return.hpp"

#include <cmath>

namespace chase::runtime {

bool aiIdleGate(std::uint32_t clock) { return clock != 0; }

std::string aiSleepClipName(const std::string& path) {
    const std::size_t slash = path.rfind('/');
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

AiSleepStep aiSleepStep(const AiSleepInput& input, const std::function<bool()>& seesPlayer) {
    AiSleepStep step;
    step.bits = input.bits;
    if (input.bits == kAiSleepAwake || input.bits == kAiSleepAsleep) return step;
    step.awake = false;
    if (input.bits == kAiSleepWaking) {
        if (input.clipEnded) step.bits = kAiSleepAwake;
        return step;
    }
    if (input.locomotionState > kAiSleepLocomotionLimit) {
        step.bits = kAiSleepAwake;
        step.awake = true;
        return step;
    }
    if (!input.friendly && seesPlayer && seesPlayer()) {
        if (input.hasWakeClip) {
            step.bits = kAiSleepWaking;
            step.clip = AiSleepClip::Wake;
        } else {
            step.bits = kAiSleepAwake;
            step.awake = true;
        }
        return step;
    }
    step.bits = kAiSleepAsleep;
    step.clip = AiSleepClip::Sleep;
    return step;
}

bool aiReturnRequested(bool waitObject, bool returnToSpawnPoint, const std::array<float, 3>& position, const std::optional<std::array<float, 3>>& spawn) {
    if (waitObject) return true;
    if (!returnToSpawnPoint || !spawn) return false;
    const float dx = position[0] - (*spawn)[0], dz = position[2] - (*spawn)[2];
    return !(kAiReturnDistance > std::sqrt(dx * dx + dz * dz));
}

std::uint32_t aiReturnCountdown(bool waitObject, const AiIdleReturnParams& params) {
    return std::uint32_t(waitObject ? params.waitTime : kAiReturnWaitDefault);
}

bool aiReturnWaitStep(bool waitObject, bool returnToSpawnPoint, std::uint32_t& countdown) {
    if (!waitObject && !returnToSpawnPoint) return false;
    const std::int32_t left = std::int32_t(countdown) - 1;
    countdown = std::uint32_t(left);
    return left <= 0;
}

}
