#include "runtime/objectives.hpp"

#include <algorithm>
#include <cmath>

namespace chase::runtime {

namespace {

struct Entry {
    const char* name;
    ObjectiveNativeKind kind;
    unsigned arguments;
    bool standIn;
};

constexpr Entry kEntries[] = {
    {"setactivewaypoint", ObjectiveNativeKind::SetActiveWayPoint, 2, true},
    {"subobjectivegotoenable", ObjectiveNativeKind::SubObjectiveGoToEnable, 3, true},
    {"subobjectiveenemyenable", ObjectiveNativeKind::SubObjectiveEnemyEnable, 3, false},
    {"subobjectivearrowgotoenable", ObjectiveNativeKind::SubObjectiveArrowGoToEnable, 2, true},
    {"subobjectivearrowenemyenable", ObjectiveNativeKind::SubObjectiveArrowEnemyEnable, 2, true},
    {"enableminimapwipe", ObjectiveNativeKind::EnableMinimapWipe, 1, true},
    {"enablescanner", ObjectiveNativeKind::EnableScanner, 1, true},
    {"overridescannable", ObjectiveNativeKind::OverrideScannable, 2, true},
    {"panmaptoobject", ObjectiveNativeKind::PanMapToObject, 4, true},
    {"setmapdummylocobj", ObjectiveNativeKind::SetMapDummyLocObj, 1, true},
    {"clearmapdummylocobj", ObjectiveNativeKind::ClearMapDummyLocObj, 0, true},
    {"enablescannerbutton", ObjectiveNativeKind::EnableScannerButton, 1, true},
    {"blackoutmap", ObjectiveNativeKind::BlackoutMap, 1, true},
    {"removescannermapmarker", ObjectiveNativeKind::RemoveScannerMapMarker, 0, true},
    {"disableicon", ObjectiveNativeKind::DisableIcon, 2, true},
};

const Entry& entryOf(ObjectiveNativeKind kind) {
    for (const Entry& entry : kEntries)
        if (entry.kind == kind) return entry;
    return kEntries[0];
}

}

std::optional<ObjectiveNativeKind> objectiveNativeKind(const std::string& lowerCaseName) {
    for (const Entry& entry : kEntries)
        if (lowerCaseName == entry.name) return entry.kind;
    return std::nullopt;
}

std::vector<std::string> objectiveNativeNames() {
    std::vector<std::string> names;
    for (const Entry& entry : kEntries) names.push_back(entry.name);
    return names;
}

unsigned objectiveNativeArguments(ObjectiveNativeKind kind) { return entryOf(kind).arguments; }

bool objectiveNativeIsStandIn(ObjectiveNativeKind kind) { return entryOf(kind).standIn; }

void ObjectiveState::setWaypointActive(const std::string& waypoint, bool active) {
    const auto found = std::find(active_.begin(), active_.end(), waypoint);
    if (active) {
        if (found == active_.end()) active_.push_back(waypoint);
    } else if (found != active_.end()) {
        active_.erase(found);
    }
}

bool ObjectiveState::waypointActive(const std::string& waypoint) const {
    return std::find(active_.begin(), active_.end(), waypoint) != active_.end();
}

void ObjectiveState::setSubObjective(const std::string& waypoint, const std::string& target, std::uint8_t type, bool enable) {
    const auto record = std::find_if(groups_.begin(), groups_.end(),
                                     [&](const SubObjectiveGroup& group) { return group.waypoint == waypoint && group.type == type; });
    if (enable) {
        SubObjectiveGroup* group;
        if (record == groups_.end()) {
            groups_.push_back({waypoint, type, {}});
            group = &groups_.back();
        } else {
            group = &*record;
        }
        if (std::find(group->targets.begin(), group->targets.end(), target) != group->targets.end()) return;
        if (group->targets.size() < kSubObjectiveTargetLimit) group->targets.push_back(target);
        return;
    }
    if (record == groups_.end()) return;
    auto& targets = record->targets;
    const auto found = std::find(targets.begin(), targets.end(), target);
    if (found == targets.end()) return;
    *found = targets.back();
    targets.pop_back();
}

const SubObjectiveGroup* ObjectiveState::group(const std::string& waypoint, std::uint8_t type) const {
    for (const SubObjectiveGroup& group : groups_)
        if (group.waypoint == waypoint && group.type == type) return &group;
    return nullptr;
}

void ObjectiveState::record(ObjectiveNativeKind kind, const std::vector<float>& values, const std::string& firstObject) {
    ++recorded_[static_cast<std::size_t>(kind)];
    switch (kind) {
    case ObjectiveNativeKind::EnableMinimapWipe: minimapWipe_ = !values.empty() && values[0] != 0.0f; break;
    case ObjectiveNativeKind::EnableScanner: scannerFlag_ = !values.empty() && values[0] != 0.0f; break;
    case ObjectiveNativeKind::PanMapToObject: panMapObject_ = firstObject; break;
    case ObjectiveNativeKind::SetMapDummyLocObj: mapDummyObject_ = firstObject; break;
    case ObjectiveNativeKind::ClearMapDummyLocObj: mapDummyObject_.clear(); break;
    default: break;
    }
}

}
