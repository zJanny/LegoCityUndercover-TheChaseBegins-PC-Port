#pragma once
#include "frontend/pause_menu.hpp"
#include "input.hpp"

namespace chase::runtime {

inline constexpr Action kEscapeAction = Action::Start;
inline constexpr Action kEscapeBackAction = Action::B;

inline void bindEscape(InputState& input, int escapeKeyCode) {
    input.fix(kEscapeAction, Source::Key, escapeKeyCode);
}

inline Action escapeActionFor(frontend::PausePage page) {
    return page == frontend::PausePage::None ? kEscapeAction : kEscapeBackAction;
}

inline void routeEscape(InputState& input, int keyCode, frontend::PausePage page) {
    input.retargetFixed(Source::Key, keyCode, escapeActionFor(page));
}

}
