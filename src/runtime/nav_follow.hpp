#pragma once
// The path follower 0x1faf04 of an AIMove on the nav grids (docs/re/ai-navigation.md). It dispatches on the phase byte of
// the path record: 0 arrived, 2 in-grid following (0x27ddb4, with the rebuild of the cell path at 0x27e12c and the grid
// transition 0x272598), 3 walking to the cell across a link (0x1fb104), 4 walking to a grid the start position lies beside
// (0x1faf70), anything else blocked. No SDL dependency.
#include <array>
#include <cstdint>

#include "runtime/nav_path.hpp"

namespace chase::runtime {

// Low byte of the value returned by 0x1faf04 / 0x27ddb4 / 0x272598: 0 arrived at the goal cell of the goal grid, 1 or 2
// cannot continue, 3 keep the current heading, 4 walk toward `target` with the heading byte `heading`, 5 + n the same
// across a link with gate n (gate 0 is a plain link; its handler 0x2a07b0 is the walk of code 4).
struct NavFollowResult {
    int code = 1;
    std::uint8_t heading = 0;
    std::array<float, 3> target{};  // path record +0x58, written with code 4 and the codes from 5
};

// `airborne` is the second argument of 0x1faf04 (bit 6 of component byte 0x314, read only when the position left the
// grid after a transition). The dynamic obstacle pass 0x2af1cc is skipped (stand-in SCRIPT-S95).
NavFollowResult navFollow(NavWorld& world, NavPathRecord& record, const float position[3], bool airborne = false);

}
