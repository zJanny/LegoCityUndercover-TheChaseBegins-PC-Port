#pragma once
// State 22 of the AI state table, the update at 0x2cbe14 behind AIFollowSimplePath (docs/re/ai-natives.md): the character
// walks straight at node `index` of a level path, takes the next node within 0.5 of it and, at the end of an open path,
// requests idle and raises the AIMove event with argument 255 (third raise site, 0x2cbf10). No SDL dependency.
#include <array>
#include <cstdint>
#include <vector>

namespace chase::runtime {

// Compared as the integer bit pattern of the float distance against 0x3f000000 at 0x2cbe8c.
constexpr float kSimplePathArrivalDistance = 0.5f;
constexpr std::uint16_t kSimplePathEndEventArgument = 255;

struct SimplePathFollow {
    std::vector<std::array<float, 3>> nodes;
    bool closed = false;
    bool reverse = false;
    std::uint16_t index = 0;
};

enum class SimplePathResult { Moving, Advanced, Ended };

// Next node index of 0x2cbe94..0x2cbef0 (reverse: index - 1, a closed path wraps to count - 2 after node 0; forward:
// index + 1, a closed path wraps to index + 1 - count). False when an open path has no next node.
bool nextSimplePathIndex(std::uint16_t index, std::uint16_t count, bool reverse, bool closed, std::uint16_t& next);

// One update of the state: the heading toward the current node is written to `heading` first (u16 at component +6,
// atan2(dx, dz) * 65536 / 2 pi truncated); then a horizontal distance (0x22b62c) below the arrival distance advances the
// index (Advanced) or ends the follow (Ended), otherwise the result is Moving.
SimplePathResult stepSimplePath(SimplePathFollow& follow, const std::array<float, 3>& position, std::uint16_t& heading);

}
