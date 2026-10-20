// The controller state word the pedestrians' bump reads (src/runtime/player_controller_state.cpp). The state numbers are from
// docs/re/player-falling-jumping.md (states seen in the oracle locomotion traces); the bump counts 2 and 3 (0x1fd93c .. 0x1fd944).
#include "runtime/player_controller_state.hpp"
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void groundStatesFollowTheLocomotionRole() {
    VerticalState vertical;
    require(playerControllerState(vertical, 0) == 1, "idle is state 1");
    require(playerControllerState(vertical, kAnimationRun) == 2, "run is state 2");
    require(playerControllerState(vertical, kAnimationSprint) == 2, "a sprint is still the run state");
    require(playerControllerState(vertical, kAnimationWalk) == 3, "walk is state 3");
    require(playerControllerState(vertical, 151) == kControllerStateUnknown, "a carried run role is not known");
}

void airAndLandingStatesFollowThePosture() {
    VerticalState vertical;
    vertical.posture = Posture::Jumping;
    require(playerControllerState(vertical, kAnimationRun) == 4, "jump is state 4");
    vertical.posture = Posture::Falling;
    require(playerControllerState(vertical, 0) == 5, "fall is state 5");
    vertical.posture = Posture::Landing;
    require(playerControllerState(vertical, 0) == 6, "soft landing is state 6");
    vertical.hardLanding = true;
    require(playerControllerState(vertical, 0) == 7, "hard landing is state 7");
    vertical.posture = Posture::Sliding;
    require(playerControllerState(vertical, 0) == 8, "slide is state 8");
}
}

int main() {
    try {
        groundStatesFollowTheLocomotionRole();
        airAndLandingStatesFollowThePosture();
    } catch (const std::exception& error) {
        std::cerr << "player-controller-state-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "player-controller-state-test: ok\n";
    return 0;
}
