#pragma once
// Poles and drain pipes (object type 187, classes props/parkour_drainpipe and props/prop_poleclimb) and the player states POLE MOUNT
// (0x11d), POLE CLIMB UP (0x11e), POLE CLIMB DOWN (0x11f), POLE IDLE (0x120) and POLE DISMOUNT (0x121). Rules and measurements:
// docs/re/player-traversal.md (pole entries). No SDL dependency.
#include "assets/assets.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr int kPoleObjectType = 187;
// Player animation roles (tablesanimations sheet 1): enters 0x26b700 (218 in the air, 219), 0x25f7c4 (219), 0x282fec (220), 0x29a274 (221), 0x28c10c (222).
constexpr int kAnimationPoleMount = 218;
constexpr int kAnimationPoleIdle = 219;
constexpr int kAnimationPoleUp = 220;
constexpr int kAnimationPoleDown = 221;
constexpr int kAnimationPoleDismount = 222;
constexpr float kPoleMountRate = 6.6666665f;     // 0x28d908: ctrl+0x24c += dt x this, up to 1
constexpr int kPoleTurnStep = 0x1900;            // 0x23df04 limit of the facing turn in the mount update
constexpr float kPoleMountPointDepth = -0.5f;    // 0x28d908: the pole's bound centre moved 0.5 along its -z before the facing target is taken
constexpr float kPoleClimbStepDefault = 0.05f;   // *0x374524 = 0x3d4ccccd (read in m12-10): the step of 0x1d579c / 0x283f18 for a clip without root track
constexpr float kPoleFallHeight = 5.0f;          // 0x1d579c: an A press above this height over the pole base (bits compared with 0x40a00001) drops the player
constexpr int kPoleStickCone = 0x4000;           // 0x206510: the stick may differ from the direction to the pole by at most this
constexpr float kPoleHeadingScale = 10430.378f;
// ctrl+0x250, read in m12-10 (0x3fea8f5c): the reach the player's head adds to the climb height when it is compared with the pole top.
// TRAV-S5: its source in the character data is not traced; it is taken as given for the player character.
constexpr float kPoleReach = 1.8325f;
// TRAV-S6: the distance from the pole axis at which the player stands after leaving the pole in the first update (A, B, bottom exit): the
// original's collision pushes him out of the pole's CollisionBound. Modelled as half depth of that bound + 0.6 (player box centre z 0.08 +
// half z 0.52) + this margin, measured as 0.77038 for 1-5_Drain_Pipe05 (oracle runs foot-walk, pole-A1, pole-down).
constexpr float kPoleExitMargin = 0.02038f;

struct PoleObject {
    std::string instance;                 // lower case
    float position[3] = {0, 0, 0};        // node translation
    float right[3] = {1, 0, 0}, up[3] = {0, 1, 0}, forward[3] = {0, 0, 1};  // node rows from the orientation (yaw only: TRAV-S9)
    std::int16_t heading = 0;             // atan2(forward.x, forward.z) x 10430.378, the handler's ctrl+0xa before the turn by pi
    float snapOffset = 0;                 // ATTR_CharacterSnapToOffset (+0x7c component +0x18)
    bool canDismount = false;             // CanDismount (component +0x1c bit 1)
    float collisionCentre[3] = {0, 0, 0}, collisionHalf[3] = {0, 0, 0};  // class bound CollisionBound: +0x60 and +0x6c of the object
    float useCentre[3] = {0, 0, 0}, useHalf[3] = {0, 0, 0};              // class bound UseBound
    float bottom() const { return collisionCentre[1] - collisionHalf[1]; }   // pole+0x64 - pole+0x70
    float top() const { return collisionHalf[1] + collisionCentre[1]; }      // pole+0x70 + pole+0x64
};

std::vector<PoleObject> loadPoles(const assets::Level& level, const assets::ClassLookup& classOf);

// 0x221570 for the pole's use zone (FaceTowards forced to 1 by 0x185420): the player's position in the pole's frame inside the UseBound
// and the player facing the pole (dot of the facing and the direction to the pole >= 0).
bool poleZoneHolds(const PoleObject& pole, const float player[3], std::int16_t facing);

// 0x20cd30 touch of the pole's collision. TRAV-S7: the original sets DAT_00377134 in the wall gather 0x21a73c; here the player's box
// (centre (0, 1.15, 0.08), half (0.6, 0.85, 0.52) in the node frame, yaw = facing) must overlap the pole's CollisionBound box.
bool poleTouched(const PoleObject& pole, const float player[3], std::int16_t facing);

