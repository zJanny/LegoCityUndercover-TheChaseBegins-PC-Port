#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
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

constexpr std::uint8_t kPedestrianObjectKind = 197;

const std::array<UpdatePhaseInfo, kUpdatePhaseCount>& updatePhases();

class UpdateSequence {
public:
    void at(UpdatePhase phase, std::function<void()> step);
    void atObjectKind(std::uint8_t kind, std::function<void()> step);
    void run();

private:
    struct Step {
        std::uint8_t kind = 0;
        std::function<void()> run;
    };
    std::array<std::vector<Step>, kUpdatePhaseCount> steps_;
};

}
