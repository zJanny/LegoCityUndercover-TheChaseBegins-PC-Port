#include "runtime/sub_objectives.hpp"

#include <algorithm>
#include <cmath>

namespace chase::runtime {

namespace {

bool test(const std::function<bool(const std::string&)>& predicate, const std::string& name) { return predicate && predicate(name); }

void removeArrow(std::vector<ObjectiveArrow>& arrows, const std::string& target) {
    for (std::size_t i = 0; i < arrows.size(); ++i) {
        if (arrows[i].target != target) continue;
        arrows[i] = arrows.back();
        arrows.pop_back();
        return;
    }
}

void updateArrows(std::array<std::vector<ObjectiveArrow>, kObjectiveArrowLists>& lists, float dt, const SubObjectiveWorld& world) {
    for (std::vector<ObjectiveArrow>& arrows : lists) {
        for (std::size_t i = arrows.size(); i-- > 0;) {
            if (i >= arrows.size()) continue;
            ObjectiveArrow& arrow = arrows[i];
            if (test(world.hidden, arrow.target) || test(world.destroyed, arrow.target)) {
                removeArrow(arrows, std::string(arrow.target));
                continue;
            }
            bool hide = arrow.hideRequest;
            float marker[3];
            float markerAlpha = 0;
            if (world.useMarker && world.useMarker(arrow.target, marker, markerAlpha)) {
                const float dx = marker[0] - world.player[0], dy = marker[1] - world.player[1], dz = marker[2] - world.player[2];
                if (std::sqrt(dx * dx + dy * dy + dz * dz) <= kObjectiveArrowUseMarkerDistance) hide = true;
            }
            if (arrow.fadingOut != hide) {
                arrow.fadingOut = hide;
                arrow.timer = kObjectiveArrowFadeSeconds - arrow.timer;
            } else {
                const float next = arrow.timer - dt;
                arrow.timer = next < 0.0f ? 0.0f : next;
            }
            arrow.hideRequest = false;
        }
    }
}

void removeFromRecord(SubObjectiveGroup& record, const std::string& target) {
    for (std::size_t i = record.targets.size(); i-- > 0;) {
        if (record.targets[i] != target) continue;
        record.targets[i] = record.targets.back();
        record.targets.pop_back();
        return;
    }
}

SubObjectiveGroup* findRecord(std::vector<SubObjectiveGroup>& groups, const std::string& key, std::uint8_t type, const std::string& target) {
    for (SubObjectiveGroup& group : groups)
        if (group.waypoint == key && group.type == type) return &group;
    for (SubObjectiveGroup& group : groups)
        if (group.type == type && std::find(group.targets.begin(), group.targets.end(), target) != group.targets.end()) return &group;
    return nullptr;
}

void centroidPass(std::vector<SubObjectiveGroup>& groups, std::size_t index, const SubObjectiveWorld& world) {
    SubObjectiveGroup& record = groups[index];
    record.centroid = {0, 0, 0};
    record.targetOnMap = false;
    for (std::size_t j = record.targets.size(); j-- > 0;) {
        if (j >= record.targets.size()) continue;
        const std::string target = record.targets[j];
        if (record.seenEnabled[j] == 0) {
            if (!test(world.hidden, target)) record.seenEnabled[j] = 1;
        } else if (test(world.hidden, target) || test(world.destroyed, target)) {
            if (SubObjectiveGroup* owner = findRecord(groups, record.waypoint, record.type, target)) removeFromRecord(*owner, target);
        }
        float position[3];
        if (!world.position || !world.position(target, position)) continue;
        for (int axis = 0; axis < 3; ++axis) record.centroid[std::size_t(axis)] += position[axis];
        record.targetOnMap = (world.onMap && world.onMap(position)) || record.targetOnMap;
    }
    const std::size_t count = record.targets.size();
    if (count <= 1) return;
    float divisor = float(count);
    if (!test(world.keyIsManagerEntity, record.waypoint)) {
        float key[3];
        if (world.position && world.position(record.waypoint, key))
            for (int axis = 0; axis < 3; ++axis) record.centroid[std::size_t(axis)] += key[axis];
        divisor = float(count + 1);
    }
    for (float& value : record.centroid) value *= 1.0f / divisor;
}

void deleteRecordOfKey(std::vector<SubObjectiveGroup>& groups, const std::string& key) {
    for (std::size_t u = groups.size(); u-- > 0;) {
        if (groups[u].waypoint != key) continue;
        groups[u] = groups.back();
        groups.pop_back();
        return;
    }
}

void updateRecords(std::vector<SubObjectiveGroup>& groups, float dt, const SubObjectiveWorld& world) {
    for (std::size_t index = groups.size(); index-- > 0;) {
        if (index >= groups.size()) continue;
        bool empty = groups[index].targets.empty();
        for (int pass = 0; pass < 2 && !empty; ++pass) {
            centroidPass(groups, index, world);
            empty = groups[index].targets.empty();
        }
        if (empty) {
            deleteRecordOfKey(groups, std::string(groups[index].waypoint));
            continue;
        }
        SubObjectiveGroup& record = groups[index];
        if (!record.targetOnMap) {
            if (!record.hasSlot) {
                record.hasSlot = true;
                record.slotTimer = 0.0f;
            }
            const float next = record.slotTimer + dt;
            record.slotTimer = kSubObjectiveSlotFadeSeconds <= next ? kSubObjectiveSlotFadeSeconds : next;
        } else if (record.hasSlot) {
            record.slotTimer -= dt;
            if (!(record.slotTimer > 0.0f)) record.hasSlot = false;
        }
    }
}

}

void ObjectiveState::update(float dt, const SubObjectiveWorld& world) {
    updateArrows(arrows_, dt, world);
    updateRecords(groups_, dt, world);
}

int objectiveArrowAlphaByte(const ObjectiveArrow& arrow, int baseAlpha) {
    if (arrow.timer <= 0.0f) return arrow.fadingOut ? 0 : baseAlpha;
    float value = arrow.timer * 2.0f;
    if (!arrow.fadingOut) value = 1.0f - value;
    return int(value * 255.0f) & 0xff;
}

std::array<float, 3> objectiveArrowAnchor(const float node[3], float boxCentreY, float boxHalfY, float clockSeconds) {
    const float lift = boxCentreY + boxHalfY + kObjectiveArrowLift;
    return {node[0], node[1] + lift + std::sin(clockSeconds / kObjectiveArrowBobDivisor) * kObjectiveArrowBobAmount, node[2]};
}

std::array<float, 3> objectiveArrowUseMarkerAnchor(const float marker[3], float clockSeconds) {
    return {marker[0], marker[1] + kObjectiveArrowLift + std::sin(clockSeconds / kObjectiveArrowBobDivisor) * kObjectiveArrowBobAmount, marker[2]};
}

std::array<std::array<float, 3>, 4> objectiveArrowQuad(const std::array<float, 3>& anchor, const float right[3], const float up[3]) {
    constexpr float signs[4][2] = {{-1, 1}, {1, 1}, {1, -1}, {-1, -1}};
    std::array<std::array<float, 3>, 4> corners{};
    for (int corner = 0; corner < 4; ++corner)
        for (int axis = 0; axis < 3; ++axis)
            corners[std::size_t(corner)][std::size_t(axis)] = anchor[std::size_t(axis)] + signs[corner][0] * kObjectiveArrowHalfSize * right[axis] +
                                                             signs[corner][1] * kObjectiveArrowHalfSize * up[axis];
    return corners;
}

}
