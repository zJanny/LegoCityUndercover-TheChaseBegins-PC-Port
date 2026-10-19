#pragma once
// One player update of the position and posture: locomotion, wall slide (0x21a73c), then the vertical step (jump,
// ground probe, fall, landing, the state-8 slide). A landing stun suppresses locomotion; in the air the stick keeps its
// walk or run speed (times 0.8 after a jump); during the first 0.5 s of the fall that follows a slide the stick is
// ignored. No SDL dependency.
#include "runtime/player_vertical.hpp"

namespace chase::runtime {

struct JumpProfiles {
    const JumpProfile* standing = nullptr;
    const JumpProfile* moving = nullptr;
};

struct PlayerMotionStep {
    LocomotionStep locomotion;
    VerticalStep vertical;
    bool stunned = false;              // the update was a landing stun update
    bool locked = false;               // the stick was ignored (fall after a slide)
    bool jumping = false;              // the update started in the jump posture
    float fallBefore = 0;              // ctrl+0x2b4 before the update (zero for a jump)
    bool jumpMoving = false;           // a jump started in this update with the stick held
};

// One update with a given ground-plane step instead of the stick (the root step of the attack states, 0x1fa7e0 -> 0x20cd30 with the stick bit
// cleared): wall slide (0x21a73c) along the facing, then the vertical step without a move request.
PlayerMotionStep playerRootStepUpdate(LocomotionState& motion, VerticalState& vertical, const CollisionWorld& world, float deltaX, float deltaZ, float dt, float deltaY = 0.0f);

PlayerMotionStep playerMotionUpdate(LocomotionState& motion, VerticalState& vertical, const JumpProfiles& jumps,
                                    const CollisionWorld& world, const StickIntent& intent, bool jumpPressed, float dt);

}
