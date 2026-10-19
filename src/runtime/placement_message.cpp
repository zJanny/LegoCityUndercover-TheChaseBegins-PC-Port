#include "runtime/placement_message.hpp"

namespace chase::runtime {

void placePlayerEffects(PlacementEffects& effects, bool inVehicle) {
    effects.settleStepPending = true;
    if (inVehicle) return;
    effects.defaultStateRequested = true;
    effects.nodeShown = true;
    effects.entityEnabled = true;
    effects.nodeShowPending = true;
}

bool takeNodeShow(PlacementEffects& effects) {
    const bool pending = effects.nodeShowPending;
    effects.nodeShowPending = false;
    return pending;
}

void locomotionStepEnded(PlacementEffects& effects) { effects.settleStepPending = false; }

}  // namespace chase::runtime
