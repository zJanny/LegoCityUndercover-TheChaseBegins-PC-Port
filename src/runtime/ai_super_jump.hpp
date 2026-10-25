#pragma once
// Setup of the AISuperJump flight, the function 0x213300 (docs/re/ai-states.md): it stores the target at `+0xb8`, the
// heading `+6`, the horizontal distance per frame `+0x308` and the climb `+0x30c`, starts locomotion state 72 and requests
// state 11 (ai_go_to_target.hpp). No SDL dependency.
#include <array>
#include <cstdint>

namespace chase::runtime {

constexpr std::int32_t kSuperJumpNativeFrames = 28;

struct SuperJumpPlan {
    std::uint16_t heading = 0;
    std::uint32_t frames = 0;
    float horizontalPerFrame = 0;
    float climb = 0;
};

// 0x213300(entity, target, frames, height). A target above the character (vertical offset > 0) replaces `frames` by
// int((pi - asin(dy / (dy + height))) * 8.9126), the climb is max(dy, 0) + height.
SuperJumpPlan planSuperJump(const std::array<float, 3>& position, const std::array<float, 3>& target, std::int32_t frames, float height);

// Locomotion state 72 ("SUPERJUMP START", enter 0x26bca0) requests state 73 ("SUPERJUMP", update 0x28e4cc); its move 0x20cd30 takes the
// horizontal step of 0x203d18 (branch 0x20402c): x += (+0x308 * 30) * (dt * sin(facing)), z += (+0x308 * 30) * (dt * cos(facing)), the facing
// ctrl+10 in units of 2 pi / 65536 (literal 0x38c90fdb), no clamp at the target. The facing is the heading state 11 sets
// (ctrl+6): in the oracle ctrl+0xa equalled ctrl+6 from the first move update of the jump (RPC 20261025-garage-C-1); the turn rule is not read.
constexpr float kSuperJumpStepRate = 30.0f;                    // literal 0x41f00000 at 0x204014
constexpr float kHeadingUnitRadians = 9.58738019107841e-05f;  // literal 0x38c90fdb at 0x20401c
struct SuperJumpStep {
    float x = 0;
    float z = 0;
};
SuperJumpStep stepSuperJumpHorizontal(const SuperJumpPlan& plan, std::uint16_t facing, float dt);

}
