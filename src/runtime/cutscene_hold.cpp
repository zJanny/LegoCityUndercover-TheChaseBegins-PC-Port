#include "runtime/cutscene_hold.hpp"
#include <algorithm>

namespace chase::runtime {

void CutsceneHold::begin(std::size_t entity) {
    if (!holds(entity)) held_.push_back({entity, std::nullopt});
}

void CutsceneHold::begin(std::size_t entity, const Node& node) {
    if (!holds(entity)) held_.push_back({entity, node});
}

bool CutsceneHold::holds(std::size_t entity) const {
    return std::any_of(held_.begin(), held_.end(), [entity](const Entry& entry) { return entry.entity == entity; });
}

CutsceneHold::Released CutsceneHold::end(std::size_t entity) {
    const auto found = std::find_if(held_.begin(), held_.end(), [entity](const Entry& entry) { return entry.entity == entity; });
    if (found == held_.end()) return {};
    const Released released{true, found->node};
    held_.erase(found);
    return released;
}

bool drawnWhileHeld(bool drawnByState, bool held) { return drawnByState || held; }

}
