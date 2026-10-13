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

struct LocomotionState {
    float x = 0, z = 0;                 // ground-plane position (node+0x8c, +0x94)
    std::int16_t facing = 0;            // ctrl+8, what the node forward vector shows
    std::int16_t lastIntent = 0;        // ctrl+0xa, last intent heading of an active stick, kept after release
    float runTimer = 0;                 // ctrl+0x24c
    bool wasRunning = false;            // previous update was in the run regime
    bool sprinting = false;             // sprint role active (ctrl+0x94 == sprintScale)
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

// One player update with the intent of the stick-to-intent step (stickIntent).
LocomotionStep locomotionUpdate(LocomotionState& state, float dt, const StickIntent& intent,
                                const RootMotionSpeeds& speeds = kCharacterSpeeds);

}
