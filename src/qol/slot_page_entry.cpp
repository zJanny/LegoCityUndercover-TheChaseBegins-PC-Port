#include "qol/slot_page_entry.hpp"

namespace qol {

SlotPageEntry::Step SlotPageEntry::step(const Keys& keys, unsigned slotCount, unsigned selectedSlot, bool pageActive, float dt) {
    Step result;
    if (!pageActive || keys.pick) {
        focused_ = false;
        seconds_ = 0;
        return result;
    }
    if (!focused_) {
        if (keys.down && slotCount > 0 && selectedSlot + 1 >= slotCount) {
            focused_ = true;
            seconds_ = 0;
        }
        return result;
    }
    result.swallow = true;
    if (keys.up) {
        focused_ = false;
        seconds_ = 0;
    } else if (keys.activate) {
        result.open = true;
    } else {
        seconds_ += dt;
    }
    return result;
}

}
