#pragma once

#include <array>
#include <cstddef>
#include <functional>
#include <vector>

namespace chase::runtime {

enum class UpdatePhase {
    Systems,
    Runner,
    Objects,
    Characters,
    AfterCharactersStandIn,
    UseSpots,
};

struct UpdatePhaseInfo {
    UpdatePhase phase;
    const char* name;
    unsigned callSite;
    bool standIn;
};

constexpr std::size_t kUpdatePhaseCount = 6;

const std::array<UpdatePhaseInfo, kUpdatePhaseCount>& updatePhases();

class UpdateSequence {
public:
    void at(UpdatePhase phase, std::function<void()> step);
    void run();

private:
    std::array<std::vector<std::function<void()>>, kUpdatePhaseCount> steps_;
};

}
