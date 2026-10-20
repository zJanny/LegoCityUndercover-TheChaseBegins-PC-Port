#include "runtime/hud_pending_add.hpp"

namespace chase::runtime {

std::optional<std::uint32_t> applyHudPendingAdd(HudPendingAdd& hud, unsigned updateCount, unsigned pickupStamp) {
    if (hud.pending == 0 || updateCount <= pickupStamp + kHudPendingAddDelay) return std::nullopt;
    const std::uint32_t amount = hud.pending;
    hud.pending = 0;
    return amount;
}

}
