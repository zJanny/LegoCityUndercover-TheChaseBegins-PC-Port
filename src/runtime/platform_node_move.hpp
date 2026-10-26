#pragma once
// The node move of the platform class (ObjectType 64; docs/re/props-rocking.md): the create 0x199adc and post-link 0x18b7bc fields, the
// trigger messages of the handler 0x1a8d74 with the request 0x2150bc, the travel of the update 0x2807c8 and the node hierarchy that
// AttachObject 0x1bf68c builds (0x245008), which carries attached objects with a moving node. No SDL dependency.
#include "assets/assets.hpp"

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

using PlatformVector = std::array<float, 3>;

constexpr std::uint16_t kPlatformStoppedState = 0;
constexpr std::uint16_t kPlatformMovingState = 2;
constexpr std::uint16_t kPlatformReturningState = 3;
constexpr std::uint16_t kPlatformControlledState = 4;
constexpr std::uint16_t kPlatformArrivedState = 5;
constexpr float kPlatformSpeedDefault = 0.01f;   // `speed` default of the create (literal 0x3c23d70a at 0x199e6c), stored at R+0x74
constexpr float kPlatformStepRate = 30.0f;       // literal 0x41f00000 at 0x280ae8
constexpr std::uint16_t kPlatformWaitLimit = 0x7fff;  // the bound of the continuous wait in updates (literal 0x7fff at 0x199f90)
constexpr std::size_t kPlatformAttachedMax = 5;  // message 43 (0x1a945c) keeps at most five objects at R+0x60

// Record fields of the create 0x199adc and the post-link 0x18b7bc that the travel reads.
struct PlatformMotion {
    std::uint8_t type = 0;                  // R+0xb4 `type`
    std::uint8_t orient = 0;                // R+0xb6 bits 10..11 `orient`
    bool rotateOnly = false;                // R+0xb6 bit 9 `rotateonly`
    bool looped = false;                    // R+0xb6 bit 8 `Looped`
    bool lerpSameBackwards = true;          // R+0xb6 bit 15 `LerpSameBackwards`
    bool stopOnUntrigger = false;           // R+0xb8 bit 4 `StopOnUntrigger`
    bool resetProgressOnTrigger = true;     // R+0xb8 bit 2 `ResetProgressOnTrigger`
    std::uint16_t lerpShape = 0;            // R+0x9c, 0x233e60 of `LerpShape`
    std::uint16_t continuousWait = 0;       // R+0xae, `continuuswaittime` x 30 (type 0)
    std::uint16_t continuousCount = 0;      // R+0xac
    bool path = false;                      // R+0x50: `Platform_Path` names a path of the level
    PlatformVector placement{};             // the node translation at the post-link
    PlatformVector origin{};                // R+0x18
    PlatformVector target{};                // R+0x24, `ObjectTo` (the platform itself when unset)
};

struct PlatformRecord {
    float value = 0;              // R+0x7c, read by 0x1de878
    std::uint16_t state = 0;      // R+2
    std::uint16_t requested = 0;  // R+4
    float speed = kPlatformSpeedDefault;  // R+0x74
    float previous = 0;           // R+0x80, the value of the last update
    PlatformMotion motion{};
};

// The create and the post-link for one kind 64 object: the parameters above, the initial state of its type (2 for type 0, 4 for
// `Controlled`, else 0) and the end points of the travel.
PlatformRecord platformRecordOf(const assets::LevelObject& object, const assets::Level& cls, const assets::Level& level);

// 0x2150bc(entity, backwards, progress): the requested state for a trigger (forwards) or an untrigger (backwards), then the
// value set to `progress` when ResetProgressOnTrigger is set. STAND-IN SCRIPT-S301: the type 7 rotation steps, the quaternion
// saved at R+0x30 and the call 0x23211c for an arrived lift are not ported.
void platformRequest(PlatformRecord& record, bool backwards, float progress);
// Messages 0xff (TriggerObject 1, 0x1a8e7c) and 0xfe (TriggerObject 0, 0x1a8edc) of the handler 0x1a8d74.
void platformMessage(PlatformRecord& record, std::uint8_t message);

// 0x20201c: the shape value, 1 - shape(1 - t) when moving back with LerpSameBackwards. Shape 0 returns t (0x221b48).
// STAND-IN SCRIPT-S301: the curves of shapes 1..5 (the table at 0x376ad8) and of named shapes (0x233e60) are not read; they return t.
float platformShape(const PlatformRecord& record, float value, bool backwards);

// One update of 0x2807c8 before the node move: the step dt x 30 x speed in state 2 (added) or 3 (subtracted), the end of the travel
// at 1.0 or 0.0 by type, and the node translation of the travel in states 2, 3 and 4 (the lerp 0x221e5c of the origin and the target
// with the shaped value). Returns the new node translation, or nothing when the node does not move this update. The requested state
// becomes the state at the end of the update (0x19a590). STAND-IN SCRIPT-S301: paths (`Platform_Path`), `orient` and type 7
// rotations, the timers of states 0, 1 and 5 (types 5 and 6, R+0xa2, R+0xb0, R+0xb6), sounds and the messages 41 to the
// controlled platforms are not ported; such a platform's node does not move.
std::optional<PlatformVector> platformUpdate(PlatformRecord& record, float dt);

// The node hierarchy of AttachObject (0x245008 keeps the child's world transform) and the translation of moved nodes: a node that
// moves takes its children along (their world matrices follow the parent's). STAND-IN SCRIPT-S302: only translations are carried
// (no ported platform rotates), and the order of the entity updates is the order of the level's objects.
class NodeCarry {
public:
    void clear();
    // `parent` empty: the child is at the world root or a room node again (AttachToWorldRoot, AttachToRoom).
    void setParent(const std::string& lowerChild, const std::string& lowerParent);
    // Message 43 to a platform (sent by AttachObject 0x1bf6d8): the child joins the list R+0x60 once, at most five entries.
    void attach(const std::string& lowerPlatform, const std::string& lowerChild);
    // Moves the node and every node below it by `delta`.
    void move(const std::string& lower, const PlatformVector& delta);
    // How far the node stands from its placement because moving nodes carried it (0 when nothing moved it).
    PlatformVector offset(const std::string& lower) const;
    const std::vector<std::string>& attached(const std::string& lowerPlatform) const;
    const std::string* parent(const std::string& lowerChild) const;
    // The objects whose offset changed since the last call, in the order they first moved.
    std::vector<std::string> takeMoved();

private:
    std::map<std::string, std::string> parents_;
    std::map<std::string, std::vector<std::string>> children_;
    std::map<std::string, std::vector<std::string>> attached_;
    std::map<std::string, PlatformVector> offsets_;
    std::vector<std::string> moved_;
};

}
