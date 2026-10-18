#pragma once
// Climbing bricks (object type 20, class props/parkour_brickgrab_new) and the player states "Brickgrab grab" (141) to "Brickgrab jump
// right" (151) and "BRICK GRAB JUMP UP" (152): the link 0x198ae0, the brick update 0x1a649c, the grab test of the jump update 0x273f6c,
// the state objects (init 0x34999c, event lists 0x2a0e08), their enter and update functions and the event handlers 0x31a408 (grab),
// 0x31a49c (jump), 0x31c430 (check), 0x31c488 (reach) and 0x3233e8 (released). Rules and measurements: docs/re/player-traversal.md.
// No SDL dependency.
#include "assets/assets.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr int kBrickGrabObjectType = 20;
constexpr std::size_t kAdjacentBrickCount = 8;       // AdjacentBrick01..08, link 0x198ae0 (+0x98)
constexpr float kBrickHangDrop = 2.0f;               // 0x32c4b8 / 0x32c5b8: the player node is the brick node 2 lower, turned by pi
constexpr int kBrickReleaseCooldown = 15;            // 0x1a649c: +0xc0 while held, then counted down per update
constexpr float kBrickGrabFacingLimit = 0.75f;       // 0x273fe4: brick forward . player forward above it fails
constexpr float kBrickGrabBoxHeightScale = 0.25f;    // 0x273fe4 literal 0x274180: half height of the player box
constexpr float kBrickGrabBoxRaise = 0.75f;          // 0x273fe4 literal 0x274184: box centre y = offset y * (1 + 0.75)
constexpr float kBrickReachLimit = 0.5f;             // 0x31c488: stick . brick rows
constexpr float kBrickUpwardLimit = -0.5f;           // 0x31a49c: stick . brick forward below it adds the brick's up y
constexpr float kBrickJumpUpLimit = 0.707f;          // 0x31a49c literal 0x3f34fdf4
constexpr float kBrickHeadingScale = 10430.378f;     // radians to the 16-bit heading
constexpr int kBrickGrabJumpCount = 4;               // 0x1d5474: the grab tests of the jump update run once ctrl+0x2cf > 4

// Player animation roles (tablesanimations sheet 1): +0x18 of the state objects 0x58dc98..0x58ddb0; 91 is played by 0x2abcb8.
constexpr int kAnimationBrickIdle = 90;
constexpr int kAnimationBrickTopJump = 91;
constexpr int kAnimationBrickJumpLeft = 92;
constexpr int kAnimationBrickJumpRight = 93;
constexpr int kAnimationBrickJumpUp = 94;
constexpr int kAnimationBrickGrab = 95;
constexpr int kAnimationBrickGrabLeft = 96;
constexpr int kAnimationBrickGrabRight = 97;
constexpr int kAnimationBrickGrabUp = 98;
constexpr int kAnimationBrickReachLeft = 99;
constexpr int kAnimationBrickReachRight = 100;
constexpr int kAnimationBrickReachUp = 101;
constexpr std::array<int, 12> kBrickGrabAnimations = {kAnimationBrickIdle,     kAnimationBrickTopJump,  kAnimationBrickJumpLeft,
                                                      kAnimationBrickJumpRight, kAnimationBrickJumpUp,  kAnimationBrickGrab,
                                                      kAnimationBrickGrabLeft, kAnimationBrickGrabRight, kAnimationBrickGrabUp,
                                                      kAnimationBrickReachLeft, kAnimationBrickReachRight, kAnimationBrickReachUp};

// TRAV-S10 stand-in: the player's shape for the grab test, entity+0x60 (offset) and entity+0x6c (half extents) read in m12-11; their
// source in the character data is not traced.
constexpr float kStandInPlayerGrabOffset[3] = {0.0f, 1.15f, 0.08f};
constexpr float kStandInPlayerGrabHalf[3] = {0.6f, 0.85f, 0.52f};

struct GrabBrick {
    std::string instance;                      // lower case
    std::array<float, 16> matrix{};            // node rows right, up, forward, translation (assets::placementMatrix)
    std::array<int, kAdjacentBrickCount> adjacent{};  // AdjacentBrick01..08 as indices in the brick list, -1 when unset
    bool topBrick = false;                     // IsTopBrick (+0xc2 bit 0)
    bool hasGrabBound = false;
    float grabCentre[3] = {0, 0, 0}, grabHalf[3] = {0, 0, 0};  // class bound GrabBound (+0x94)
    bool held = false;                         // +0xb8: the player hangs on it
    int cooldown = 0;                          // +0xc0
    const float* position() const { return &matrix[12]; }
    const float* right() const { return &matrix[0]; }
    const float* up() const { return &matrix[4]; }
    const float* forward() const { return &matrix[8]; }
};

