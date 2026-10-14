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

}
