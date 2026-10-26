#include "runtime/lever_use.hpp"

#include "runtime/carry_animation.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>

namespace chase::runtime {
namespace {

constexpr float kHeadingUnitsPerRadian = 10430.378f;
constexpr float kEventRise = 0.5f;

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

std::int16_t headingOf(float x, float z) {
    const float value = std::atan2(x, z) * kHeadingUnitsPerRadian;
    return std::int16_t(std::uint16_t(std::int32_t(value)));
}

}

int leverSwitchClip(int leverType, int characterSwitchAnim) {
    switch (leverType) {
    case 0: return characterSwitchAnim;
    case 6: return kLeverClipKey;
    case 7: return kLeverClipPull;
    default: return 0;
    }
}

LeverClipTiming leverClipTiming(const assets::Animation& clip) {
    LeverClipTiming timing;
    timing.seconds = float(assets::animationLength(clip)) / assets::animationFramesPerSecond(clip);
    const float duration = assets::animationDuration(clip);
    for (const auto& sub : clip.subAnimations)
        for (const assets::AnimTrack& track : sub) {
            if (track.hash != kCarryEventTrackHash) continue;
            float previous = 0.0f;
            for (const assets::CurveKey& key : track.keys) {
                if (key.value - previous > kEventRise) {
                    timing.eventSeconds = key.time * duration;
                    return timing;
                }
                previous = key.value;
            }
        }
    return timing;
}

std::optional<UseLerpTarget> leverUseTarget(const assets::Model& model, const std::array<float, 16>& objectMatrix) {
    for (std::size_t node = 0; node < model.nodeNames.size() && node < model.nodeMatrices.size(); ++node) {
        if (lower(model.nodeNames[node]) != kLeverUseNode) continue;
        const std::array<float, 16>& m = model.nodeMatrices[node];
        std::array<float, 16> world{};
        for (std::size_t row = 0; row < 4; ++row)
            for (std::size_t column = 0; column < 4; ++column) {
                float sum = 0.0f;
                for (std::size_t k = 0; k < 4; ++k) sum += m[row * 4 + k] * objectMatrix[k * 4 + column];
                world[row * 4 + column] = sum;
            }
        UseLerpTarget target;
        target.position = {world[12], world[13], world[14]};
        target.heading = headingOf(world[8], world[10]);
        return target;
    }
    return std::nullopt;
}

std::int16_t turnTowards(std::int16_t facing, std::int16_t target, int step) {
    if (facing == target) return facing;
    int d = int(target) - int(facing);
    if (d < -32768) d += 65536;
    if (d > 32768) d -= 65536;
    d = std::clamp(d, -step, step);
    return std::int16_t(std::uint16_t(int(facing) + d));
}

void PlayerLeverUse::start(std::size_t lever, const float position[3], std::int16_t facing, const LeverUseSetup& setup, float dt) {
    setup_ = setup;
    lever_ = lever;
    state_ = kPlayerStateUseLerp;
    pending_ = 0;
    lerp_ = 0.0f;
    clipSeconds_ = 0.0f;
    eventFired_ = false;
    start_ = {position[0], position[1], position[2]};
    position_ = start_;
    facing_ = facing;
    if (!setup_.target) setup_.target = UseLerpTarget{start_, facing};
    stepLerp(dt);
}

void PlayerLeverUse::stepLerp(float dt) {
    const UseLerpTarget& target = *setup_.target;
    facing_ = turnTowards(facing_, target.heading, kUseLerpTurnStep);
    if (lerp_ == 1.0f) {
        facing_ = target.heading;
        pending_ = kPlayerStatePullLeverUse;
        return;
    }
    lerp_ = std::min(1.0f, lerp_ + dt / kUseLerpSeconds);
    for (std::size_t a = 0; a < 3; ++a) position_[a] = start_[a] + (target.position[a] - start_[a]) * lerp_;
}

LeverUseStep PlayerLeverUse::update(float dt, bool actionHeld, bool leverUsed) {
    LeverUseStep step;
    if (pending_ != 0) {
        state_ = pending_;
        pending_ = 0;
        step.entered = state_;
        if (state_ != kPlayerStatePullLeverUse) {
            state_ = 0;
            step.ended = true;
            return step;
        }
        turnsLeft_ = setup_.numberOfTurns - 1.0f;
        clipSeconds_ = 0.0f;
        eventFired_ = false;
        if (turnsLeft_ > 0.0f) step.leverAnim = LeverAnim::Forward;
    }
    if (state_ == kPlayerStateUseLerp) {
        stepLerp(dt);
        return step;
    }
    if (state_ != kPlayerStatePullLeverUse) return step;
    clipSeconds_ += dt;
    if (!eventFired_ && setup_.clip.eventSeconds >= 0.0f && clipSeconds_ >= setup_.clip.eventSeconds) {
        eventFired_ = true;
        if (!(turnsLeft_ > 0.0f) && !leverUsed) step.useBit = true;
    }
    bool finished = clipSeconds_ >= setup_.clip.seconds;
    if (!(turnsLeft_ < 0.0f)) {
        if (finished) turnsLeft_ -= 1.0f;
        if (actionHeld) {
            if (finished && !(turnsLeft_ < 0.0f)) {
                clipSeconds_ = 0.0f;
                eventFired_ = false;
                finished = false;
                step.leverAnim = LeverAnim::Forward;
            }
        } else if (finished && !(turnsLeft_ < 0.0f)) {
            step.leverAnim = LeverAnim::Backward;
            pending_ = kPlayerStateAfterLeverUse;
            return step;
        }
    }
    if (!finished) return step;
    pending_ = setup_.holdMe ? kPlayerStatePullLeverHold : kPlayerStateAfterLeverUse;
    if (!setup_.holdMe && setup_.numberOfTurns > 1.0f) step.useBit = true;
    return step;
}

}
