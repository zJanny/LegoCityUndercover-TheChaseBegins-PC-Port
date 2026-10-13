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

// The landing stun lasts as long as the landing animation: (frames - 1) / rate, 13/15 s for "land" and 18/15 s for
// "fallland" (docs/re/player.md, derivation entry). Matches the measured 26/30 s and 36/30 s.
struct LandingDurations {
    float soft = 0;
    float hard = 0;
};
LandingDurations landingDurations(const assets::Animation& land, const assets::Animation& fallLand);

constexpr int kAnimationJump = 3;
constexpr int kAnimationJumpRun = 4;
constexpr int kAnimationLand = 5;
constexpr int kAnimationFall = 6;
constexpr int kAnimationFallLand = 107;

// Jump root motion (docs/re/player.md, derivation entry): the Y translation of the extra joint curve of the jump
// animation relative to frame 0, sampled at the animation rate over (frames - 1) intervals, linear between frames.
// The state-4 entry 0x200cc4 stores kJumpHeight in ctrl+0x88 and 0x200e70 scales the animation so that the extent of
// its root Y motion (max - min over the frames, minimum and maximum starting at 0) becomes kJumpHeight.
constexpr float kJumpHeight = 2.1f;               // *0x3742a0, stored by 0x3468c8 (literal 0x346908)
// 0x203d18, state 4 (and 68): the horizontal step is multiplied by the literal 0x3f4ccccd at 0x204028.
constexpr float kJumpHorizontalScale = 0.8f;

// Walk and run speeds of the stick while the jump animation plays.
RootMotionSpeeds jumpSpeeds(const RootMotionSpeeds& ground);

struct JumpProfile {
    std::vector<float> offsets;
    float framesPerSecond = 15.0f;
    float heightScale = 0;
    float duration() const;
    float offsetAt(float elapsed) const;
};
JumpProfile makeJumpProfile(const assets::Animation& jump, float height = kJumpHeight);

enum class Posture { Ground, Jumping, Falling, Landing };

struct VerticalState {
    Posture posture = Posture::Ground;
    float y = 0;
    float fallSpeed = 0;      // ctrl+0x2b8, world units per update, positive downwards
    float fallDistance = 0;   // ctrl+0x2b4, accumulated fall speed
    float landingTime = 0;
    float landingDuration = 0;
    LandingDurations landingDurations;
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
