#pragma once
// Ziplines (object type 101 with GrappleLineType 1, class props/parkour_zipline) and the player states "Zipline Get On" (42),
// "Zipline Slide" (43) and "Zipline Jump" (44): the anchor link 0x19b074, the grab test of the jump update 0x2a9d94, the use press
// on an anchor (handler 0x1b6664, message 3), the attach 0x275000, the slide update 0x2c4114 and the dismount (enter 0x2e82b8, root
// motion of 0x215804). Rules and measurements: docs/re/player-traversal.md. No SDL dependency.
#include "assets/assets.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr int kGrappleLineObjectType = 101;
constexpr int kGrappleLineZipline = 1;            // GrappleLineType (+0x162); 0 is the tightrope
constexpr float kZipLineSpeedDefault = 7.0f;      // 0x1a999c: ZipLineSpeed (+0x154) when the object has none
constexpr float kZiplineFollowFactor = 0.2f;      // 0x2c4114: literal 0x3e4ccccd at 0x2c42ec
constexpr int kZiplineTurnStep = 0x1900;          // *0x3598c4, the turn limit of 0x1ff030 / 0x23df04
constexpr int kZiplineUpdateRate = 30;            // 0x23ca4c (updates per second)
constexpr int kZiplineGrabJumpCount = 4;          // 0x1d5474: the grab tests of the jump update run once ctrl+0x2cf > 4
constexpr float kZiplineHeadingScale = 10430.378f;  // 0x275138
// Player animation roles (tablesanimations sheet 1): the state objects 0x5925a0 (43), 0x592584 (42) and 0x5925bc (44), role at +0x18.
constexpr int kAnimationZiplineSlide = 21;
constexpr int kAnimationZiplineGrab = 22;
constexpr int kAnimationZiplineDismount = 23;
// *(0x377470 + 0x20), read in the save state under the zipline of mission 1-2: 107, tablessoundfx row 107 "abilities/zipline_loop"; 0x28aca0 plays it, 0x28acc8 stops it.
constexpr std::uint16_t kZiplineLoopSound = 107;
// TRAV-S1: the grab point is the player position moved 0.08 along the facing (entity+0x60 = (0, 1.15, 0.08) read in the save state under the zipline of mission 1-2 and
// turned by the node in 0x2a9d94, y replaced by the node y). Its source in the character data is not traced.
constexpr float kZiplineGrabForwardOffset = 0.08f;

struct ZiplineAnchor {
    std::string instance;                 // lower case
    std::size_t order = 0;                // TRAV-S2: level order, stands in for the entity id +0x14 that 0x1a9bb4 compares
    float position[3] = {0, 0, 0};        // node translation
    int target = -1;                      // TargetTightropeAnchor (+0xf4), index in the anchor list
    float speed = kZipLineSpeedDefault;   // ZipLineSpeed (+0x154)
    int lineType = 0;                     // GrappleLineType (+0x162)
    int faceTowards = 0;                  // FaceTowards, low 3 bits of the use registry flags (0x23e130)
    float classWalkCentre[3] = {0, 0, 0}, classWalkHalf[3] = {0, 0, 0};  // class bound WalkBound
    bool hasUseBound = false;
    float useCentre[3] = {0, 0, 0}, useHalf[3] = {0, 0, 0};             // class bound UseBound
    // Filled by linkZiplineAnchors (0x19b074, 0x1a9bb4):
    float right[3] = {1, 0, 0}, up[3] = {0, 1, 0}, forward[3] = {0, 0, 1};  // node rows; forward is horizontal, towards the target
    float walkCentre[3] = {0, 0, 0}, walkHalf[3] = {0, 0, 0};  // WalkBound spread along the line
    float endOffset = 0;                  // +0x110: class WalkBound centre z minus half z
    bool listed = false;                  // in the line list 0x592490
};

// 0x19b074 and 0x1a9bb4 for every anchor whose target is set: node rows, WalkBound along the line and the line list.
void linkZiplineAnchors(std::vector<ZiplineAnchor>& anchors);

// The type 101 objects of a level whose class gives GrappleLineType 1, with their targets resolved and linked.
std::vector<ZiplineAnchor> loadZiplineAnchors(const assets::Level& level, const assets::ClassLookup& classOf);

