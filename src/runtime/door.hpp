#pragma once
// Doors (object type 42, classes props/prop_door and leprops/leprop_door): the state machine of the door update 0x1d16b8 and the
// state change 0x1a6b6c, the messages of the handler 0x1749b8 and the motion 0x26905c. Rules and evidence: docs/re/doors.md.
// No SDL dependency.
#include "assets/assets.hpp"
#include <array>
#include <cstdint>
#include <vector>

namespace chase::runtime {

enum class DoorPhase : std::uint16_t { Closed = 1, Opening = 2, Open = 3, Closing = 4 };

struct DoorSounds {
    std::uint16_t opening = 0, closing = 0, shut = 0, open = 0;  // ATTR_SFXOpening, ATTR_SFXClosing, ATTR_SFXShut, ATTR_SFXOpen (record +0x2c..+0x32)
};

struct DoorParams {
    std::uint8_t openFrames = 0;       // record +0xd: openclosetime * 30 cut to a byte
    std::uint8_t autoCloseFrames = 0;  // record +0xe: autoclosetime * 30 cut to a byte, 0 = stays open
    std::uint8_t soundOpenDelay = 0;   // record +0x10
    std::uint8_t bounce = 0;           // record +0x13
    bool autoOpen = false;             // record +0xf bit 1
    bool twoLeaves = false;            // record +0xf bit 0: the model has more than one node
    int doorType = 0;                  // record +0xf >> 3
    int lerpShape = 0;                 // record +0x3c
    float restAngle = 0;               // record +0x14 for the hinged types: -OpenAngle or +OpenAngle in radians
    float slide[3] = {0, 0, 0};        // record +0x14 for the sliding types
    float zoneCentre[3] = {0, 0, 0};   // entity +0x60
    float zoneHalf[3] = {0, 0, 0};     // entity +0x6c
    DoorSounds sounds;
};

struct DoorState {
    std::uint16_t state = static_cast<std::uint16_t>(DoorPhase::Closed);   // record +2
    std::uint16_t target = static_cast<std::uint16_t>(DoorPhase::Closed);  // record +4
    std::uint8_t timer = 0;                                                // record +0xc
    std::uint8_t bounceLeft = 0;                                           // record +0x11
    std::uint8_t bounceTimer = 0;                                          // record +0x12
    bool partial = false;                                                  // record +0xf bit 2
    float partialValue = 0;                                                // record +0x38
    bool openFlag = false;                                                 // entity flag 0x800
};

struct DoorSoundEvent {
    enum class Action { Play, Stop } action = Action::Play;
    std::uint16_t id = 0;
};

enum class DoorMessage { Reset, Open, Close, OpenAtOnce, SetOpening };

// Seconds to the byte frame counts of the record (0x163a2c: seconds times 30, cut to int and then to a byte).
std::uint8_t doorFrames(float seconds);

// Easing of the lerp shape (0x20201c / 0x221b48, table 0x17ea64). Shapes 0 to 3 and 5 are recovered; 4 and the custom curves return t.
float doorEase(float t, int shape);

// The door-local test of 0x1d16b8: the player position moved into the door's frame by the inverse of its world matrix, compared with
// the zone box (entity box, half extents widened by 0.25 in y and 4.0 in z). `worldMatrix` holds 16 floats, rows are the axes.
bool doorZoneContains(const float worldMatrix[16], const float player[3], const DoorParams& params);

// One game update of a door whose player is (or is not) in its zone: the update 0x1d16b8 while the state equals the target, then the
// state change 0x1a6b6c. Sound events are appended.
void doorUpdate(DoorState& door, const DoorParams& params, bool playerInZone, std::vector<DoorSoundEvent>& sounds);

// A request to the door (handler 0x1749b8). `value` is used by SetOpening (message 0x29).
void doorMessage(DoorState& door, const DoorParams& params, DoorMessage message, float value = 0);

// Pose matrices (16 floats, row vectors, translation last) the door gives its first and second node (0x26905c, through 0x22cbbc).
// `pivot` holds the float3 at node record +4 of the two nodes. Hinged types 0 and 1 turn about the pivot, the sliding types 5 to 8 and
// 0x11 move; the other types keep the identity (not recovered).
struct DoorPose {
    std::array<float, 16> node[2];
};
DoorPose doorPose(const DoorState& door, const DoorParams& params, const float pivot[2][3]);

// Parameters of a door object from its class file and instance (0x169494, 0x163a2c). `model` supplies the node count, the pivots and the
// sliding distance; `zoneCentre` / `zoneHalf` are the entity box (ObjectCollision).
DoorParams doorParams(const assets::LevelObject& object, const assets::Level& cls, const assets::Model& model, const float zoneCentre[3],
                      const float zoneHalf[3]);

}