// 0x206510 (called by the walk update 0x26aca8 through 0x2010d4): the touched pole whose zone holds, with a stick held within a quarter
// turn of the direction to the pole. Returns the index or -1. `disabled` tells whether the pole object is disabled.
int poleWalkInTest(const std::vector<PoleObject>& poles, const float player[3], std::int16_t facing, bool stickActive, std::int16_t stickHeading,
                   const std::function<bool(const PoleObject&)>& disabled);

// 0x2011b0 -> registry walk 0x1fef10 with pressed 1 on an A press in the idle, walk and run states: the first pole whose zone holds.
int poleUseTest(const std::vector<PoleObject>& poles, const float player[3], std::int16_t facing,
                const std::function<bool(const PoleObject&)>& disabled);

enum class PolePhase { None, Mount, Idle, Up, Down, Dismount };
enum class PoleExit { None, Fall, Jump, Ground };

struct PoleClimber {
    PolePhase phase = PolePhase::None;
    int pole = -1;                  // ctrl+0x130
    float height = 0;               // ctrl+0x24c: mount blend, then the height over the pole's origin
    float start[3] = {0, 0, 0};     // ctrl+0x25c
    float target[3] = {0, 0, 0};    // ctrl+0x268 (y = ctrl+0x26c)
    std::int16_t targetHeading = 0; // ctrl+0xa
    unsigned phaseUpdates = 0;
    float base[3] = {0, 0, 0};      // dismount start
    bool active() const { return phase != PolePhase::None; }
};

struct PoleClips {
    const assets::RootMotion* up = nullptr;
    const assets::RootMotion* down = nullptr;
    const assets::RootMotion* dismount = nullptr;
    float upFramesPerSecond = 15, downFramesPerSecond = 15, dismountFramesPerSecond = 15;
    unsigned upFrames = 10, downFrames = 15;       // looped clip lengths (animationLength)
    unsigned dismountFrames = 17;                  // length of vpole_dismount
};

struct PoleInput {
    float dt = 1.0f / 30.0f;
    float position[3] = {0, 0, 0};
    std::int16_t facing = 0;           // ctrl+8
    bool stickActive = false;          // ctrl+0xc bit 0
    std::int16_t stickHeading = 0;     // ctrl+6
    float cameraYaw = 0;               // *0x376894
    bool aPressed = false;             // ctrl+0xe bit 2
    bool bPressed = false;             // ctrl+0xe bit 1
    PoleClips clips;
    // TRAV-S8: 0x22f134 casts a ray of the step length from the player; modelled as: a floor within that distance below the node.
    std::function<bool(const float position[3], float length)> floorBelow;
    // TRAV-S8: ground contact of the dismount move 0x20cd30: the floor height under the node, when there is one in reach.
    std::function<std::optional<float>(const float position[3], float reach)> floorHeight;
};

struct PoleStep {
    bool riding = false;               // the pole states own the player in this update
    float position[3] = {0, 0, 0};
    std::int16_t facing = 0;
    int animation = 0;
    PoleExit exit = PoleExit::None;    // the player leaves the pole in this update
    bool holdFacing = false;
};

// The mount from the walk-in (0x206510) or the A press (0x2011b0, `byUse`): the handler 0x191ac0 for message 3 with pressed 1, then the
// enter 0x26b700. Ground entry only (the air entry with clip 218 is not ported: TRAV-11).
PoleStep poleMount(PoleClimber& climber, const std::vector<PoleObject>& poles, int index, const PoleInput& input, bool byUse);

// What a level keeps for the poles: the objects, the climber and the inputs the glue gathers for the update.
struct PoleSession {
    std::vector<PoleObject> poles;
    PoleClimber climber;
    std::optional<assets::RootMotion> up, down, dismount;  // root motion of vpole_up, vpole_down and vpole_dismount, read from the player's clips
    float cameraYaw = 0;
    bool usePressed = false;  // the A press edge (ctrl+0xe bit 2)
};

// One update while on the pole (the update functions 0x28d908, 0x283f18, 0x1d579c, 0x2ac32c).
PoleStep poleUpdate(PoleClimber& climber, const std::vector<PoleObject>& poles, const PoleInput& input);

// Root motion of a looped clip over one update: root(end) - root(start) with the frame counted from the state's first update.
float poleClimbStep(const assets::RootMotion& motion, unsigned frames, unsigned update, float dt, float framesPerSecond);

}
