#include "runtime/save_hud_counter.hpp"

namespace chase::runtime {

HudStudCounter hudStudCounterAtLoad(unsigned slotType, std::uint64_t studs) {
    return {studs == 0 ? 0u : slotType, studs};
}

}
