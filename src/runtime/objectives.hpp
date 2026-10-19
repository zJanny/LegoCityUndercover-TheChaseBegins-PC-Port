#pragma once
// Objective natives of the level scripts (batch SCRIPT-31, docs/re/objective-hud.md): SetActiveWayPoint 0x27bde8 turns a
// waypoint object (class props/map_*waypoint*) on and off, SubObjectiveGoToEnable 0x2adaac adds a target to or removes
// it from the group of a waypoint (registry 0x1fce14 / 0x2a62b8). The map and scanner natives only record their call.
// No SDL dependency.
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

enum class ObjectiveNativeKind {
    SetActiveWayPoint,
    SubObjectiveGoToEnable,
    SubObjectiveEnemyEnable,
    SubObjectiveArrowGoToEnable,
    SubObjectiveArrowEnemyEnable,
    EnableMinimapWipe,
    EnableScanner,
    OverrideScannable,
    PanMapToObject,
    SetMapDummyLocObj,
    ClearMapDummyLocObj,
    EnableScannerButton,
    BlackoutMap,
    RemoveScannerMapMarker,
    DisableIcon,
};
constexpr std::size_t kObjectiveNativeCount = 15;

std::optional<ObjectiveNativeKind> objectiveNativeKind(const std::string& lowerCaseName);
std::vector<std::string> objectiveNativeNames();
// Leading script arguments of each native, an object argument counts as one.
unsigned objectiveNativeArguments(ObjectiveNativeKind kind);
// SubObjectiveEnemyEnable 0x2b4c3c is `mov r0, #1; bx lr`; SubObjectiveGoToEnable (0x2a62b8 with 0x1fce14 / 0x1fcc68) and the
// two arrow natives (0x2c289c, 0x230dbc with 0x1f5fa8) are transcribed. SetActiveWayPoint (SCRIPT-S269) and the map and
// scanner natives (SCRIPT-S27) are STAND-INS.
bool objectiveNativeIsStandIn(ObjectiveNativeKind kind);

// Registry record 0xc4 bytes at *0x51a3a8 + 0x498 + 0xc4 i: key (+0xbc: the waypoint entity), type byte (+0xc0, 0 go to,
// 1 enemy, 2 carry), up to 32 target entities (+0 ..., count at +0xa0), one byte per target slot at +0x80 (set to "enabled"
// when the target is added by 0x1fce14, set again by the update once it is seen enabled; the bytes stay with their slot when a
// target is removed), the centroid (+0xa4), the map slot index (+0xb0, -1 none), its fade timer (+0xb4) and the byte "a target
// is on the map" (+0xb8). Here the entities are instance names (lower case).
constexpr std::size_t kSubObjectiveTargetLimit = 32;
struct SubObjectiveGroup {
    std::string waypoint;
    std::uint8_t type = 0;
    std::vector<std::string> targets;
    std::array<std::uint8_t, kSubObjectiveTargetLimit> seenEnabled{};
    std::array<float, 3> centroid{};
    bool hasSlot = false;
    float slotTimer = 0;
    bool targetOnMap = false;
};

// Arrow list entries of the sub-objective manager (*0x51a3a8): list 0 go to (SubObjectiveArrowGoToEnable 0x2c289c), 1 enemy
// (SubObjectiveArrowEnemyEnable 0x230dbc), 2 carry (0x22fb44); entities at +0x30c + 0x80 list, count at +0x48c + 4 list,
// state at +0xc + 0x100 list + 8 i: timer (0.5 when added), byte +4 "fading out", byte +5 "hide this update" (0x2b9c28).
constexpr std::size_t kObjectiveArrowLists = 3;
constexpr std::size_t kObjectiveArrowLimit = 32;
constexpr float kObjectiveArrowFadeSeconds = 0.5f;  // 0x2c293c and 0x230e5c, the timer of a new entry
struct ObjectiveArrow {
    std::string target;
    float timer = kObjectiveArrowFadeSeconds;
    bool fadingOut = false;
    bool hideRequest = false;
};

struct SubObjectiveWorld;

class ObjectiveState {
public:
    // 0x26e168: the active list of the objective natives. ScriptRunner passes the result of WaypointSet::setActive, which
    // ignores a disable while AutoTrigger is set (bit 4 of component +0x8e is AutoTrigger == 0).
    void setWaypointActive(const std::string& waypoint, bool active);
    bool waypointActive(const std::string& waypoint) const;
    // Instance names (lower case) in the order they were switched on.
    const std::vector<std::string>& activeWaypoints() const { return active_; }

    // 0x2a62b8: enable adds the target to the record of (waypoint, type) unless it is in the list (0x1fce14); disable
    // removes it by moving the last entry into its place and drops nothing else (a record that becomes empty is removed
    // by the update 0x2be934, not here).
    // `targetEnabled` is the entity flag test of 0x1fce14 (~(target + 0x10) & 1). A disable whose key has no record of the type
    // falls back to the first record of the type that lists the target (0x1fcc68 with key and target).
    void setSubObjective(const std::string& waypoint, const std::string& target, std::uint8_t type, bool enable, bool targetEnabled = true);
    const std::vector<SubObjectiveGroup>& groups() const { return groups_; }
    const SubObjectiveGroup* group(const std::string& waypoint, std::uint8_t type) const;

    // 0x2c289c / 0x230dbc / 0x22fb44: enable appends the target unless it is in the list (timer 0.5, not fading out); disable
    // removes it by moving the last entry and its state into its place (0x1f5fa8).
    void setArrow(std::size_t list, const std::string& target, bool enable);
    const std::array<std::vector<ObjectiveArrow>, kObjectiveArrowLists>& arrows() const { return arrows_; }
    // 0x2b9c28: the arrows of `target` in every list are hidden in the next update (the use hint 0x1cba8c of kinds 122, 123, 159).
    void requestArrowHide(const std::string& target);

    // 0x2ed8d8 per update: the arrow update, then the record update 0x2be934 (sub_objectives.cpp).
    void update(float dt, const SubObjectiveWorld& world);

    // STAND-IN: natives without a drawn effect only store their call count and last values.
    void record(ObjectiveNativeKind kind, const std::vector<float>& values, const std::string& firstObject);
    unsigned recordedCalls(ObjectiveNativeKind kind) const { return recorded_[static_cast<std::size_t>(kind)]; }
    bool minimapWipe() const { return minimapWipe_; }
    bool scannerFlag() const { return scannerFlag_; }
    // BlackoutMap 0x1b23f4 -> 0x19464c: the argument word != 0 in the byte 0x519e24 + 0x484, which raises the panel alpha (dialog_bg_fade.hpp).
    // STAND-IN HUD-S77: its second reader, the map alpha +0x4a0 direction at 0x2de3b0, is not ported (HUD-83).
    bool mapBlackout() const { return mapBlackout_; }
    const std::string& panMapObject() const { return panMapObject_; }
    const std::string& mapDummyObject() const { return mapDummyObject_; }

private:
    std::vector<std::string> active_;
    std::vector<SubObjectiveGroup> groups_;
    std::array<std::vector<ObjectiveArrow>, kObjectiveArrowLists> arrows_;
    unsigned recorded_[kObjectiveNativeCount] = {};
    bool minimapWipe_ = false;
    bool mapBlackout_ = false;
    bool scannerFlag_ = false;
    std::string panMapObject_, mapDummyObject_;
};

}
