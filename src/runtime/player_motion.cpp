#include "runtime/player_motion.hpp"
#include <array>
#include <cmath>

namespace chase::runtime {
namespace {
std::array<float, 3> takeGroundEntityMove(VerticalState& vertical) {
    const std::array<float, 3> move{vertical.groundEntityMove[0], vertical.groundEntityMove[1], vertical.groundEntityMove[2]};
    vertical.groundEntityMove[0] = vertical.groundEntityMove[1] = vertical.groundEntityMove[2] = 0;
    return move;
}

bool riding(const std::array<float, 3>& move) { return move[0] != 0 || move[1] != 0 || move[2] != 0; }

float movementLength(float dx, float dy, float dz, bool withRide) { return withRide ? std::sqrt(dx * dx + dy * dy + dz * dz) : std::hypot(dx, dz); }
}

PlayerMotionStep playerRootStepUpdate(LocomotionState& motion, VerticalState& vertical, const CollisionWorld& world, float deltaX, float deltaZ, float dt, float deltaY) {
    PlayerMotionStep result;
    VerticalInput input;
    input.dt = dt;
    input.ticks = stepTicks(dt);
    const float heading = headingRadians(motion.facing);
    input.forwardX = std::sin(heading);
    input.forwardZ = std::cos(heading);
    const std::array<float, 3> ride = takeGroundEntityMove(vertical);
    float dx = deltaX + ride[0], dz = deltaZ + ride[2];
    const float rootDelta = movementLength(dx, ride[1], dz, riding(ride));
    if (std::hypot(dx, dz) > 0 && !world.empty())
        slideAlongWalls(world, motion.x, vertical.y, motion.z, input.forwardX, input.forwardZ, dx, dz, PlayerBox(), kPlayerExcludedSurfaces, ride[1]);
    motion.x += dx;
    motion.z += dz;
    vertical.y += deltaY;
    input.x = motion.x;
    input.z = motion.z;
    input.rootDelta = rootDelta;
    input.rise = ride[1] > 0 ? ride[1] : 0.0f;
    input.horizontalMove = dx != 0 || dz != 0;
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
    const std::array<float, 3> ride = vertical.posture == Posture::Jumping ? std::array<float, 3>{} : takeGroundEntityMove(vertical);
    if (vertical.posture == Posture::Landing) {
        // STAND-IN SCRIPT-S310: the landing stun takes the ground entity's move without the wall slide and the probe.
        motion.x += ride[0];
        vertical.y += ride[1];
        motion.z += ride[2];
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
    float dx = step.deltaX + ride[0], dz = step.deltaZ + ride[2];
    const float rootDelta = movementLength(dx, ride[1], dz, riding(ride));
    const float heading = headingRadians(motion.facing);
    input.forwardX = std::sin(heading);
    input.forwardZ = std::cos(heading);
    const bool dropping = vertical.posture == Posture::Falling || vertical.posture == Posture::Sliding;
    if (std::hypot(dx, dz) > 0 && !world.empty())
        slideAlongWalls(world, before.x, vertical.y, before.z, input.forwardX, input.forwardZ, dx, dz, PlayerBox(), kPlayerExcludedSurfaces,
                        (dropping ? -vertical.fallSpeed : 0.0f) + ride[1]);
    motion.x = before.x + dx;
    motion.z = before.z + dz;
    input.x = motion.x;
    input.z = motion.z;
    input.rootDelta = rootDelta;
    input.rise = ride[1] > 0 ? ride[1] : 0.0f;
    input.horizontalMove = dx != 0 || dz != 0;
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
