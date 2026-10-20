#pragma once
// The pending add of the HUD update 0x2a5f80 (0x2a6068..0x2a60c4, docs/re/super-bricks.md entry 2026-10-20 "The pickup of super debris bricks"):
// a non-zero amount at HUD +0x2ac is added to the shown count +0x2a8, the save copy u16 0x5181a0 + 0x30 and the counter 0x593430 once the
// update count is above the pickup stamp +0x2a0 plus 35; +0x2ac becomes 0 and +0x2a4 (the bounce stamp) the update count. No code writes a
// non-zero amount to +0x2ac (only the HUD setup 0x290df0 clears it), so chase-game does not call this. No SDL dependency.
#include <cstdint>
#include <optional>

namespace chase::runtime {

inline constexpr unsigned kHudPendingAddDelay = 35;  // 0x2a6080: stamp + 35, compared with `bls`

struct HudPendingAdd {
    std::uint32_t pending = 0;   // HUD +0x2ac
};

// The amount 0x2a5f80 applies in the update with `updateCount`, if any; the pending amount is cleared then.
std::optional<std::uint32_t> applyHudPendingAdd(HudPendingAdd& hud, unsigned updateCount, unsigned pickupStamp);

}
