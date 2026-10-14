#pragma once
// State 10 of the AI state table, the update at 0x2c5bc4 (docs/re/ai-states.md): the state AIFaceTarget requests through
// 0x211110. Each tick the node's x and z are put back to the home position (`+0xb8` / `+0xc0`, the node position when the
// state was requested) and, while a target exists, the heading u16 at `+6` and `+10` is int(atan2(dx, dz) * 10430.378)
// of the vector from the character to the target. No SDL dependency.
#include <array>
#include <cstdint>

namespace chase::runtime {

struct FaceTargetResult {
    std::array<float, 3> position;
    std::uint16_t heading = 0;
    bool facing = false;
};

FaceTargetResult stepFaceTarget(const std::array<float, 3>& position, const std::array<float, 3>& home, const std::array<float, 3>* target,
                                std::uint16_t previousHeading);

}
