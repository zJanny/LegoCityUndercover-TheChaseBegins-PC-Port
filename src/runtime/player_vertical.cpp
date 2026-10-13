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

void land(VerticalState& state, const VerticalInput& input, VerticalStep& step) {
    step.landed = true;
    step.hard = state.fallDistance >= kHardLandingDistance;
    const bool stun = step.hard || (!input.moveRequested && state.fallDistance > kGravity);
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
    if (state.posture == Posture::Ground && input.rootDelta <= 0) return step;
    const std::optional<float> ground = world.empty() ? std::nullopt : groundHeight(world, probeFor(state, input));
    if (ground) {
        state.y = *ground;
        if (state.posture == Posture::Falling) land(state, input, step);
        return step;
    }
    if (!world.empty()) gravityStep(state, input.dt);
    return step;
}

}
