#include "runtime/mission13_end.hpp"

#include <algorithm>
#include <sstream>

namespace chase::runtime {

std::optional<std::string> AiMoveWatch::observe(bool moving, const float position[3], unsigned update) {
    if (moving == moving_) {
        if (moving || endUpdate_ == 0 || update != endUpdate_ + kSettleUpdates) return std::nullopt;
        std::ostringstream settled;
        settled << "player AI move: " << kSettleUpdates << " updates after the end the player is at " << position[0] << ' ' << position[1] << ' ' << position[2];
        endUpdate_ = 0;
        return settled.str();
    }
    moving_ = moving;
    std::ostringstream line;
    if (moving) {
        startUpdate_ = update;
        line << "player AI move: started at update " << update << " from " << position[0] << ' ' << position[1] << ' ' << position[2];
    } else {
        endUpdate_ = update;
        line << "player AI move: ended at update " << update << " at " << position[0] << ' ' << position[1] << ' ' << position[2] << ", " << update - startUpdate_ << " updates";
    }
    return line.str();
}

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
