#include "runtime/buildit.hpp"

#include <cmath>

namespace chase::runtime {
namespace {

constexpr std::uint16_t phase(BuilditPhase value) { return static_cast<std::uint16_t>(value); }

void resetBricks(BuilditState& build) {
    build.rate = kBuilditStartRate;
}

void check(BuilditState& build) {
    switch (build.state) {
    case 8: {
        const std::size_t index = static_cast<std::size_t>(build.bit);
        if (build.bit >= 0 && index < build.progress.size() && build.progress[index] >= 1.0f) {
            build.target = phase(BuilditPhase::BitDone);
            return;
        }
        if (build.bit >= 0 && index < build.progress.size()) build.progress[index] += build.rate;
        build.rate += kBuilditRateStep;
        if (build.rate > kBuilditRateMax) build.rate = kBuilditRateMax;
        return;
    }
    case 9:
        build.target = build.bit == static_cast<int>(build.progress.size()) - 1 ? phase(BuilditPhase::Finishing) : phase(BuilditPhase::Ready);
        return;
    case 10:
        build.spin += build.spin >= kBuilditSpinLateFrom ? kBuilditSpinStepLate : kBuilditSpinStep;
        if (build.spin >= kBuilditSpinEnd) build.target = phase(BuilditPhase::Finished);
        return;
    default:
        return;
    }
}

void change(BuilditState& build, std::vector<BuilditEvent>& events) {
    if (build.state == build.target) return;
    switch (build.target) {
    case 2:
        resetBricks(build);
        break;
    case 8:
        if (build.bit < 0) resetBricks(build);
        if (build.bit + 1 < static_cast<int>(build.progress.size())) ++build.bit;
        events.push_back(BuilditEvent::BrickStarted);
        break;
    case 9:
        if (build.bit >= 0 && static_cast<std::size_t>(build.bit) < build.progress.size()) build.progress[static_cast<std::size_t>(build.bit)] = 1.0f;
        events.push_back(BuilditEvent::BrickDone);
        break;
    case 10:
        events.push_back(BuilditEvent::FinalPropShown);
        break;
    case 11:
        for (float& value : build.progress) value = 0.0f;
        build.bit = -1;
        build.rate = kBuilditStartRate;
        build.spin = 0.0f;
        build.built = true;
        build.target = phase(BuilditPhase::Ready);
        events.push_back(BuilditEvent::Built);
        break;
    default:
        break;
    }
    build.state = build.target;
}

}

std::vector<BuilditEvent> builditUpdate(BuilditState& build) {
    std::vector<BuilditEvent> events;
    if (build.state == build.target) check(build);
    change(build, events);
    return events;
}

int builditUse(BuilditState& build, bool buttonHeld) {
    const bool takes = build.state == phase(BuilditPhase::Ready) || build.state == 3 || build.state == 4;
    if (!takes) return build.state >= phase(BuilditPhase::Finishing) ? 2 : 1;
    if (buttonHeld) build.target = phase(BuilditPhase::Building);
    return 1;
}

float builditPropRise(float spinDegrees) {
    return std::sin(spinDegrees * 0.0174532925f);
}

}
