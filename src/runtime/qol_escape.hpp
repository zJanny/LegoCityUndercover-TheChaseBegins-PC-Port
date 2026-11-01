#pragma once
#include "input.hpp"

namespace chase::runtime {

inline constexpr Action kEscapeAction = Action::Start;

inline void bindEscape(InputState& input, int escapeKeyCode) {
    input.fix(kEscapeAction, Source::Key, escapeKeyCode);
}

}
