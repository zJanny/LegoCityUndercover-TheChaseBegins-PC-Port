#pragma once
// Objective natives of the level scripts (batch SCRIPT-31, docs/re/objective-hud.md): SetActiveWayPoint 0x27bde8 turns a
// waypoint object (class props/map_*waypoint*) on and off, SubObjectiveGoToEnable 0x2adaac adds a target to or removes
// it from the group of a waypoint (registry 0x1fce14 / 0x2a62b8). The map and scanner natives only record their call.
// No SDL dependency.
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
// SubObjectiveEnemyEnable 0x2b4c3c is `mov r0, #1; bx lr` and SetActiveWayPoint and SubObjectiveGoToEnable are
// transcribed with their registry but skip a disable test and the group effect, so they and every other native are
// STAND-INS (SCRIPT-S26).
bool objectiveNativeIsStandIn(ObjectiveNativeKind kind);

// Registry record 0xc4 bytes at 0x51a3a8 + 0x498 + 0xc4 i: key (+0x5c: the waypoint entity), type byte (+0x60, 0 for
// SubObjectiveGoToEnable), up to 32 target entities (+0 ..., count at +0xa0). Here the entities are instance names.
constexpr std::size_t kSubObjectiveTargetLimit = 32;
struct SubObjectiveGroup {
    std::string waypoint;
    std::uint8_t type = 0;
    std::vector<std::string> targets;
};

class ObjectiveState {
public:
    // 0x26e168: enabling always applies. Disabling is skipped by the original when bit 4 of the byte at component +0x8e
    // is clear; its meaning is not recovered, so a disable always applies here (SCRIPT-S26).
    void setWaypointActive(const std::string& waypoint, bool active);
    bool waypointActive(const std::string& waypoint) const;
    // Instance names (lower case) in the order they were switched on.
    const std::vector<std::string>& activeWaypoints() const { return active_; }

    // 0x2a62b8: enable adds the target to the record of (waypoint, type) unless it is in the list (0x1fce14); disable
    // removes it by moving the last entry into its place and drops nothing else (a record that becomes empty is removed
    // by the update 0x2be934, not here).
    void setSubObjective(const std::string& waypoint, const std::string& target, std::uint8_t type, bool enable);
    const std::vector<SubObjectiveGroup>& groups() const { return groups_; }
    const SubObjectiveGroup* group(const std::string& waypoint, std::uint8_t type) const;

    // STAND-IN: natives without a drawn effect only store their call count and last values.
    void record(ObjectiveNativeKind kind, const std::vector<float>& values, const std::string& firstObject);
    unsigned recordedCalls(ObjectiveNativeKind kind) const { return recorded_[static_cast<std::size_t>(kind)]; }
    bool minimapWipe() const { return minimapWipe_; }
    bool scannerFlag() const { return scannerFlag_; }
    const std::string& panMapObject() const { return panMapObject_; }
    const std::string& mapDummyObject() const { return mapDummyObject_; }

private:
    std::vector<std::string> active_;
    std::vector<SubObjectiveGroup> groups_;
    unsigned recorded_[kObjectiveNativeCount] = {};
    bool minimapWipe_ = false;
    bool scannerFlag_ = false;
    std::string panMapObject_, mapDummyObject_;
};

}
