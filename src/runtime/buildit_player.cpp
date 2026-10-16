#include "runtime/buildit_player.hpp"

namespace chase::runtime {

void BuilditPlayer::update(const BuilditUse& use, bool actionPressed, bool actionHeld, bool free) {
    if (active_) {
        if (use.inZone && use.reply == 1 && actionHeld) {
            ++updates_;
            return;
        }
        active_ = false;
        return;
    }
    if (free && use.inZone && use.reply == 1 && (use.accepted || actionPressed)) {
        active_ = true;
        updates_ = 0;
    }
}

}
