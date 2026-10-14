#pragma once
// State 11 of the AI state table, the update at 0x2c3bd8 (docs/re/ai-navigation.md): the state AISuperJump requests
// through 0x213300, which stores the target at `+0xb8` (x, y, z) and starts the jump locomotion (state 72). Each tick the
// state compares the horizontal distance with the ctrl+0x8c (ATTR_RunSpeed, 0.23 for the player; docs/re/ai-states.md) and the height with
// the target's, and raises the AIMove event (second raise site 0x2c3c68, start argument component byte 0) on arrival; the
// follow-up lookup 0x1f8d5c and idle are the caller's. No SDL dependency.
#include <array>
#include <cstdint>

namespace chase::runtime {

struct GoToTarget {
    std::array<float, 3> target{};
    float minDistance = -1.0f;
    // Bit 6 of component byte +0x314: a character below the target's height has arrived.
    bool arriveBelowTarget = false;
};

enum class GoToTargetResult { Moving, Arrived };

// 0x2c3bd8: arrived when MinDistance is greater than the horizontal distance (0x229708, y ignored), or when the character's
// height is below the target's and bit 6 is set. Otherwise the heading u16 (component +6) is atan2(dx, dz) * 10430.378
// truncated; the caller sets bit 0 of +0xc (move wanted).
GoToTargetResult stepGoToTarget(const GoToTarget& order, const std::array<float, 3>& position, std::uint16_t& heading);

}
