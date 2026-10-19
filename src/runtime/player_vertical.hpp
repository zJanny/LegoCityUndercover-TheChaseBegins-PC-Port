#pragma once
// Vertical motion of the player: falling, landing and the landing stun, replicating 0x20b48c (ground call and
// fall branch), the event 0x26 handler (gravity step) and the landing decision of event 0x24 (0x2b24bc).
// Evidence and traces: docs/re/player.md, 2026-10-13 falling entry. No SDL dependency.
#include "runtime/collision.hpp"
#include "runtime/placement_message.hpp"
#include "runtime/player_locomotion.hpp"
#include "runtime/scheduler.hpp"
#include <vector>

namespace chase::runtime {

constexpr float kGravity = 0.05f;                 // *0x3771ac, per update at 30 updates per second
constexpr float kGravityRate = 30.0f;             // literal at 0x18f5a0
constexpr float kTerminalFallSpeed = 0.8f;        // *0x3771b0 through 0x2b75d0, per update
constexpr float kHardLandingDistance = 12.0f;     // *0x374360, compared with ctrl+0x2b4
// States 25 and 26 (bounce take-off and rise): every update the node rises by the speed and the speed grows by this step, not scaled by dt (oracle, state
// m12-trampoline-built, dt 0.033 and 0.07, docs/re/props-wobbly-bouncer.md); the first update with a positive speed is an ordinary falling update and the next one does not move the node.
constexpr float kBounceRiseStep = 0.05f;

// States 6 (soft landing) and 7 (hard landing) end when the animation they started has played to its last frame: the
// update functions 0x1d55cc and 0x2a47b0 ask 0x22f560, which asks 0x24cbd4, which tests
// elapsed clock ticks * rate / 3000 >= frames - 1 (docs/re/player.md, landing entry).
struct StunAnimation {
    float lastFrame = 0;
    float framesPerSecond = 15.0f;
};
// STAND-IN: the soft landing at the end of the fall that follows a slide (within 0.5 s) lasted one update in all four
// recordings (dt 0.0163, 0.033, 0.066) whatever the animation length; the rule that ends it early in the original is not
// recovered, so it is modelled as an animation that is finished at once.
constexpr StunAnimation kSlideLandingStunStandIn{0.0f, 15.0f};
StunAnimation stunAnimation(const assets::Animation& animation);
bool stunFinished(const StunAnimation& animation, std::uint32_t elapsedTicks);

struct LandingAnimations {
    StunAnimation soft;
    StunAnimation hard;
};
LandingAnimations landingAnimations(const assets::Animation& land, const assets::Animation& fallLand);

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

// Controller states: Ground = 1/2/3, Jumping = 4, Falling = 5, Landing = 6/7, Sliding = 8 (steep slope).
enum class Posture { Ground, Jumping, Falling, Landing, Sliding };

// State 8 (docs/re/player.md, slide entry). The branch of the ground-contact handler 0x2b24bc for a floor normal with
// 0 < y < *0x376910 (kFloorNormalMin) starts it with ctrl+0x2b8 = *0x3e99999a (0.5 on surface 6).
constexpr float kSlideSpeed = 0.3f;
constexpr float kSlideSpeedSlipperySurface = 0.5f;
constexpr std::uint8_t kSlipperySurface = 6;
// Event 0x26 in state 8 starts state 5; state 5 entered from state 8 sets ext+8 = clock + 0.5 (0x1ab98c, literal 0x1abaa4)
// and its update (0x1d4ce0) runs 0x215804 (no stick, facing target = facing) until the clock passes that time.
constexpr std::uint32_t kSlideFallLockTicks = kClockFrequency / 2;

struct VerticalState {
    Posture posture = Posture::Ground;
    float y = 0;
    float fallSpeed = 0;      // ctrl+0x2b8, world units per update, positive downwards
    float fallDistance = 0;   // ctrl+0x2b4, accumulated fall speed
    std::uint32_t landingTicks = 0;     // clock ticks since the landing update
    StunAnimation landingAnimation;     // the animation of the current stun
    LandingAnimations landingAnimations;
    bool hardLanding = false;
    float groundNormal[3] = {0, 1, 0};  // ctrl+0x1ec, set by a ground probe that found a floor
    std::uint8_t groundSurface = 0;     // surface byte of the ground triangle
    const JumpProfile* jump = nullptr;
    float jumpElapsed = 0;
    float jumpBase = 0;
    float slide[3] = {0, 0, 0};         // ext+0x5c: the slide vector of the last state-8 update
    const assets::CollisionTriangle* groundRecord = nullptr;  // the triangle of the last ground probe that found a floor
    bool bouncing = false;              // states 25 and 26 (docs/re/props-wobbly-bouncer.md): posture Falling, risen by the bounce speed
    bool bounceSettling = false;        // the update after the last rise update (state 5 entered): the node does not move
    bool slideFall = false;             // state 5 entered from state 8
    std::uint32_t slideFallTicks = 0;   // clock ticks since that state 5 began
    PlacementEffects placement;         // what the last PositionPlayer / PositionObject left on the player (placement_message.hpp)
    bool steeringLocked() const { return posture == Posture::Falling && slideFall && slideFallTicks <= kSlideFallLockTicks; }
};

struct VerticalInput {
    float x = 0, z = 0;                 // node position after this update's horizontal move
    float forwardX = 0, forwardZ = 1;
    float rootDelta = 0;                // length of this update's horizontal movement
    float dt = 0;
    std::uint32_t ticks = 0;            // clock ticks of this update, dt in units of 1/3000 s
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
    bool slideStarted = false;
    float moveX = 0, moveZ = 0;         // horizontal movement the slide adds on top of the update's locomotion
};

VerticalStep verticalUpdate(VerticalState& state, const CollisionWorld& world, const VerticalInput& input);

}
