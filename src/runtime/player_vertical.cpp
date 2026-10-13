#include "runtime/player_vertical.hpp"
#include <algorithm>
#include <cmath>

namespace chase::runtime {

namespace {
void gravityStep(VerticalState& state, float dt) {
    state.fallSpeed = std::min(kTerminalFallSpeed, state.fallSpeed + kGravity * kGravityRate * dt);
    state.y -= state.fallSpeed;
    state.fallDistance += state.fallSpeed;
    state.posture = Posture::Falling;
}

GroundProbe probeFor(const VerticalState& state, const VerticalInput& input) {
    GroundProbe probe;
    probe.x = input.x;
    probe.y = state.y;
    probe.z = input.z;
    probe.forwardX = input.forwardX;
    probe.forwardZ = input.forwardZ;
    probe.rootDelta = input.rootDelta;
    probe.dt = input.dt;
    if (state.posture == Posture::Falling) probe.distance = std::max(std::fabs(state.fallSpeed), kGroundMinimumDistance);
    return probe;
}

void land(VerticalState& state, const VerticalInput& input, VerticalStep& step, bool fromJump) {
    step.landed = true;
    step.hard = state.fallDistance >= kHardLandingDistance;
    const bool stun = step.hard || (!input.moveRequested && (fromJump || state.fallDistance > kGravity));
    state.fallSpeed = 0;
    state.fallDistance = 0;
    if (stun) {
        state.posture = Posture::Landing;
        state.landingTime = 0;
        state.hardLanding = step.hard;
        state.landingDuration = step.hard ? kHardLandingSeconds : kSoftLandingSeconds;
    } else {
        state.posture = Posture::Ground;
    }
}
}

RootMotionSpeeds jumpSpeeds(const RootMotionSpeeds& ground) {
    return {ground.walk * kJumpHorizontalScale, ground.run * kJumpHorizontalScale, ground.sprintScale};
}

float JumpProfile::duration() const {
    return offsets.size() < 2 ? 0.0f : float(offsets.size() - 1) / kJumpFramesPerSecond;
}

float JumpProfile::offsetAt(float elapsed) const {
    if (offsets.empty()) return 0.0f;
    const float last = float(offsets.size() - 1);
    float position = elapsed * kJumpFramesPerSecond;
    if (position >= last) return offsets.back();
    if (position < 0) position = 0;
    const std::size_t frame = std::size_t(position);
    const float fraction = position - float(frame);
    return offsets[frame] + (offsets[frame + 1] - offsets[frame]) * fraction;
}

JumpProfile makeJumpProfile(const assets::AnimTrack& rootHeight, unsigned animationFrames, float heightScale) {
    JumpProfile profile;
    const unsigned intervals = animationFrames > 1 ? animationFrames - 1 : 1;
    for (unsigned frame = 0; frame <= intervals; ++frame)
        profile.offsets.push_back(heightScale * assets::evaluateTrack(rootHeight, float(frame) / float(intervals)));
    return profile;
}

VerticalStep verticalUpdate(VerticalState& state, const CollisionWorld& world, const VerticalInput& input) {
    VerticalStep step;
    if (state.posture == Posture::Landing) {
        state.landingTime += input.dt;
        if (state.landingTime >= state.landingDuration) {
            state.posture = Posture::Ground;
            step.stunEnded = true;
        }
        step.stunned = true;
        return step;
    }
    if (state.posture == Posture::Jumping) {
        state.jumpElapsed += input.dt;
        const float previousY = state.y;
        state.y = state.jumpBase + state.jump->offsetAt(state.jumpElapsed);
        const float climb = state.y - previousY;
        std::optional<float> ground;
        if (climb <= 0 && !world.empty()) {
            VerticalInput probeInput = input;
            probeInput.rootDelta = std::sqrt(input.rootDelta * input.rootDelta + climb * climb);
            GroundProbe probe = probeFor(state, probeInput);
            probe.distance.reset();
            ground = groundHeight(world, probe);
        }
        if (ground) {
            state.y = *ground;
            state.fallDistance = 0;
            land(state, input, step, true);
            state.jump = nullptr;
        } else if (state.jumpElapsed >= state.jump->duration()) {
            state.posture = Posture::Falling;
            state.fallSpeed = 0;
            state.fallDistance = 0;
            state.jump = nullptr;
            step.jumpEnded = true;
        }
        return step;
    }
    if (state.posture == Posture::Ground && input.startJump) {
        state.posture = Posture::Jumping;
        state.jump = input.startJump;
        state.jumpElapsed = 0;
        state.jumpBase = state.y;
        step.jumpStarted = true;
        return step;
    }
    if (state.posture == Posture::Ground && input.rootDelta <= 0) return step;
    const std::optional<float> ground = world.empty() ? std::nullopt : groundHeight(world, probeFor(state, input));
    if (ground) {
        state.y = *ground;
        if (state.posture == Posture::Falling) land(state, input, step, false);
        return step;
    }
    if (!world.empty()) gravityStep(state, input.dt);
    return step;
}

}
