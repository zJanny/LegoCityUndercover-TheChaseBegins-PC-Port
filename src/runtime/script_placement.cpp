#include "runtime/script_placement.hpp"

#include <algorithm>

namespace chase::runtime {

bool entityHasController(std::uint8_t kind, std::uint32_t flags, const std::vector<std::uint8_t>& kindList) {
    if (kind == kPlayerEntityKind || kind == kSecondControllerEntityKind) return true;
    if (std::find(kindList.begin(), kindList.end(), kind) != kindList.end()) return true;
    return (flags & kEntityFlagController) != 0;
}

void resetControllerAfterPlacement(VerticalState& state, bool inVehicle) {
    state.fallDistance = 0.0f;
    state.fallSpeed = 0.0f;
    placePlayerEffects(state.placement, inVehicle);
}

bool isProbeCharacterKind(std::uint8_t kind) { return kind == 179 || kind == 183 || kind == 243; }

void placementProbeEnd(const float start[3], float out[3]) {
    for (int axis = 0; axis < 3; ++axis) out[axis] = start[axis] + kPlacementProbeDirection[axis] * kPlacementProbeLength;
}

}  // namespace chase::runtime
