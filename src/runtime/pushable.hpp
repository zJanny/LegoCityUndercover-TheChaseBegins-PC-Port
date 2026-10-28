#pragma once
// Pushable objects (kind 71, leprops/leprop_pushable) with their push handles (kind 72, leprops/leprop_pushhandle nested in the class file),
// and the player's handle states 84 HANDLE IDLE, 85 HANDLE PUSH and 86 HANDLE PULL. Rules and addresses: docs/re/pushables.md. No SDL dependency.
#include "assets/assets.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr std::uint8_t kPushableKind = 71;
constexpr std::uint8_t kPushHandleKind = 72;
// Clips of the handle states' enter functions (0x21941c, tablesanimations sheet 1): 86 0x2959f4, 84 0x295974, 85 0x295a40.
constexpr int kAnimationPushPull = 7;
constexpr int kAnimationPushIdle = 8;
constexpr int kAnimationPush = 9;
constexpr std::uint16_t kPlayerStateHandleIdle = 84;
constexpr std::uint16_t kPlayerStateHandlePush = 85;
constexpr std::uint16_t kPlayerStateHandlePull = 86;
// Parameter defaults of the create 0x19a7c8 (literals of 0x241280), used when neither the instance nor the class sets them.
constexpr float kPushableTriggerDistanceDefault = 0.75f;
constexpr float kPushableSpeedDefault = 0.06f;
constexpr float kPushableSinkHeightDefault = 1.0f;
constexpr float kPushHandleRayScale = 1.1f;   // 0x19aa88: the handle ray starts at 1.1 x the bounds face
constexpr float kPushGripBlendRate = 5.0f;    // 0x289d28: the blend to the use point grows by dt x 5
constexpr unsigned kPushGripTurnPerSecond = 60000;  // 0x289d28: the turn step is 60000 / frame rate (0x23ca4c, 30) per update
constexpr std::uint8_t kPushableSinkUpdates = 60;    // 0x281434 state 1 ends after 60 updates (or at once with SinkHeight 0)

using PushMatrix = assets::Matrix;  // row vectors, translation in the last row

struct PushHandle {
    std::string instance;   // "pushable.handle", lower case
    PushMatrix local{};     // 0x19aa88: the handle's matrix in the pushable's frame
    bool active = true;     // ActiveHandles_Front, _Back, _Right, _Left: 0 disables the handle (0x2428c0)
    bool usable = true;     // use record +4 bit 4 (UseStartActive 1), cleared when the pushable reaches state 2 (0x19aecc)
    float useCentre[3] = {0, 0, 0}, useHalf[3] = {0, 0, 0};  // the handle class's UseBound (box)
};

struct PushableObject {
    std::string instance, destination, triggerObject;  // lower case; references empty when unset
    float triggerDistance = kPushableTriggerDistanceDefault;  // record +0x30
    float pushSpeed = kPushableSpeedDefault;                  // record +0x34
    float sinkHeight = kPushableSinkHeightDefault;            // record +0x3c
    bool strong = false, autoPush = false, breakOnFall = false, noGravity = false;  // record +0x2c bits 1, 0, 4, 6
    bool canPush = false, canPull = false;  // record +0x2c bits 2 and 3 from MoveType (0: push, 1: pull, 2: push and pull, 3: none)
    std::uint16_t soundPush = 0, soundLand = 0, soundDestroyed = 0;  // ATTR_SFX_PUSH, _LAND, _DESTROYED (record +0x24, +0x26, +0x28)
    PushMatrix node{};  // world matrix
    float boundsCentre[3] = {0, 0, 0}, boundsHalf[3] = {0, 0, 0};  // the model's node bounds (node +0xa0, +0xac)
    std::uint16_t state = 0, target = 0;  // record +2, +4
    std::uint8_t sinkUpdates = 0;         // record +0x2b
    bool pushed = false;                  // record +0x60: message 9 arrived before this update's move
    bool pushPending = false;             // message 9 of this update; the pushable acts on it in the next update (oracle: pusher set in one update, first move in the next)
    float direction[3] = {0, 0, 0};       // record +0x44
    bool pushSound = false;               // record +0x2c bit 5
    std::vector<PushHandle> handles;
};

