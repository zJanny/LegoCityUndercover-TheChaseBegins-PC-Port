#include "runtime/player_locomotion.hpp"
#include <cmath>

namespace chase::runtime {

float headingRadians(std::int16_t heading) {
    return static_cast<float>(heading) * (2.0f * 3.14159265358979f / 65536.0f);
}

std::int16_t wrapHeading(std::int32_t value) {
    return static_cast<std::int16_t>(static_cast<std::uint16_t>(value));
}

std::int16_t turnFacing(std::int16_t facing, std::int16_t target) {
    const std::int32_t difference = wrapHeading(std::int32_t(target) - std::int32_t(facing));
    if (difference == 0) return facing;
    const float magnitude = kFacingStepFraction * static_cast<float>(std::abs(difference));
    std::int32_t step = static_cast<std::int32_t>(std::ceil(magnitude));
    if (step > kMaxFacingStep) step = kMaxFacingStep;
    return wrapHeading(std::int32_t(facing) + (difference > 0 ? step : -step));
}

LocomotionStep locomotionUpdate(LocomotionState& state, float dt, const StickIntent& intent,
                                const RootMotionSpeeds& speeds) {
    LocomotionStep step;
    if (intent.active) state.lastIntent = intent.heading;
    state.facing = turnFacing(state.facing, state.lastIntent);

    float speed = 0;
    if (!intent.active) {
        step.regime = Regime::Idle;
        state.wasRunning = false;
        state.sprinting = false;
    } else if (intent.heldSet & kHeldSlow) {
        step.regime = Regime::Walk;
        speed = speeds.walk;
        state.wasRunning = false;
        state.sprinting = false;
    } else {
        step.regime = Regime::Run;
        speed = state.sprinting ? speeds.run * speeds.sprintScale : speeds.run;
        if (!state.wasRunning) state.runTimer = 0;
        state.wasRunning = true;
        state.runTimer += dt;
        state.sprinting = state.runTimer > kSprintTime;
    }
    if (step.regime == Regime::Walk) step.animation = kAnimationWalk;
    if (step.regime == Regime::Run) step.animation = state.sprinting ? kAnimationSprint : kAnimationRun;
    step.animationScale = state.sprinting ? speeds.sprintScale : 1.0f;

    if (speed != 0) {
        const float heading = headingRadians(state.lastIntent);
        step.deltaX = speed * dt * std::sin(heading);
        step.deltaZ = speed * dt * std::cos(heading);
        state.x += step.deltaX;
        state.z += step.deltaZ;
    }
    return step;
}

}
