#pragma once

namespace qol {

class SlotPageEntry {
public:
    struct Keys {
        bool up = false, down = false, activate = false, pick = false;
    };

    struct Step {
        bool swallow = false;
        bool open = false;
    };

    Step step(const Keys& keys, unsigned slotCount, unsigned selectedSlot, bool pageActive, float dt);
    bool focused() const { return focused_; }
    float seconds() const { return seconds_; }

private:
    bool focused_ = false;
    float seconds_ = 0;
};

}
