#pragma once

#include "runtime/ai.hpp"
#include "runtime/player_vertical.hpp"

namespace chase::runtime {

// The player's own jump and fall during an AISuperJump flight (locomotion state 73, docs/re/pushables.md): the jump_run clip scaled to the
// climb of the set-up 0x213300 lifts the node, and when the flight ends the jump becomes a fall that the ordinary vertical update lands.
// STAND-IN SCRIPT-S311: the fall speed and fall distance at the end of the flight are taken from the descent of the port's own jump
// (the original sets ctrl+0x2b8 = -ctrl+0x208 and keeps ctrl+0x2b4 itself, not read).
class AiSuperJumpVertical {
public:
    void update(const AiCharacter* player, bool flying, const assets::Animation& jumpRun, VerticalState& vertical);

private:
    bool flying_ = false;
    JumpProfile profile_;
    float previousY_ = 0, lastDrop_ = 0, descent_ = 0;
};

}
