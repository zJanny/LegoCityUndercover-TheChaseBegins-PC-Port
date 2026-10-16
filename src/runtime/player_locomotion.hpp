#pragma once
// Player locomotion step: regime from the stick intent, run timer, sprint role, facing turn and the
// ground-plane movement, replicating the rules measured in the original game (docs/re/player.md,
// 2026-10-13T13:42:20 and the entry that follows it). No SDL dependency.
#include "runtime/player_intent.hpp"
#include <cstdint>

namespace chase::runtime {

// Animation ids of the roles the locomotion switches between (tables/tablesanimations.bxls sheet 1,
// row = id; role table 0x219614 records 1 and 2 default column, record 12).
constexpr int kAnimationWalk = 1;
constexpr int kAnimationRun = 2;
constexpr int kAnimationSprint = 281;

// The mover 0x20cd30 gets the stick movement from 0x203d18, not from animation root motion (0x2327b4 yields zero for
// the walk, run and sprint animations): per update dt * 30 * (ctrl speed * ctrl+0x94) along the intent heading.
// docs/re/player.md, derivation entry.
constexpr float kRunSpeedPerStep = 0.23f;          // ATTR_RunSpeed default *0x37450c (stored by 0x343de4), ctrl+0x8c (0x236ec0)
constexpr float kWalkSpeedFraction = 0.33333334f;  // literal 0x3eaaaaab at 0x2371b0, ctrl+0x90 = ctrl+0x8c * this
constexpr float kStepsPerSecond = 30.0f;           // literal 0x41f00000 at 0x204014
// 0x203d18: while the player carries a big object (0x225754 = 2) the movement always uses the walk step (index forced to 1) multiplied by the float at 0x374368.
// Value 2.5 read from the heap in the oracle (state m11-8-donuts-in-hand, 0x374368 = 0x40200000, standing and walking; docs/re/carrying.md).
constexpr float kCarryWalkStepScale = 2.5f;
constexpr float kSprintSpeedScale = 1.5f;          // literal 0x3fc00000 at 0x1ac154, multiplied into ctrl+0x94 by 0x1abe60 (1.3 variant at 0x1ac150)

// Speed in world units per second at playback scale 1.0.
struct RootMotionSpeeds {
    float walk;
    float run;
    float sprintScale;  // ctrl+0x94 once the run timer passed kSprintTime; sprint plays at run speed times this
};

constexpr RootMotionSpeeds kCharacterSpeeds{kRunSpeedPerStep * kWalkSpeedFraction * kStepsPerSecond, kRunSpeedPerStep * kStepsPerSecond,
                                            kSprintSpeedScale};

// 0x1abe60: ctrl+0x24c above this value switches to the sprint role.
constexpr float kSprintTime = 1.0f;
// Facing step: per update, 0.3 of the remaining angle (rounded up), at most this many units (35.2 degrees).
constexpr std::int16_t kMaxFacingStep = 6400;
constexpr float kFacingStepFraction = 0.3f;

enum class Regime { Idle, Walk, Run };

// 0x1abe60 at 0x1abec4..0x1abf44: while the run animation plays on the ground, ctrl+0x94 = 1 - 0.55 * min(1, -2 d) *
// (1 - u) with d = forward . ground normal (only when d < 0, walking uphill) and u = max(0, (n.y - 0.6088) / (1 - 0.6088)).
constexpr float kSlopeSlowDown = 0.55f;   // literal 0x3f0ccccd at 0x1ac148
float slopeFactor(std::int16_t facing, const float normal[3]);

// 0x1abe60 runs the slope term, the run timer and the sprint only while the run role of the entity (0x219614(entity, 2))
// is "run" 2, 0x6f or 0x83; the carried run roles (151 itemleft_run, 552 supercarry_walk) skip them.
bool runRoleSprints(int runRole);

struct LocomotionState {
    float x = 0, z = 0;                 // ground-plane position (node+0x8c, +0x94)
    std::int16_t facing = 0;            // ctrl+8, what the node forward vector shows
    std::int16_t lastIntent = 0;        // ctrl+0xa, last intent heading of an active stick, kept after release
    float runTimer = 0;                 // ctrl+0x24c
    bool wasRunning = false;            // previous update was in the run regime
    bool carryingBig = false;           // the player holds a big carry object (0x225754 = 2), set before the update
    int runRole = kAnimationRun;        // 0x219614(entity, 2), set before the update
    bool sprinting = false;             // sprint role active (ctrl+0x94 == sprintScale)
    float slope = 1.0f;                 // slope factor of ctrl+0x94 computed at the end of the previous run update
};

struct LocomotionStep {
    Regime regime = Regime::Idle;
    int animation = 0;                  // animation to play from now on (kAnimationWalk / Run / Sprint, 0 when idle); this update moved with the previous role
    float animationScale = 1.0f;        // playback speed scale ctrl+0x94 in effect after this update
    float deltaX = 0, deltaZ = 0;       // movement of this update
};

// 16-bit heading: 32768 = pi. Forward on the ground plane is (sin h, cos h) for (x, z).
float headingRadians(std::int16_t heading);
std::int16_t wrapHeading(std::int32_t value);
// One facing update: sign(d) * min(kMaxFacingStep, ceil(f32(0.3) * |d|)) with d = wrap(target - facing).
std::int16_t turnFacing(std::int16_t facing, std::int16_t target);

// State 8 (docs/re/player.md, slide entry). 0x1f3f1c: the facing target (ctrl+0xa) is the heading of the horizontal part
// of the slide vector ext+0x5c, normalised, atan2(x, z) * 65536 / 2 pi truncated.
std::int16_t slideHeading(const float slide[3]);
// 0x203d18 for state 8: a held stick moves the character at its walk or run step, perpendicular to the target heading, to
// the side the stick is on, scaled by the angle between stick and target (folded to 0..16384, divided by 16384). The
// facing takes one turnFacing step toward the target first (0x2c9770 runs 0x1f3f1c before 0x203d18).
LocomotionStep slideLocomotionUpdate(LocomotionState& state, float dt, const StickIntent& intent, std::int16_t target,
                                     const RootMotionSpeeds& speeds = kCharacterSpeeds);
// 0x215804, the state 5 update within 0.5 s of a slide: the stick is cleared and the facing target is the facing.
void lockedFallLocomotion(LocomotionState& state);

// One player update with the intent of the stick-to-intent step (stickIntent).
LocomotionStep locomotionUpdate(LocomotionState& state, float dt, const StickIntent& intent,
                                const RootMotionSpeeds& speeds = kCharacterSpeeds);

}