enum class PushableEventKind {
    Moved,
    SoundStart,  // 0x215030(1): ATTR_SFX_PUSH at the pushable unless it plays
    SoundStop,   // 0x215030(0): the push sound fades out over 0.2 s
    Arrived,     // state 1: the pushable is within TriggerDistance of its Destination
    Done,        // state 2: TriggerObject is activated (0x244f40), the handles are no longer usable
};

struct PushableEvent {
    PushableEventKind kind = PushableEventKind::Moved;
    std::size_t index = 0;
};

class PushableSet {
public:
    void load(assets::FuseArchive& archive, const assets::Level& level, const assets::ClassLookup& classOf);
    void add(PushableObject object) { objects_.push_back(std::move(object)); }
    const std::vector<PushableObject>& objects() const { return objects_; }
    std::vector<PushableObject>& objects() { return objects_; }
    // Message 9 (0x1a9540): the pusher pushes in `direction` (a unit vector times the push speed in the next update).
    void push(std::size_t index, const float direction[3]);
    // One update of every enabled pushable (0x19aecc -> 0x281434). `destination` gives the position of an enabled destination object.
    std::vector<PushableEvent> update(const std::function<bool(const std::string&)>& disabled,
                                      const std::function<bool(const std::string&, float[3])>& destination);

private:
    std::vector<PushableObject> objects_;
};

// 0x19aa88 for one handle slot (0 front, 1 back, 2 right, 3 left) of a pushable with node bounds `centre` / `half`. STAND-IN SCRIPT-S303:
// the ray 0x21a728 against the pushable is not cast, so the handle stands at the bounds face (the original's value when the ray misses).
PushMatrix pushHandleLocal(int slot, const float centre[3], const float half[3]);
PushMatrix pushHandleWorld(const PushableObject& object, const PushHandle& handle);
// 0x221570 for a handle (FaceTowards 4): the player faces against the handle's z axis and stands inside its UseBound.
bool pushHandleZoneHolds(const PushableObject& object, const PushHandle& handle, const float player[3], std::int16_t facing);
// 0x2aff3c: the touched pushable's handle nearest to the player, when it is enabled, the pushable has AutoPush and its zone holds.
// STAND-IN SCRIPT-S304: the touch (DAT_00377134) is the overlap of the player's box with the pushable's bounds box; the use query of
// 0x210f30 (0x227b5c, the character's use test) is taken as passed.
std::optional<std::size_t> pushGrabTest(const PushableObject& object, const float player[3], std::int16_t facing);

struct PushGrip {
    bool active = false;
    std::size_t pushable = 0, handle = 0;  // player record +0x130
    float blend = 0;                       // +0x24c
    float start[3] = {0, 0, 0};            // +0x274
    std::uint16_t state = kPlayerStateHandleIdle;
};

struct PushGripInput {
    float position[3] = {0, 0, 0};
    std::int16_t facing = 0;
    bool stickActive = false;
    std::int16_t stickHeading = 0;
    bool actionHeld = false;  // STAND-IN SCRIPT-S304: player record +0xc bit 2, read as the action button
    float dt = 0;
    unsigned frameRate = 30;
};

struct PushGripStep {
    bool exited = false;  // state 1 requested: the player stays where he is in this update
    float position[3] = {0, 0, 0};
    std::int16_t facing = 0;
    int animation = kAnimationPushIdle;
    std::optional<std::uint16_t> enteredState;
};

void pushGripStart(PushGrip& grip, std::size_t pushable, std::size_t handle, const float position[3]);
// 0x289d28, the update of states 84 to 86: the zone and stick tests, message 9 once the blend is complete, the blend of x and z
// to the handle's use point and the turn to the handle.
PushGripStep pushGripUpdate(PushGrip& grip, PushableSet& set, const PushGripInput& input);

}
