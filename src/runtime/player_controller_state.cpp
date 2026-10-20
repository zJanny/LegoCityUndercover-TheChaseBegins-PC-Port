#include "runtime/player_controller_state.hpp"

namespace chase::runtime {

std::uint16_t playerControllerState(const VerticalState& vertical, int locomotionRole) {
    switch (vertical.posture) {
    case Posture::Jumping: return 4;
    case Posture::Falling: return 5;
    case Posture::Landing: return vertical.hardLanding ? 7 : 6;
    case Posture::Sliding: return 8;
    case Posture::Ground: break;
    }
    if (locomotionRole == 0) return kControllerStateIdle;
    if (locomotionRole == kAnimationWalk) return kControllerStateWalk;
    if (locomotionRole == kAnimationRun || locomotionRole == kAnimationSprint) return kControllerStateRun;
    return kControllerStateUnknown;
}

}
