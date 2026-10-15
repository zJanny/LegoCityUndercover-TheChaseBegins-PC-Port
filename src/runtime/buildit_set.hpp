#pragma once
// The build-its of a level (class props/prop_buildit): their use zone, parameters and state, and what a finished build does to the
// other objects. Rules and measurements: docs/re/buildit.md. No SDL dependency.
#include "assets/assets.hpp"
#include "assets/object_collision.hpp"
#include "runtime/buildit.hpp"
#include "runtime/buildit_wobble.hpp"
#include "runtime/game_random.hpp"
#include "runtime/reference_trigger.hpp"
#include "runtime/script_runner.hpp"
#include <string>
#include <vector>

namespace chase::runtime {

constexpr const char* kBuilditClass = "props/prop_buildit";

// Use zone (record +0x28): the box of the object's node bounds relative to the node, every half extent widened by 1.0 (*0x56d40c).
constexpr float kBuilditZoneMargin = 1.0f;

struct BuilditObject {
    std::string instance;       // lower case
    std::string finalProp;      // BuilditFinalProp, lower case, empty when the reference is unset
    std::string triggerObject;  // TriggerObject, lower case, empty when the reference is unset
    float studValue = kBuilditDefaultStudValue;
    float position[3] = {0, 0, 0};
    float zoneCentre[3] = {0, 0, 0};  // relative to position
    float zoneHalf[3] = {0, 0, 0};
    bool hasZone = false;
    BuilditState build;
    BuilditWobble wobble;  // LEVOBJ-S70: the start values come from the stand-in random source
};

constexpr std::uint32_t kBuilditWobbleStandInSeed = 0x5d2e91c7u;

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
    std::vector<BuilditAction> update(const assets::PlacedObjectState& state, const float player[3], bool actionHeld);
    const std::vector<BuilditObject>& objects() const { return objects_; }
    const BuilditObject* find(const std::string& lowerInstance) const;
    ReferenceTriggers& references() { return references_; }
    static bool inZone(const BuilditObject& object, const float player[3]);

private:
    void updateWobble(BuilditObject& object, const float player[3]);
    std::vector<BuilditObject> objects_;
    ReferenceTriggers references_;  // one per object, same index (TriggerObject)
    GameRandom random_{kBuilditWobbleStandInSeed};
};

// What the actions change: Reveal shows the final prop and disables the parts, Built runs the reference-trigger step 0x23211c on TriggerObject (reference_trigger.hpp).
void applyBuilditActions(BuilditSet& set, const std::vector<BuilditAction>& actions, assets::PlacedObjectState& state, ScriptRunner& scripts);

}
