#pragma once
// The player controller's state word ctrl+0x6c as the pedestrians' bump reads it (0x1fd8c8, docs/re/pedestrians-inputs.md). No SDL.
#include "runtime/player_vertical.hpp"

#include <cstdint>

namespace chase::runtime {

constexpr std::uint16_t kControllerStateUnknown = 0;
constexpr std::uint16_t kControllerStateIdle = 1;
constexpr std::uint16_t kControllerStateRun = 2;
constexpr std::uint16_t kControllerStateWalk = 3;

// States on the ground 1 idle / 2 run / 3 walk, 4 jump, 5 fall, 6 soft and 7 hard landing, 8 slide (docs/re/player-falling-jumping.md).
// STAND-IN PED-S14: the port derives it from the posture and the playing locomotion role (a sprint is the run state); every state of the
// carry, the vehicle, an action or a script (any other role) is reported as unknown (0), which the bump does not count.
std::uint16_t playerControllerState(const VerticalState& vertical, int locomotionRole);

}
