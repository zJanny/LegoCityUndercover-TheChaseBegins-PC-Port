#include "runtime/ai_super_jump.hpp"

#include "runtime/player_intent.hpp"

#include <cmath>
#include <cstring>

namespace chase::runtime {
namespace {

float floatOf(std::uint32_t bits) {
    float value;
    std::memcpy(&value, &bits, sizeof value);
    return value;
}

}

SuperJumpPlan planSuperJump(const std::array<float, 3>& position, const std::array<float, 3>& target, std::int32_t frames, float height) {
    const float dx = target[0] - position[0];
    const float dy = target[1] - position[1];
    const float dz = target[2] - position[2];
    SuperJumpPlan plan;
    plan.heading = static_cast<std::uint16_t>(static_cast<std::int32_t>(std::atan2(dx, dz) * floatOf(0x4622f983u)));
    float squared = dx * dx;
    squared = squared + dz * dz;
    const float distance = std::sqrt(squared);
    if (dy > 0.0f) {
        const float ratio = dy / (dy + height);
        frames = static_cast<std::int32_t>((floatOf(0x40490fdbu) - std::asin(ratio)) * floatOf(0x410e9a53u));
    }
    plan.frames = static_cast<std::uint32_t>(frames);
    plan.horizontalPerFrame = distance / static_cast<float>(plan.frames);
    plan.climb = (dy > 0.0f ? dy : 0.0f) + height;
    return plan;
}

SuperJumpStep stepSuperJumpHorizontal(const SuperJumpPlan& plan, std::uint16_t facing, float dt) {
    const float angle = static_cast<float>(facing) * kHeadingUnitRadians;
    const float speed = plan.horizontalPerFrame * kSuperJumpStepRate;
    return {speed * (dt * gameSin(angle)), speed * (dt * gameCos(angle))};
}

}
