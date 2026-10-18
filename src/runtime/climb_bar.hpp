#pragma once
// Rails and climb bars (object type 29, classes props/parkour_railgrab, prop_climbbar, parkour_flagpole) with the corner rail
// (type 218, props/parkour_cornerrail) and the player states "CLIMB BAR IDLE / LEFT / RIGHT" (0x59, 0x5a, 0x5b), the grab arrival
// (0x58) and "CORNER RAIL LEFT / RIGHT" (0x179, 0x17a): grab test 0x206240 -> 0x295da0, grab 0x2158a4, arrival update 0x320ae4,
// hang update 0x2a1c54 with the move 0x1fc2fc / 0x2bbdc0, corner update 0x18fe40 and the corner states 0x2766bc / 0x29a384.
// Rules and measurements: docs/re/player-traversal.md. No SDL dependency.
#include "assets/assets.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr int kClimbBarObjectType = 29;
constexpr int kCornerRailObjectType = 218;
// Clip ids of the state enters 0x3209d0 / 0x27fccc / 0x2766bc (tablesanimations sheet 1: ledge_idle, ledge_left, ledge_right).
constexpr int kAnimationLedgeIdle = 39;
constexpr int kAnimationLedgeLeft = 40;
constexpr int kAnimationLedgeRight = 41;
constexpr float kClimbBarFallbackStep = 0.065f;       // -*0x377218 (0xbd851eb8): the move of the update without clip root motion
constexpr float kClimbBarArrivalSpeed = 8.0f;         // *0x377224 (0x41000000): arrival speed of 0x320ae4 times dt
constexpr float kClimbBarFacingLimit = 0.75f;         // 0x295da0: dot of the two forward rows must be at most 0x3f400000
constexpr float kClimbBarDirectionLimit = 0.3f;       // 0x2a1c54: the stick against the bar counts beyond +-0x3e99999a
constexpr float kCornerRailRate = 6.0f;               // 0x29a384: t moves by dt * 6
constexpr float kCornerRailLength = 3.0f;             // 0x29a384: the corner state runs while 0 <= t < 3
constexpr int kCornerRailSamplesPerSegment = 10;      // 0x2766bc: argument 10 of the closest-point search 0x21d5b0
constexpr int kClimbBarGrabJumpCount = 4;             // the grab tests of the jump update run once ctrl+0x2cf > 4 (as the zipline)
// TRAV-S15: ctrl+0x2e8, the offset of the hanging player from the bar line (bar-local y and z), measured in the oracle state m12-12
// for Chase (0, -1.0606986, 0.3636877); the original samples the hand joint of the ledge clip (0x27086c) and does not store a constant.
constexpr float kClimbBarHangOffsetY = -1.0606986f;
constexpr float kClimbBarHangOffsetZ = 0.36368772f;
// TRAV-S21: ctrl+0x2e8 for an AcrobatPole (flag poles), the hanging player's offset from the bar line (bar-local y and z), measured in the
// oracle (m12-trampoline-built, grab of 1-2_FlagPole_01 from the bounce): the player ends the arrival at 41.45909 / 637.57922 under the pole
// at 723.11157 / 43.46950 / 637.55219 with yaw 180; the original samples the hand joint of the clip 0x2c.
constexpr float kAcrobatPoleHangOffsetY = -2.0104117f;
constexpr float kAcrobatPoleHangOffsetZ = -0.027031898f;
// Player box of the grab test 0x295da0: entity+0x60 (0, 1.15, 0.08) is its centre, entity+0x6c (0.6, 0.85, 0.52) its half extents
// (read in m12-12) with y scaled by *0x377220 - *0x37721c = 0.7 - 0.2 = 0.5.
constexpr float kClimbBarPlayerBoxCentre[3] = {0.0f, 1.15f, 0.08f};
constexpr float kClimbBarPlayerBoxHalf[3] = {0.6f, 0.85f * 0.5f, 0.52f};

struct ClimbBar {
    std::string instance;                 // lower case
    std::size_t order = 0;
    float position[3] = {0, 0, 0};
    float yawDegrees = 0;                 // TRAV-S18: only the yaw of the placement is used, pitch and roll are ignored
    float grabCentre[3] = {0, 0, 0}, grabHalf[3] = {0, 0, 0};  // class bound GrabBound
    bool hasGrabBound = false;
    bool acrobatPole = false, narrowBar = false, faceJumpDir = true, immediateJump = false, ledge = false;  // comp+0x3c bits 0..4
    std::uint16_t grabSound = 0, swingSound = 0;                                                         // ATTR_SFX_GRAB / ATTR_SFX_SWING
};

