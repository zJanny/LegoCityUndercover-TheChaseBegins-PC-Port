#pragma once
// Ladders (object kind 56: props/prop_ladderlarge, props/prop_laddermedium, props/prop_crane_ladderlarge, ...) and the player states
// Ladder Mount (33), Ladder Climb Up (34), Ladder Climb Down (35), Ladder Idle (36), Ladder Idle (Blocked) (37) and the clamber at the
// top, Ledge dismount clamber (23). Rules and measurements: docs/re/player-traversal-ladders.md. No SDL dependency.
#include "assets/assets.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr int kLadderObjectKind = 56;
// Clip ids of the state objects (+0x18; tablesanimations sheet 1): ladder_idle for 33 / 36 / 37, ladder_up, ladder_down, and +0x1c of
// the mount state (ladder_dismount), the clip of state 23.
constexpr int kAnimationLadderIdle = 10;
constexpr int kAnimationLadderUp = 11;
constexpr int kAnimationLadderDown = 12;
constexpr int kAnimationLadderDismount = 13;
constexpr float kLadderMountStep = 0.25f;         // 0x325380: ctrl+0x24c += dt x 30 x 0.25, up to 1
constexpr float kLadderClimbStep = 0.05f;         // TRAV-S34: runtime global *0x377274 read as 0x3d4ccccd (writer not read): 0x1e43c0 steps 0.05 x sign x 30 x dt when the clip has no root track
constexpr float kLadderSnapOffsetDefault = 0.3f;  // 0x17f3c0: ATTR_CharacterSnapToOffset default 0x3e99999a
constexpr float kLadderHeadingScale = 10430.378f;
// TRAV-S34: entity+0x70 of the player (0x3f59999a, read in the save state in front of the crane ladder of mission 1-3, writer not read): the climb down ends below the ladder base by this much (0x32f83c).
constexpr float kLadderPlayerHalfHeight = 0.85f;

struct LadderObject {
    std::string instance;                 // lower case
    float position[3] = {0, 0, 0};        // node translation
    float right[3] = {1, 0, 0}, up[3] = {0, 1, 0}, forward[3] = {0, 0, 1};  // node rows from the yaw only (TRAV-S34)
    std::int16_t heading = 0;             // atan2(forward.x, forward.z) x 10430.378
    float snapOffset = kLadderSnapOffsetDefault;  // ATTR_CharacterSnapToOffset (+0x94)
    bool canDismount = true;              // CanDismount (default 1; +0x9c bit 1 is its inverse)
    bool furled = false;                  // StartFurled (+0x9c bit 3 after the post-link 0x17f540: the handler answers 0)
    int faceTowards = 0;                  // FaceTowards of the use zone (0x23e130), low 3 bits
    bool useStartActive = true;           // UseStartActive (default 1)
    bool hasUseBound = false;
    float collisionCentre[3] = {0, 0, 0}, collisionHalf[3] = {0, 0, 0};  // class bound CollisionBound: +0x60 and +0x6c of the object
    float useCentre[3] = {0, 0, 0}, useHalf[3] = {0, 0, 0};              // class bound UseBound
    float top() const { return collisionHalf[1] + collisionCentre[1]; }  // ladder+0x70 + ladder+0x64
};

// The placed objects of kind 56. TRAV-S34: the kind 56 objects nested in a class file (ladder_part_02 of the crane ladders) are not loaded;
// the parent's bounds cover them (the walk-in test 0x206668 hands a touched part to the object 0x239678 names, entity+0x80 not traced).
std::vector<LadderObject> loadLadders(const assets::Level& level, const assets::ClassLookup& classOf);

// 0x221570 for a ladder's use zone: UseStartActive, the FaceTowards test of the player (modes 1..5) and the player's position in the
// ladder's frame inside the UseBound. A ladder without UseBound has no zone here (0x23e130 would build one from the model bounds).
bool ladderZoneHolds(const LadderObject& ladder, const float player[3], std::int16_t facing);

// The handler 0x18b458 for message 3 or 4 with pressed 1, with ability 5 taken as present (TRAV-S34): refuses a furled ladder and a
// player whose height is above the ladder top minus the clamber rise.
bool ladderAccepts(const LadderObject& ladder, const float player[3], float clamberRise);

