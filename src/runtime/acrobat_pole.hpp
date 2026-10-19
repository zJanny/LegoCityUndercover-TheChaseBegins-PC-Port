#pragma once
// The acrobat pole states (flag poles, type 29 bars with AcrobatPole): 93 idle (enter 0x31c20c, update 0x31c2dc), 94 flip (enter 0x31f31c),
// 95..98 jump (enter 0x31f3bc, update 0x31f6e4) with the event listeners attached by 0x2c42f8 (Flip Done 0x334680, Jump Done 0x334794,
// Jump Anim 0x33475c, Jump X Done 0x32ec40). Rules and measurements: docs/re/player-traversal-rails.md. No SDL dependency.
#include "assets/assets.hpp"
#include "runtime/climb_bar.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace chase::runtime {

// Clip ids of the state objects (static initialiser 0x347e34, tablesanimations sheet 1): hpole_loop, hpole_180, hpole_dismount twice.
constexpr int kAnimationAcrobatPoleIdle = 0x2c;
constexpr int kAnimationAcrobatPoleFlip = 0x2d;
constexpr int kAnimationAcrobatPoleJump = 0x2e;
constexpr int kAnimationAcrobatPoleJumpOff = 0x2f;
constexpr int kAcrobatPoleFallState = 5;               // the argument of 0x32ec40 (Jump X Done) and the failed Jump Done
constexpr int kPointObjectType = 11;                   // 0x31f7d8: a target of this type is reached by the locomotion move after frame 10
constexpr float kAcrobatPoleTargetMoveFrame = 10.0f;   // *0x31f8f0
// The track whose value changes are the Jump Anim events (0x1d5ec0 literal 0x1d6868) and whose keys give the vertical speed (0x31f6dc).
constexpr std::uint32_t kAcrobatPoleJumpTrackHash = 0xc3fcea91;
// TRAV-S24: ctrl+0x2e8 after the jump enter, the hand joint of hpole_dismount at frame 0 (0x1fb8d8 samples it through 0x27086c), measured
// pole-local in the r24-C traces (t1 row 16: 41.41252 under 1-2_FlagPole_01 at 43.46950, z 637.53436 against 637.55212 with yaw 180).
constexpr float kAcrobatPoleJumpHangOffsetY = -2.05698f;
constexpr float kAcrobatPoleJumpHangOffsetZ = 0.01783f;

struct AcrobatPoleClip {
    float length = 0;        // animationLength (slot end +0x2e)
    float rate = 15.0f;      // frames per second (slot +0x30, 0x265174)
};

// What the states read from the three clips.
struct AcrobatPoleClips {
    AcrobatPoleClip idle, flip, jump;
    std::vector<std::array<float, 3>> jumpRoot;  // root motion of hpole_dismount per frame
    float jumpTravel = 0;                        // 0x2147e4: the root extent z of hpole_dismount
    std::vector<float> jumpEventFrames;          // clip times of the value changes of the track 0xc3fcea91 (key time x length)
    float jumpPhaseFrames = 0;                   // 0x31f3bc: from the first key above 0 to the next key below 1, times the slot end
};

AcrobatPoleClips acrobatPoleClips(const assets::Animation& idle, const assets::Animation& flip, const assets::Animation& jump);

struct AcrobatPoleTarget {
    bool present = false;
    int bar = -1;                 // index in ClimbBarSet::bars when the target is a bar
    int objectType = -1;
    float point[3] = {0, 0, 0};   // the bar's GrabBound centre in world, else the object position (0x23fb78)
};

struct AcrobatPoleRider {
    int state = 0;                // ctrl+0x6c while an acrobat pole state runs, 0 otherwise
    int pending = 0;              // ctrl+0x6e: the request entered in the next update
    int pole = -1;                // ctrl+0x130
    int from = -1;                // ctrl+0x134
    AcrobatPoleTarget target;     // ctrl+0x140 / ctrl+0x2e0
    float clipTime = 0;           // frames of the current clip at this update
    float rootTime = 0;           // the clip time of the previous root motion step
    bool clipFinished = false;    // slot kind 6
    float phase = 0;              // ctrl+0x24c
    float verticalSpeed = 0;      // ctrl+0x2e4
    float rootScale = 1;          // anim+0x20
    bool jumpWanted = false;      // comp+0x3c bit 5 of the pole
    bool offsetSampled = true;    // ctrl+0x28c != 0
    float hangOffsetY = kAcrobatPoleHangOffsetY;  // ctrl+0x2e8
    float hangOffsetZ = kAcrobatPoleHangOffsetZ;
    bool active() const { return state != 0 || pending != 0; }
};

struct AcrobatPoleInput {
    float dt = 1.0f / 30.0f;
    float position[3] = {0, 0, 0};
    std::int16_t facing = 0;
    bool bHeld = false;                 // ctrl+0xc bit 1
    bool stickHeld = false;             // ctrl+0xc bit 0
    std::int16_t stickHeading = 0;      // ctrl+6
    float cameraYaw = 0;                // *0x376894
    std::function<bool(const ClimbBarLink&)> linkEnabled;
    // TRAV-S26: the ground the moves 0x215804 / 0x20cd30 stand the player on: the floor height found below `position` within `reach`.
    std::function<std::optional<float>(const float position[3], float reach)> ground;
};

struct AcrobatPoleStep {
    bool riding = true;                 // false: released to the fall (state 5)
    int state = 0;
    float position[3] = {0, 0, 0};
    std::int16_t facing = 0;
    int animation = kAnimationAcrobatPoleIdle;
    float clipTime = 0;
    std::vector<ClimbBarSound> sounds;
};

// The arrival on an acrobat pole is done (state 88 requests 93): state 93 is entered in the next update with the hang offset of the grab.
void acrobatPoleStart(AcrobatPoleRider& rider, int pole);

// 0x1e4744: the stick asks for the flip when it points to the screen side away from the one the player faces (TRAV-S18: camera 0's x row is
// taken as (cos yaw, 0, -sin yaw) of the camera yaw).
bool acrobatPoleFlipTest(std::int16_t facing, bool stickHeld, std::int16_t stickHeading, float cameraYaw);

// One update while an acrobat pole state owns the player (TRAV-S25 for the order inside the update).
AcrobatPoleStep acrobatPoleUpdate(AcrobatPoleRider& rider, const ClimbBarSet& set, const AcrobatPoleClips& clips, const AcrobatPoleInput& input);

}
