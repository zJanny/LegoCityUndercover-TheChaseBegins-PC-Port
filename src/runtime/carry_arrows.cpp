#include "runtime/carry_arrows.hpp"

namespace chase::runtime {

CarryArrowUpdate carryArrows(const CarrySet& set, const assets::PlacedObjectState& state, const float player[3], bool hintActive) {
    CarryArrowUpdate update;
    for (const CarryObject& object : set.objects()) {
        if (!object.usable || state.isDisabled(object.instance)) continue;
        if (object.carried) update.requests.push_back({object.instance, false});
        else if (!object.freeplay) update.requests.push_back({object.instance, true});
    }
    const int carried = set.carriedIndex();
    for (const CarryTarget& target : set.targets()) {
        if (state.isDisabled(target.instance)) continue;
        const bool holdsUseObject = carried >= 0 && !target.useObject.empty() && set.objects()[std::size_t(carried)].instance == target.useObject;
        update.requests.push_back({target.instance, holdsUseObject});
    }
    if (hintActive)
        if (const std::optional<CarryUse> use = set.useAt(state, player))
            update.hide = use->target ? set.targets()[use->index].instance : set.objects()[use->index].instance;
    return update;
}

void applyCarryArrows(const CarryArrowUpdate& update, ScriptRunner& scripts) {
    for (const CarryArrowRequest& request : update.requests) scripts.setCarryGroupTarget(request.instance, request.enable);
    if (!update.hide.empty()) scripts.hideObjectiveArrow(update.hide);
}

}
