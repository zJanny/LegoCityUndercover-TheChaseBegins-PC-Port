#pragma once
// Carrying (object types 122 and 123: the carry objects props/prop_carry and the carry targets props/prop_carrytarget*): the pickup, the
// carried state, the drop and the placing into a target, and the script objects they fire. Rules and measurements: docs/re/carrying.md.
// No SDL dependency.
#include "assets/assets.hpp"
#include "assets/object_collision.hpp"
#include "runtime/script_runner.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr int kCarryObjectType = 122;
constexpr int kCarryTargetType = 123;

// The use zone of a carry object (0x23e130 without a class "UseBound"): the box of the model bounds relative to the node, widened by
// (1, 1.2, 1) (the vector *0x56d40c = (1, 1, 1) plus 0.2 on the second component). Measured in m11-8-donuts-in-hand: half extents
// (1.7607, 1.6064, 1.3828) for the donut box, whose model half extents are (0.762, 0.4035, 0.3813).
constexpr float kCarryZoneMargin[3] = {1.0f, 1.2f, 1.0f};

// Updates per second the original multiplies ResetTime by (0x23ca4c); ResetTime 5 gave 150 in the record of the donut box.
constexpr unsigned kCarryUpdatesPerSecond = 30;
constexpr float kCarryDefaultResetSeconds = 5.0f;

// STAND-IN LEVOBJ-S31: the original ends each of these on events of its animations (docs/re/carrying.md). The place values are measured in
// the oracle (state 0xd6 entered at update 0; release 8 updates later, back to state 1 at update 17); the drop release is 4 updates (state
// 0xa8); the pickup values are copies of the place values.
constexpr unsigned kCarryPickupAttachUpdates = 8;
constexpr unsigned kCarryPickupEndUpdates = 17;
constexpr unsigned kCarryPlaceReleaseUpdates = 8;
constexpr unsigned kCarryPlaceEndUpdates = 17;
constexpr unsigned kCarryDropReleaseUpdates = 4;
constexpr unsigned kCarryDropEndUpdates = 17;

// STAND-IN LEVOBJ-S32: the original hangs the object on the player's hand bone through its "carryhandle" node (0x28db30); until the skeleton
// attachment exists a carried object floats in front of the player at this offset.
constexpr float kCarryHeldForward = 0.5f;
constexpr float kCarryHeldHeight = 0.9f;

struct CarryObject {
    std::string instance;       // lower case
    std::string triggerPickup;  // TriggerPickup, lower case, empty when unset
    std::string triggerOnRespawn;  // TriggerOnRespawn (record +0x14), lower case, empty when unset
    float home[3] = {0, 0, 0};  // initialposition
    float position[3] = {0, 0, 0};
    float zoneCentre[3] = {0, 0, 0};  // relative to the position
    float zoneHalf[3] = {0, 0, 0};
    bool hasZone = false;
    bool small = true;         // ATTR_SmallObject, default 1 (record +0x9e bit 3); the carry test 0x225754 gives 2 for a big object
    bool strengthOnly = false;  // ATTR_StrengthOnly (record +0x9e bit 2)
    unsigned resetUpdates = 0;  // ResetTime * 30 (record +0x9a)
    bool carried = false;
    unsigned resetCountdown = 0;  // record +0x98: counts down while the object lies where it was dropped
};

struct CarryTarget {
    std::string instance;       // lower case
    std::string useObject;      // UseObject, lower case: the only carry object this target takes
    std::string triggerObject;  // TriggerObject, lower case
    float position[3] = {0, 0, 0};
    float zoneCentre[3] = {0, 0, 0};  // UseBound of the class file, relative to the position
    float zoneHalf[3] = {0, 0, 0};
    bool hasZone = false;
    unsigned sfx = 0;  // ATTR_SFX_DROP
    bool used = false;
    bool usePending = false;  // the release asked for state 1; the effect runs at the next update (0x25e354)
};

enum class CarryPhase { Free, PickingUp, Placing, Dropping };

enum class CarryActionKind {
    PickedUp,   // the object attached to the player: TriggerPickup is fired
    Placed,     // the object was released into a target
    Dropped,    // the object was released at the player
    Used,       // the target changed to state 1: UseObject is disabled, TriggerObject is fired, the drop sound plays
    Respawned,  // a dropped object returned to its start position
};

struct CarryAction {
    CarryActionKind kind = CarryActionKind::PickedUp;
    std::size_t object = 0;
    std::size_t target = 0;
};

struct CarryInput {
    float player[3] = {0, 0, 0};
    float facing[3] = {0, 0, 1};  // unit vector the player looks along
    bool actionPressed = false;   // the action button went down in this update
    bool playerFree = true;       // the player is in a state that takes the action (idle, walk, run)
};

class CarrySet {
public:
    void load(assets::FuseArchive& archive, const assets::Level& level, const assets::ClassLookup& classOf);
    void add(const CarryObject& object) { objects_.push_back(object); }
    void add(const CarryTarget& target) { targets_.push_back(target); }
    // One game update: the pending target effects, the phase of the player (timers of the animation events), then the action button.
    std::vector<CarryAction> update(const assets::PlacedObjectState& state, const CarryInput& input);

    const std::vector<CarryObject>& objects() const { return objects_; }
    const std::vector<CarryTarget>& targets() const { return targets_; }
    const CarryObject* find(const std::string& lowerInstance) const;
    // World position of a carry object (carried objects follow the player); false for another instance.
    bool position(const std::string& lowerInstance, float out[3]) const;
    CarryPhase phase() const { return phase_; }
    // The player holds a carry object (ctrl+0x13c): 0 none, 2 a big object in the hands, 1 a small one (0x225754).
    int carryingMode() const;
    bool busy() const { return phase_ != CarryPhase::Free; }
    int carriedIndex() const { return carried_; }
    static bool inZone(const float zoneCentre[3], const float zoneHalf[3], const float origin[3], const float point[3]);

private:
    std::vector<CarryObject> objects_;
    std::vector<CarryTarget> targets_;
    CarryPhase phase_ = CarryPhase::Free;
    unsigned phaseUpdates_ = 0;
    int carried_ = -1;
    int placingTarget_ = -1;
    int pickingUp_ = -1;
    float player_[3] = {0, 0, 0};
    float facing_[3] = {0, 0, 1};
};

// What the actions change: Used disables the carry object (0x2428c0) and triggers the target's TriggerObject; PickedUp triggers the object's
// TriggerPickup and Respawned its TriggerOnRespawn, all three through 0x244f40 (ScriptRunner::triggerObjectEnabling).
void applyCarryActions(const CarrySet& set, const std::vector<CarryAction>& actions, assets::PlacedObjectState& state, ScriptRunner& scripts);

}