// 0x206668 (walk, fall and jump updates through 0x2010d4; the port runs it after a ground step only): the player touched a ladder
// (TRAV-S35: the box overlap of TRAV-S7 stands in for DAT_00377134) whose zone holds and whose handler accepts. Returns the index or -1.
int ladderWalkInTest(const std::vector<LadderObject>& ladders, const float player[3], std::int16_t facing, float clamberRise,
                     const std::function<bool(const LadderObject&)>& disabled);

// 0x2011b0 -> registry walk 0x1fef10 with pressed 1 on an A press: the first ladder whose zone holds and whose handler accepts.
int ladderUseTest(const std::vector<LadderObject>& ladders, const float player[3], std::int16_t facing, float clamberRise,
                  const std::function<bool(const LadderObject&)>& disabled);

enum class LadderPhase { None, Mount, Idle, Blocked, Up, Down, Clamber };
enum class LadderExit { None, Fall, Jump, Ground };

struct LadderClimber {
    LadderPhase phase = LadderPhase::None;
    int ladder = -1;                // ctrl+0x130 / ctrl+0x134
    float height = 0;               // ctrl+0x24c: mount blend, then the height over the ladder's origin
    float start[3] = {0, 0, 0};     // ctrl+0x25c
    float target[3] = {0, 0, 0};    // ctrl+0x268 (y = ctrl+0x26c)
    unsigned phaseUpdates = 0;
    bool active() const { return phase != LadderPhase::None; }
};

struct LadderInput {
    float dt = 1.0f / 30.0f;
    float position[3] = {0, 0, 0};
    std::int16_t facing = 0;           // ctrl+8
    bool stickActive = false;          // ctrl+0xc bit 0
    std::int16_t stickHeading = 0;     // ctrl+6
    float cameraYaw = 0;               // *0x376894
    bool aPressed = false;             // event 7 "Ladder Jump Off Event"
    bool bPressed = false;             // event 6 "Default Jump Event"
    float clamberRise = 0;             // ctrl+0x250: root y of ladder_dismount at its last frame (0x221a54)
    const assets::RootMotion* clamber = nullptr;  // root motion of ladder_dismount
    float clamberFramesPerSecond = 15;
    unsigned clamberFrames = 15;       // animationLength of ladder_dismount
    // TRAV-S35: the ray 0x22f134 from 0.3 above the node to 0.1 + the step below it, modelled as a floor within that distance.
    std::function<bool(const float position[3], float length)> floorBelow;
    // TRAV-S35: ground contact of the clamber move 0x215804 without its walls, the floor height under the node within reach.
    std::function<std::optional<float>(const float position[3], float reach)> floorHeight;
};

struct LadderStep {
    bool riding = false;               // the ladder states own the player in this update
    float position[3] = {0, 0, 0};
    std::int16_t facing = 0;           // ctrl+8
    std::int16_t targetHeading = 0;    // ctrl+0xa: the facing, or the ladder heading after a Ladder Jump Off Event
    int animation = 0;
    LadderExit exit = LadderExit::None;
};

// The handler 0x18b458 (pressed 1) and the enter 0x325258: Ladder Mount from the player's position. `byUse` is the A press, after which
// 0x2011b0 leaves the facing; the walk-in 0x206668 sets ctrl+8 to the handler's heading at once.
LadderStep ladderMount(LadderClimber& climber, const std::vector<LadderObject>& ladders, int index, const LadderInput& input, bool byUse);

// One update while on the ladder: 0x325380 (mount), 0x1e43c0 (idle), 0x32ca18 (up), 0x32f83c (down), 0x302e94 (clamber).
LadderStep ladderUpdate(LadderClimber& climber, const std::vector<LadderObject>& ladders, const LadderInput& input);

// What a level keeps for the ladders: the objects, the climber and the inputs the glue gathers for the update.
struct LadderSession {
    std::vector<LadderObject> ladders;
    LadderClimber climber;
    std::optional<assets::RootMotion> clamber;  // root motion of ladder_dismount
};

}
