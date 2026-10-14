#pragma once
// The path follower 0x1faf04 for a plain AIMove inside one nav grid (docs/re/ai-navigation.md). The follower dispatches on
// the phase byte of the path record: 0 arrived, 2 in-grid following (0x27ddb4, ported here), 1 blocked; the grid transition
// 0x272598 (phases 3) and the recovery for a start position beside a grid (phase 4) are not ported (SCRIPT-93), the
// follower then reports the blocked code 1. No SDL dependency.
#include <array>
#include <cstdint>

#include "runtime/nav_path.hpp"

namespace chase::runtime {

// Codes of the low byte returned by 0x1faf04 / 0x27ddb4: 0 arrived at the goal cell of the goal grid, 1 or 2 cannot
// continue, 3 keep the current heading, 4 walk toward `target` with the heading byte `heading`.
struct NavFollowResult {
    int code = 1;
    std::uint8_t heading = 0;
    std::array<float, 3> target{};  // path record +0x58, written with code 4
};

// 0x27ddb4 reached through 0x1faf04 phase 2, for a record whose avoid flag (+0x7c bit 0) is set, as every AIMove makes it
// (0x22d85c): the dynamic obstacle pass 0x2af1cc is skipped by that bit. A path across grids that needs the transition or the
// rebuild of 0x27e12c reports code 1.
NavFollowResult navFollow(NavWorld& world, NavPathRecord& record, const float position[3]);

}
