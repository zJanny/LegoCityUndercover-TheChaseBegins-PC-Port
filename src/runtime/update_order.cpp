#include "runtime/update_order.hpp"

#include <utility>

namespace chase::runtime {

const std::array<UpdatePhaseInfo, kUpdatePhaseCount>& updatePhases() {
    static const std::array<UpdatePhaseInfo, kUpdatePhaseCount> phases = {{
        {UpdatePhase::Systems, "system list slot +0x48 (0x169374)", 0x2e5488, false},
        {UpdatePhase::Runner, "script runner", 0x2e54b4, false},
        {UpdatePhase::Objects, "entity pass, classes below priority 254", 0x2e5530, false},
        {UpdatePhase::Characters, "entity pass, characters (priority 254)", 0x2e5530, false},
        {UpdatePhase::AfterCharactersStandIn, "updates whose place is not read (STAND-IN SCRIPT-S284)", 0x2e55b0, true},
        {UpdatePhase::UseSpots, "use spot update", 0x2e55b0, false},
    }};
    return phases;
}

void UpdateSequence::at(UpdatePhase phase, std::function<void()> step) {
    steps_[static_cast<std::size_t>(phase)].push_back(std::move(step));
}

void UpdateSequence::run() {
    for (auto& phase : steps_) {
        for (auto& step : phase) step();
        phase.clear();
    }
}

}
