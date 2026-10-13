#pragma once
// Vertical motion of the player: falling, landing and the landing stun, replicating 0x20b48c (ground call and
// fall branch), the event 0x26 handler (gravity step) and the landing decision of event 0x24 (0x2b24bc).
// Evidence and traces: docs/re/player.md, 2026-10-13 falling entry. No SDL dependency.
#include "runtime/collision.hpp"
#include "runtime/player_locomotion.hpp"
#include <vector>

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

// Jump root motion (docs/re/player.md jump entry): the vertical root track of the jump animation sampled at
// 15 frames per second over (frames - 1) intervals, linear between frames, scaled per animation.
constexpr float kJumpFramesPerSecond = 15.0f;
// PROVISIONAL: fitted to the oracle traces (both fits: sum of squared errors below 5e-7 over 22 updates);
// the peak of the scaled track is 2.093 in both animations, the source of the scale is not recovered.
constexpr float kJumpHeightScale = 0.860869f;
constexpr float kJumpRunHeightScale = 0.828197f;
// ctrl+0xa0 of the player (0.8): horizontal speed factor after take-off, measured as 0.8 for walk, run and sprint.
constexpr float kJumpHorizontalScale = 0.8f;

// Walk and run speeds of the stick while the jump animation plays.
RootMotionSpeeds jumpSpeeds(const RootMotionSpeeds& ground);

struct JumpProfile {
    std::vector<float> offsets;
    float duration() const;
    float offsetAt(float elapsed) const;
};
JumpProfile makeJumpProfile(const assets::AnimTrack& rootHeight, unsigned animationFrames, float heightScale);

enum class Posture { Ground, Jumping, Falling, Landing };

struct VerticalState {
    Posture posture = Posture::Ground;
    float y = 0;
    float fallSpeed = 0;      // ctrl+0x2b8, world units per update, positive downwards
    float fallDistance = 0;   // ctrl+0x2b4, accumulated fall speed
    float landingTime = 0;
    float landingDuration = 0;
    bool hardLanding = false;
    const JumpProfile* jump = nullptr;
    float jumpElapsed = 0;
    float jumpBase = 0;
};

struct VerticalInput {
    float x = 0, z = 0;                 // node position after this update's horizontal move
    float forwardX = 0, forwardZ = 1;
    float rootDelta = 0;                // length of this update's horizontal movement
    float dt = 0;
    bool moveRequested = false;         // ctrl+0xc bit 0
    const JumpProfile* startJump = nullptr;  // set in the update of a jump button press while grounded
};

struct VerticalStep {
    bool landed = false;
    bool hard = false;
    bool stunEnded = false;             // the update in which the stun ends is an idle update
    bool stunned = false;               // locomotion is suppressed in this update
    bool jumpStarted = false;
    bool jumpEnded = false;             // the jump animation ran out in the air
};

VerticalStep verticalUpdate(VerticalState& state, const CollisionWorld& world, const VerticalInput& input);

}
