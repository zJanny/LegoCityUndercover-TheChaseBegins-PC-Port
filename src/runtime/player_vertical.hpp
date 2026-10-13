#pragma once
// Vertical motion of the player: falling, landing and the landing stun, replicating 0x20b48c (ground call and
// fall branch), the event 0x26 handler (gravity step) and the landing decision of event 0x24 (0x2b24bc).
// Evidence and traces: docs/re/player.md, 2026-10-13 falling entry. No SDL dependency.
#include "runtime/collision.hpp"

namespace chase::runtime {

constexpr float kGravity = 0.05f;                 // *0x3771ac, per update at 30 updates per second
constexpr float kGravityRate = 30.0f;             // literal at 0x18f5a0
constexpr float kTerminalFallSpeed = 0.8f;        // *0x3771b0 through 0x2b75d0, per update
constexpr float kHardLandingDistance = 12.0f;     // *0x374360, compared with ctrl+0x2b4
// PROVISIONAL: measured landing stun lengths (the end of the landing animation state), docs/re/player.md.
constexpr float kSoftLandingSeconds = 26.0f / 30.0f;
constexpr float kHardLandingSeconds = 36.0f / 30.0f;

constexpr int kAnimationJump = 3;
constexpr int kAnimationJumpRun = 4;
constexpr int kAnimationLand = 5;
constexpr int kAnimationFall = 6;
constexpr int kAnimationFallLand = 107;

enum class Posture { Ground, Falling, Landing };

struct VerticalState {
    Posture posture = Posture::Ground;
    float y = 0;
    float fallSpeed = 0;      // ctrl+0x2b8, world units per update, positive downwards
    float fallDistance = 0;   // ctrl+0x2b4, accumulated fall speed
    float landingTime = 0;
    float landingDuration = 0;
    bool hardLanding = false;
};

struct VerticalInput {
    float x = 0, z = 0;                 // node position after this update's horizontal move
    float forwardX = 0, forwardZ = 1;
    float rootDelta = 0;                // length of this update's horizontal movement
    float dt = 0;
    bool moveRequested = false;         // ctrl+0xc bit 0
};

struct VerticalStep {
    bool landed = false;
    bool hard = false;
    bool stunEnded = false;             // the update in which the stun ends is an idle update
    bool stunned = false;               // locomotion is suppressed in this update
};

VerticalStep verticalUpdate(VerticalState& state, const CollisionWorld& world, const VerticalInput& input);

}
