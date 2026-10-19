#include "runtime/player_motion.hpp"
#include <cmath>

namespace chase::runtime {

PlayerMotionStep playerRootStepUpdate(LocomotionState& motion, VerticalState& vertical, const CollisionWorld& world, float deltaX, float deltaZ, float dt, float deltaY) {
    PlayerMotionStep result;
    VerticalInput input;
    input.dt = dt;
    input.ticks = stepTicks(dt);
    const float heading = headingRadians(motion.facing);
    input.forwardX = std::sin(heading);
    input.forwardZ = std::cos(heading);
    float dx = deltaX, dz = deltaZ;
    const float rootDelta = std::hypot(dx, dz);
    if (rootDelta > 0 && !world.empty())
        slideAlongWalls(world, motion.x, vertical.y, motion.z, input.forwardX, input.forwardZ, dx, dz, PlayerBox(), kPlayerExcludedSurfaces, 0.0f);
    motion.x += dx;
    motion.z += dz;
    vertical.y += deltaY;
    input.x = motion.x;
    input.z = motion.z;
    input.rootDelta = rootDelta;
    result.locomotion.deltaX = dx;
    result.locomotion.deltaZ = dz;
    result.vertical = verticalUpdate(vertical, world, input);
    motion.x += result.vertical.moveX;
    motion.z += result.vertical.moveZ;
    return result;
}

PlayerMotionStep playerMotionUpdate(LocomotionState& motion, VerticalState& vertical, const JumpProfiles& jumps,
                                    const CollisionWorld& world, const StickIntent& intent, bool jumpPressed, float dt) {
    PlayerMotionStep result;
    VerticalInput input;
    input.dt = dt;
    input.ticks = stepTicks(dt);
    input.moveRequested = intent.active;
    if (vertical.posture == Posture::Landing) {
        result.vertical = verticalUpdate(vertical, world, input);
        result.stunned = true;
        return result;
    }
    const bool jumping = vertical.posture == Posture::Jumping;
    const bool sliding = vertical.posture == Posture::Sliding;
    const bool locked = vertical.steeringLocked();
    const RootMotionSpeeds speeds = jumping ? jumpSpeeds(kCharacterSpeeds) : kCharacterSpeeds;
    const LocomotionState before = motion;
    LocomotionStep& step = result.locomotion;
    if (sliding) {
        step = slideLocomotionUpdate(motion, dt, intent, slideHeading(vertical.slide), speeds);
    } else if (locked) {
        lockedFallLocomotion(motion);
        step = locomotionUpdate(motion, dt, StickIntent(), speeds);
    } else {
        step = locomotionUpdate(motion, dt, intent, speeds);
    }
    input.moveRequested = intent.active && !locked;
    if (jumping) {
        motion.runTimer = before.runTimer;
        motion.wasRunning = before.wasRunning;
        motion.sprinting = before.sprinting;
        step.animationScale = before.sprinting ? kCharacterSpeeds.sprintScale : 1.0f;
    }
    float dx = step.deltaX, dz = step.deltaZ;
    const float rootDelta = std::hypot(dx, dz);
    const float heading = headingRadians(motion.facing);
    input.forwardX = std::sin(heading);
    input.forwardZ = std::cos(heading);
    const bool dropping = vertical.posture == Posture::Falling || vertical.posture == Posture::Sliding;
    if (rootDelta > 0 && !world.empty())
        slideAlongWalls(world, before.x, vertical.y, before.z, input.forwardX, input.forwardZ, dx, dz, PlayerBox(), kPlayerExcludedSurfaces,
                        dropping ? -vertical.fallSpeed : 0.0f);
    motion.x = before.x + dx;
    motion.z = before.z + dz;
    input.x = motion.x;
    input.z = motion.z;
    input.rootDelta = rootDelta;
    if (jumpPressed && vertical.posture == Posture::Ground) {
        result.jumpMoving = intent.active;
        input.startJump = intent.active ? jumps.moving : jumps.standing;
    }
    result.jumping = jumping;
    result.locked = locked;
    result.fallBefore = jumping ? 0.0f : vertical.fallDistance;
    result.vertical = verticalUpdate(vertical, world, input);
    motion.x += result.vertical.moveX;
    motion.z += result.vertical.moveZ;
    if (result.vertical.landed && vertical.posture == Posture::Ground && intent.active && step.regime == Regime::Run) {
        motion.runTimer = (motion.sprinting ? kSprintTime : 0.0f) + dt;
        motion.wasRunning = true;
        motion.sprinting = motion.runTimer > kSprintTime;
    }
    motion.slope = 1.0f;
    if (vertical.posture == Posture::Ground && step.regime == Regime::Run) {
        motion.slope = slopeFactor(motion.facing, vertical.groundNormal);
        step.animationScale *= motion.slope;
    }
    if (vertical.posture == Posture::Falling || vertical.posture == Posture::Sliding) {
        motion.runTimer = 0;
        motion.wasRunning = false;
        motion.sprinting = false;
    }
    return result;
}

}