// 0x2a9d94 with the height of the character's attachment set 18 taken as 0 (the list is empty for the player, the save state under the zipline of mission 1-2): the first
// listed anchor whose WalkBound, with the height measured from the line, contains the point; -1 when none. `enabled` tells
// whether an anchor object and its target are not disabled.
int ziplineGrabTest(const std::vector<ZiplineAnchor>& anchors, const float point[3], const std::function<bool(const ZiplineAnchor&)>& enabled);

// The use press (registry walk 0x1fee94 -> 0x269e30 -> 0x221570, handler 0x1b6664 message 3): the first anchor whose UseBound holds
// the player position in the anchor's frame, which the player faces (FaceTowards 1) and which lies higher than its target; -1 when none.
int ziplineUseTest(const std::vector<ZiplineAnchor>& anchors, const float player[3], std::int16_t facing,
                   const std::function<bool(const ZiplineAnchor&)>& enabled);

enum class ZiplinePhase { None, GetOn, Slide, Dismount };

struct ZiplineRider {
    ZiplinePhase phase = ZiplinePhase::None;
    int line = -1;              // ctrl+0x130: the higher anchor of the line
    float t = 0;                // ctrl+0x24c, along the line from its start point to its end point
    float rate = 0;             // ctrl+0x250, per second
    std::int16_t heading = 0;   // ctrl+0xa
    unsigned phaseUpdates = 0;  // updates since the phase was entered
    bool endReached = false;    // event 6 of 0x2c4114, applied at the next update
    float base[3] = {0, 0, 0};  // dismount start position
    float fallSpeed = 0;        // ctrl+0x2b8: height lost in the last update
    float fallDistance = 0;     // ctrl+0x2b4
    int jumpCount = -1;         // ctrl+0x2cf while the jump state runs, -1 outside it
    bool active() const { return phase != ZiplinePhase::None; }
};

struct ZiplineInput {
    float dt = 1.0f / 30.0f;
    float position[3] = {0, 0, 0};  // player node position
    std::int16_t facing = 0;        // ctrl+8
    bool jumpPressed = false;       // B press edge (event 6 in states 42 and 43)
    unsigned grabUpdates = 38;      // updates of the zipline_grab clip (anim done event of state 42)
    unsigned dismountUpdates = 30;  // updates of the zipline_dismount clip (anim done event of state 44)
    const assets::RootMotion* dismountMotion = nullptr;  // root motion of zipline_dismount
    float dismountFramesPerSecond = 15.0f;
    std::function<bool(const ZiplineAnchor&)> enabled;  // the anchor object is not disabled
};

enum class ZiplineSoundAction { Play, Stop };
struct ZiplineSound {
    ZiplineSoundAction action = ZiplineSoundAction::Play;
    std::uint16_t id = kZiplineLoopSound;
};

struct ZiplineStep {
    bool riding = false;            // the zipline states own the player in this update
    float position[3] = {0, 0, 0};
    std::int16_t facing = 0;
    int animation = 0;              // role to play
    bool falling = false;           // state 5 starts now: fall with fallSpeed and fallDistance
    float fallSpeed = 0, fallDistance = 0;
    std::vector<ZiplineSound> sounds;
};

// The attach in the update the grab or the use press happened (state 42 enter 0x2ecb30 -> 0x275000) followed by the first slide update.
// `listed` is the anchor the test found (ctrl+0x134).
ZiplineStep ziplineEnter(ZiplineRider& rider, const std::vector<ZiplineAnchor>& anchors, int listed, const ZiplineInput& input);

// One update while riding: the dismount request of the last update, the B press, the Get On -> Slide switch, the slide 0x2c4114, the
// dismount root motion and its end (state 5). TRAV-S3: during the dismount the fall distance (ctrl+0x2b4) follows the save state under the zipline of mission 1-2 trace (the
// height lost over the current key interval of the root track while descending, the last update's change otherwise); the rule of
// 0x215804 is not read.
ZiplineStep ziplineUpdate(ZiplineRider& rider, const std::vector<ZiplineAnchor>& anchors, const ZiplineInput& input);

// The jump counter ctrl+0x2cf: -1 when state 4 is entered (0x200e6c), +1 by the third function 0x1b8560 of state 4 in every update.
// Returns whether the grab tests of the jump update 0x1d5254 run in this update; call once per update with the posture before and
// after the player step.
bool ziplineJumpTick(ZiplineRider& rider, bool jumpStarted, bool jumping);

}
