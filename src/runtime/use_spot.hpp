#pragma once
// The use spot (props/prop_simpleuse, docs/re/use-spot.md): a use zone that fires its TriggerObject once when the player presses the action
// button inside it. Descriptor with create 0x1863fc, parameter reader 0x179470 and message handler 0x1922e8. No SDL dependency.
#include "assets/assets.hpp"
#include "assets/object_collision.hpp"

#include <array>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr const char* kUseSpotClass = "props/prop_simpleuse";
constexpr unsigned kUseSpotKind = 165;  // entity kind of the class (list 0x374640)

// The zone is the class bound "UseBound" of the class file (0x23e130 looks it up with 0x23e798; without it the zone would come from the node
// bounds, which the class does not need: props/prop_simpleuse has the bound). A class without the bound makes no spot.

struct UseSpot {
    std::string instance;  // lower case
    std::size_t objectIndex = 0;
    float position[3] = {0, 0, 0};
    std::array<float, 16> matrix{};  // node rows right, up, forward, translation (assets::placementMatrix)
    float zoneCentre[3] = {0, 0, 0};
    float zoneHalf[3] = {0, 0, 0};
    std::string triggerObject;      // record +0x18
    bool sendPlayerObject = false;  // record +0x1c
    bool active = true;             // zone record bit 4 (UseStartActive, 0x23e130; cleared by 0x236624 after a use)
};

// What a use does: 0x244f40 on the referenced object (enable, message 0xff, ObjectTriggered). The instigator is the spot (SendPlayerObject 0) or
// the register 0x1922e8 leaves for the second argument (SendPlayerObject 1, assumed to be the player); the port does not model it (SCRIPT-190).
struct UseSpotAction {
    std::string spot;
    std::string target;
    bool instigatorIsPlayer = false;
};

class UseSpotSet {
public:
    void load(const assets::Level& level, const assets::ClassLookup& classOf);
    // The action press of one update (0x1fee94 walks the registry and sends message 3 with the action flag; the first handler that returns 1 takes
    // the press). An active, enabled spot with a TriggerObject whose zone contains the player takes it and deactivates its zone (0x236624).
    std::vector<UseSpotAction> update(const assets::PlacedObjectState& state, const float player[3], bool actionPressed, bool playerFree);
    // Message 8 (0x1922e8): the zone is active again.
    void reactivate(const std::string& lowerInstance);
    const std::vector<UseSpot>& spots() const { return spots_; }
    static bool inZone(const UseSpot& spot, const float player[3]);

private:
    std::vector<UseSpot> spots_;
};

}
