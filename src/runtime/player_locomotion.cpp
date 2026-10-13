#include "runtime/player_locomotion.hpp"
#include "runtime/collision.hpp"
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

float slopeFactor(std::int16_t facing, const float normal[3]) {
    const float heading = headingRadians(facing);
    const float forward[3] = {gameSin(heading), 0.0f, gameCos(heading)};
    float dot = forward[0] * normal[0];
    dot = dot + forward[1] * normal[1];
    dot = dot + forward[2] * normal[2];
    if (!(dot < 0.0f)) return 1.0f;
    const float limit = -(2.0f * dot);
    const float clamped = limit < 1.0f ? (-dot) * 2.0f : 1.0f;
    const float upright = (normal[1] - kFloorNormalMin) / (1.0f - kFloorNormalMin);
    const float u = upright >= 0.0f ? upright : 0.0f;
    const float weighted = clamped * kSlopeSlowDown;
    return 1.0f - weighted * (1.0f - u);
}

std::int16_t slideHeading(const float slide[3]) {
    float flat[3] = {slide[0], 0, slide[2]};
    normaliseVector(flat);
    const float angle = gameAtan2(flat[0], flat[2]);
    return static_cast<std::int16_t>(static_cast<std::int32_t>(angle * 10430.378f));
}

LocomotionStep slideLocomotionUpdate(LocomotionState& state, float dt, const StickIntent& intent, std::int16_t target,
                                     const RootMotionSpeeds& speeds) {
    LocomotionStep step;
    state.lastIntent = target;
    state.facing = turnFacing(state.facing, target);
    state.wasRunning = false;
    if (!intent.active) return step;
    const bool slow = (intent.heldSet & kHeldSlow) != 0;
    step.regime = slow ? Regime::Walk : Regime::Run;
    const float speed = slow ? speeds.walk : speeds.run;
    std::int32_t difference = wrapHeading(std::int32_t(target) - std::int32_t(intent.heading));
    std::uint16_t direction = static_cast<std::uint16_t>(std::int32_t(target) + 0x4000);
    if (difference > 0) direction = static_cast<std::uint16_t>(direction + 0x8000);
    if (difference < 0) difference = -difference;
    if (difference > 0x4000) difference = 0x8000 - difference;
    const float scale = static_cast<float>(difference) * 0.00006103515625f;
    const float angle = static_cast<float>(direction) * (2.0f * 3.14159265358979f / 65536.0f);
    step.deltaX = speed * dt * (scale * gameSin(angle));
    step.deltaZ = speed * dt * (scale * gameCos(angle));
    state.x += step.deltaX;
    state.z += step.deltaZ;
    return step;
}

void lockedFallLocomotion(LocomotionState& state) {
    state.lastIntent = state.facing;
    state.wasRunning = false;
    state.sprinting = false;
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
        const float slope = state.wasRunning ? state.slope : 1.0f;
        speed = (state.sprinting ? speeds.run * speeds.sprintScale : speeds.run) * slope;
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
        step.deltaX = speed * dt * gameSin(heading);
        step.deltaZ = speed * dt * gameCos(heading);
        state.x += step.deltaX;
        state.z += step.deltaZ;
    }
    return step;
}

}
