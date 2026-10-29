#include "runtime/pad_events.hpp"

namespace chase::runtime {

bool feedPadButton(InputState& input, int button, bool down) {
    if (!padButtonName(button)) return false;
    input.event(Source::Pad, button, down);
    return true;
}

}
