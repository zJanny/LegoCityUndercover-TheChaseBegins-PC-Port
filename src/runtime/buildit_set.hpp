#pragma once
// The build-its of a level (class props/prop_buildit): their use zone, parameters and state, and what a finished build does to the
// other objects. Rules and measurements: docs/re/buildit.md. No SDL dependency.
#include "assets/assets.hpp"
#include "assets/object_collision.hpp"
#include "runtime/buildit.hpp"
#include "runtime/buildit_wobble.hpp"
#include "runtime/game_random.hpp"
#include "runtime/reference_trigger.hpp"
#include "runtime/sound_events.hpp"
#include "runtime/script_runner.hpp"
#include <string>
#include <vector>

namespace chase::runtime {

// STAND-IN LEVOBJ-S74: the sound stop key of a build-it (the u16 at entity +0x14 of the original) is this plus the object index.
constexpr std::uint16_t kBuilditSoundOwnerBase = 0x4000;

constexpr const char* kBuilditClass = "props/prop_buildit";

// Use zone (record +0x28): the box of the object's node bounds relative to the node, every half extent widened by 1.0 (*0x56d40c).
constexpr float kBuilditZoneMargin = 1.0f;

struct BuilditObject {
    std::string instance;       // lower case
    std::string finalProp;      // BuilditFinalProp, lower case, empty when the reference is unset
    std::string triggerObject;  // TriggerObject, lower case, empty when the reference is unset
    float studValue = kBuilditDefaultStudValue;
    std::uint16_t soundBitDone = 0;  // ATTR_SFXBitDone (record +0x88), played by the change to state 9
    std::uint16_t soundBuilt = 0;    // ATTR_SFXBuilt (record +0x8a), played by the change to state 11
    float position[3] = {0, 0, 0};
    float zoneCentre[3] = {0, 0, 0};  // relative to position
    float zoneHalf[3] = {0, 0, 0};
    bool hasZone = false;
    BuilditState build;
    BuilditWobble wobble;  // LEVOBJ-S70: the start values come from the stand-in random source
};

constexpr std::uint32_t kBuilditWobbleStandInSeed = 0x5d2e91c7u;

// The use query (message 3, 0x176160) of the build-it whose zone holds the player, as the player's state 9 reads it.
struct BuilditUse {
    bool inZone = false;
    int reply = 0;           // builditUse: 1 usable, 2 not usable
    bool accepted = false;   // the action button was taken: a state 2, 3 or 4 got target 8
    std::size_t object = 0;
};

enum class BuilditActionKind {
    BrickStarted,
    BrickDone,
    Reveal,     // state 10 starts: the final prop is shown and the parts are hidden
    Built,      // state 11: the final prop is enabled, the parts are disabled, TriggerObject is activated
};

struct BuilditAction {
    BuilditActionKind kind = BuilditActionKind::BrickStarted;
    std::size_t index = 0;
};

class BuilditSet {
public:
    void load(assets::FuseArchive& archive, const assets::Level& level, const assets::ClassLookup& classOf);
    // One game update: the object update of every enabled build-it, then the use message of the zone the player is in
    // (`actionHeld` is the action button). A disabled build-it does not run while it waits (state 2); one that is building keeps running
    // until it is done, because the original disables the parts in the change to state 11.
    // `playerCarrying`: the query is refused while the player holds an object (player record +0x13c, 0x1762d8).
    std::vector<BuilditAction> update(const assets::PlacedObjectState& state, const float player[3], bool actionHeld, bool playerCarrying);
    const BuilditUse& lastUse() const { return lastUse_; }
    const std::vector<BuilditObject>& objects() const { return objects_; }
    const BuilditObject* find(const std::string& lowerInstance) const;
    ReferenceTriggers& references() { return references_; }
    static bool inZone(const BuilditObject& object, const float player[3]);

private:
    void updateWobble(BuilditObject& object, const float player[3]);
    std::vector<BuilditObject> objects_;
    BuilditUse lastUse_;
    ReferenceTriggers references_;  // one per object, same index (TriggerObject)
    GameRandom random_{kBuilditWobbleStandInSeed};
};

// The sound requests of the actions (0x23ae84 with the sound id and the build-it's node): BrickDone plays ATTR_SFXBitDone, Built plays ATTR_SFXBuilt;
// an id of 0 plays nothing. `ownerBase` + the object index is the stop key of the calls (STAND-IN LEVOBJ-S74).
std::vector<SoundCall> builditSounds(const BuilditSet& set, const std::vector<BuilditAction>& actions, std::uint16_t ownerBase);

// What the actions change: Reveal shows the final prop and disables the parts, Built runs the reference-trigger step 0x23211c on TriggerObject (reference_trigger.hpp).
void applyBuilditActions(BuilditSet& set, const std::vector<BuilditAction>& actions, assets::PlacedObjectState& state, ScriptRunner& scripts);

}