struct CornerRail {
    std::string instance;
    std::size_t order = 0;
    float position[3] = {0, 0, 0};
    float yawDegrees = 0;
    float useCentre[3] = {0, 0, 0}, useHalf[3] = {0, 0, 0};  // class bound UseBound
    std::vector<std::array<float, 3>> path;                  // the class path (Catmull-Rom, open), corner-local
};

struct ClimbBarSet {
    std::vector<ClimbBar> bars;
    std::vector<CornerRail> corners;
};

ClimbBarSet loadClimbBars(const assets::Level& level, const assets::ClassLookup& classOf);

// 0x202504: oriented box overlap (the box A with half extents `a`, the box B with half extents `b` at `t` in the frame of A,
// r[k][i] = dot(axis k of B, axis i of A)).
bool obbOverlap(const float a[3], const float t[3], const float b[3], const float r[3][3]);

// 0x1c17c0: the open Catmull-Rom spline through the points at parameter t (clamped to 0..n-1).
std::array<float, 3> splinePoint(const std::vector<std::array<float, 3>>& points, float t);

enum class ClimbPhase { None, Arrive, Idle, Left, Right, CornerLeft, CornerRight };

struct ClimbBarRider {
    ClimbPhase phase = ClimbPhase::None;
    int bar = -1;                 // ctrl+0x130 while on a bar
    int corner = -1;              // ctrl+0x130 while on a corner
    int ignoreBar = -1;           // ctrl+0x138: the bar just left, not grabbed again until the player box leaves it
    bool jumpLatch = false;       // ctrl+0x315 bit 0: the B press was used to leave
    float t = 0;                  // ctrl+0x24c on a corner
    float clipFrame = 0;          // playing time (frames) of the ledge_left / ledge_right clip since the state was entered
    bool grabbedFromBehind = false;  // the player was on the side of the bar's -z row (decides the facing of an acrobat pole)
    bool enteredThisUpdate = false;  // TRAV-S17: the first update of Idle after the arrival only places the player (enter 0x27fccc)
    int jumpCount = -1;           // ctrl+0x2cf while the jump state runs, -1 outside it
    bool active() const { return phase != ClimbPhase::None; }
};

struct ClimbBarInput {
    float dt = 1.0f / 30.0f;
    float position[3] = {0, 0, 0};
    std::int16_t facing = 0;
    bool stickHeld = false;       // ctrl+0xc bit 0
    std::int16_t stickHeading = 0;  // ctrl+6
    bool bHeld = false;           // ctrl+0xc bit 1
    bool aPressed = false;        // ctrl+0xe bit 2
    const assets::RootMotion* leftMotion = nullptr;   // root motion of ledge_left / ledge_right (x along the bar)
    const assets::RootMotion* rightMotion = nullptr;
    float clipFramesPerSecond = 15.0f;
    std::function<bool(const ClimbBar&)> enabled;
};

struct ClimbBarSound {
    std::uint16_t id = 0;
};

enum class ClimbBarRelease { None, Fall, Jump };

struct ClimbBarStep {
    bool riding = false;
    float position[3] = {0, 0, 0};
    std::int16_t facing = 0;
    int animation = 0;
    ClimbBarRelease release = ClimbBarRelease::None;
    std::vector<ClimbBarSound> sounds;
};

// 0x206240 / 0x295da0 over the bars in level order: the first bar whose box meets the player box (not the one in ctrl+0x138, which
// is forgotten as soon as its box no longer meets the player box); -1 when none. TRAV-S16: the ability check 0x227b5c (0x219a8c)
// is taken as passed for Chase, the bar state comp+2 in 1..3 as always given.
int climbBarGrabTest(const ClimbBarSet& set, ClimbBarRider& rider, const float position[3], std::int16_t facing,
                     const std::function<bool(const ClimbBar&)>& enabled);

// State 0x58 entered (0x2158a4 -> 0x3209d0): the grab sound and the ledge_idle clip; the arrival starts in the next update.
ClimbBarStep climbBarGrab(ClimbBarRider& rider, const ClimbBarSet& set, int bar, const ClimbBarInput& input);

// One update while a bar or corner state owns the player.
ClimbBarStep climbBarUpdate(ClimbBarRider& rider, const ClimbBarSet& set, const ClimbBarInput& input);

// The jump counter ctrl+0x2cf as for the zipline: whether the grab tests of the jump update run in this update.
bool climbBarJumpTick(ClimbBarRider& rider, bool jumpStarted, bool jumping);

// Left the bar by a jump or a fall: remember it in ctrl+0x138.
void climbBarLeave(ClimbBarRider& rider);

}
