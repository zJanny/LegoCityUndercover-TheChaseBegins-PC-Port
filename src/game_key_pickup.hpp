#pragma once
#include "runtime/carry.hpp"
#include <vector>

namespace game_key_pickup {

// One line per pickup, placing and drop of a carry object, for tests and developers.
void report(const chase::runtime::CarrySet& carry, const std::vector<chase::runtime::CarryAction>& actions, unsigned update);
// One line when an entity joins or leaves the carry arrow list 2.
void reportArrows(const std::vector<chase::runtime::ObjectiveArrow>& before, const std::vector<chase::runtime::ObjectiveArrow>& after, unsigned update);

}
