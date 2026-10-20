#pragma once
// The HUD stud counter after a level load (docs/re/save-format-state.md): the level load 0x2e5e64 copies the studs of the slot
// (+0x5148) to 0x593418 and calls 0x22ddf4(0x519760, bits 2..4 of slot +0x520b, studs), the Studs bar creation 0x15b2f0 and the
// HUD page 0x2cedb8 reset the type to 0 when the counter is 0, and the call shows the bar for 2 s with the studs as its value.
#include <cstdint>

namespace chase::runtime {

struct HudStudCounter {
    unsigned type = 0;
    std::uint64_t total = 0;
};

// `slotType`: bits 2..4 of slot +0x520b. The result's type is also what 0x22ddf4 writes back into those bits.
HudStudCounter hudStudCounterAtLoad(unsigned slotType, std::uint64_t studs);

}
