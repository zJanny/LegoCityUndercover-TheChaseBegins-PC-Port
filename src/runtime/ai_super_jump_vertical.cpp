#include "runtime/ai_super_jump_vertical.hpp"

#include <algorithm>

namespace chase::runtime {

void AiSuperJumpVertical::update(const AiCharacter* player, bool flying, const assets::Animation& jumpRun, VerticalState& vertical) {
    const bool inFlight = flying && player && player->superJump;
    if (inFlight && !flying_) {
        profile_ = makeJumpProfile(jumpRun, player->superJump->plan.climb);
        vertical.posture = Posture::Jumping;
        vertical.jump = &profile_;
        vertical.jumpElapsed = 0;
        vertical.jumpBase = vertical.y;
        vertical.fallSpeed = 0;
        vertical.fallDistance = 0;
        previousY_ = vertical.y;
        lastDrop_ = descent_ = 0;
    } else if (inFlight) {
        lastDrop_ = std::max(0.0f, previousY_ - vertical.y);
        descent_ += lastDrop_;
        previousY_ = vertical.y;
    } else if (flying_ && vertical.posture == Posture::Jumping) {
        vertical.posture = Posture::Falling;
        vertical.jump = nullptr;
        vertical.fallSpeed = lastDrop_;
        vertical.fallDistance = descent_;
    }
    flying_ = inFlight;
}

}