// The type 20 objects of a level in level order (the brick list 0x37718c), their adjacent bricks resolved.
std::vector<GrabBrick> loadGrabBricks(const assets::Level& level, const assets::ClassLookup& classOf);

// 0x1a649c for every brick: the cooldown is 15 while the brick is held and counts down after.
void updateGrabBricks(std::vector<GrabBrick>& bricks);

enum class BrickState {
    None = 0,
    Grab = 141,
    GrabUp = 142,
    GrabLeft = 143,
    GrabRight = 144,
    Idle = 145,
    ReachUp = 146,
    ReachLeft = 147,
    ReachRight = 148,
    JumpUp = 149,
    JumpLeft = 150,
    JumpRight = 151,
    TopJump = 152,
};
int brickStateAnimation(BrickState state);

struct BrickClimber {
    BrickState state = BrickState::None;
    int brick = -1;              // ctrl+0x130
    int target = -1;             // ctrl+0x134
    unsigned stateUpdates = 0;   // updates since the state was entered
    float elapsed = 0;           // clip time of the jump states
    float start[3] = {0, 0, 0};  // node position when a jump state began
    float scale[3] = {1, 1, 1};  // 0x32c6fc: root motion scale of the jump clip (0x211a00)
    float lastY = 0;
    int jumpCount = -1;          // ctrl+0x2cf while the jump state 4 runs, -1 outside it
    bool active() const { return state != BrickState::None; }
};

struct BrickClip {
    unsigned updates = 0;                      // updates until the clip's done event (event 1)
    const assets::RootMotion* motion = nullptr;
    float framesPerSecond = 15.0f;
};

struct BrickInput {
    float dt = 1.0f / 30.0f;
    float position[3] = {0, 0, 0};             // player node position
    std::int16_t facing = 0;                   // ctrl+8
    bool stickHeld = false;                    // ctrl+0xc bit 0
    std::int16_t stickHeading = 0;             // ctrl+6
    float cameraYaw = 0;                       // *0x376894
    float cameraForward[3] = {0, 0, 1};        // camera matrix row 2 (TRAV-S13 stand-in in the glue)
    bool jumpPressed = false;                  // B, event 6
    bool usePressed = false;                   // A, event 7
    std::map<int, BrickClip> clips;            // by animation role
    std::function<bool(const GrabBrick&)> enabled;  // the brick object is not disabled (+0x10 bit 0) and shown (+0x8c bit 4)
};

enum class BrickExit { None, Fall, Jump };

struct BrickStep {
    bool holding = false;                      // the brick states own the player in this update
    float position[3] = {0, 0, 0};
    std::int16_t facing = 0;
    int animation = kAnimationBrickIdle;
    BrickExit exit = BrickExit::None;          // Fall: state 5 with fallSpeed; Jump: state 4 starts in this update
    float fallSpeed = 0;
    BrickState entered = BrickState::None;     // the state entered in this update, None when it did not change
};

// The jump counter ctrl+0x2cf as the zipline keeps it (0x200e6c sets -1, 0x1b8560 adds 1 per update of state 4). Returns whether the
// grab tests of the jump update 0x1d5254 run in this update.
bool brickJumpTick(BrickClimber& climber, bool jumpStarted, bool jumping);

// 0x273f6c -> 0x273fe4 over the brick list: the first free, shown brick whose GrabBound box overlaps the player's grab box and which the
// player does not face along; -1 when none.
int brickGrabTest(const std::vector<GrabBrick>& bricks, const float position[3], std::int16_t facing,
                  const std::function<bool(const GrabBrick&)>& enabled);

// State 141 entered from the jump update (ctrl+0x134 = the brick): the snap of 0x32c4b8.
BrickStep brickGrabEnter(BrickClimber& climber, std::vector<GrabBrick>& bricks, int brick);

// One update while the brick states run. TRAV-S11 stand-in: one state change per update, events in the order B (6), A (7), check (3),
// clip done (1), stick (9); the released event (5) at every state change.
BrickStep brickGrabUpdate(BrickClimber& climber, std::vector<GrabBrick>& bricks, const BrickInput& input);

}
