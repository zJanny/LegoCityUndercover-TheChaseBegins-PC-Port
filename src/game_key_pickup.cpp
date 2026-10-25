#include "game_key_pickup.hpp"
#include "game_level.hpp"
#include "runtime/carry_arrows.hpp"
#include <algorithm>
#include <iostream>

namespace game_key_pickup {
using namespace chase;

void report(const runtime::CarrySet& carry, const std::vector<runtime::CarryAction>& actions, unsigned update) {
    for (const runtime::CarryAction& action : actions) {
        const char* what = action.kind == runtime::CarryActionKind::PickedUp ? "picked up"
                           : action.kind == runtime::CarryActionKind::Placed  ? "placed into "
                           : action.kind == runtime::CarryActionKind::Dropped ? "dropped"
                                                                              : nullptr;
        if (!what) continue;
        std::cout << "carry " << carry.objects()[action.object].instance << ": " << what;
        if (action.kind == runtime::CarryActionKind::Placed) std::cout << carry.targets()[action.target].instance;
        std::cout << " at update " << update << '\n';
    }
}

void reportArrows(const std::vector<runtime::ObjectiveArrow>& before, const std::vector<runtime::ObjectiveArrow>& after, unsigned update) {
    const auto listed = [](const std::vector<runtime::ObjectiveArrow>& arrows, const std::string& target) {
        return std::any_of(arrows.begin(), arrows.end(), [&](const runtime::ObjectiveArrow& arrow) { return arrow.target == target; });
    };
    for (const runtime::ObjectiveArrow& arrow : after)
        if (!listed(before, arrow.target)) std::cout << "carry arrow over " << arrow.target << " added at update " << update << '\n';
    for (const runtime::ObjectiveArrow& arrow : before)
        if (!listed(after, arrow.target)) std::cout << "carry arrow over " << arrow.target << " removed at update " << update << '\n';
}

}

namespace game_level {

void LevelSession::updateCarryArrows(const std::vector<chase::runtime::CarryAction>& actions, bool hintActive, const float playerPosition[3]) {
    game_key_pickup::report(carry, actions, updates);
    const std::vector<chase::runtime::ObjectiveArrow> before = levelScripts.objectives().arrows()[2];
    chase::runtime::applyCarryArrows(chase::runtime::carryArrows(carry, objectState, playerPosition, hintActive && !carry.busy()), levelScripts);
    game_key_pickup::reportArrows(before, levelScripts.objectives().arrows()[2], updates);
}

}
