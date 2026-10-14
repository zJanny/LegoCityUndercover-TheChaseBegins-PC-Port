#include "runtime/buildit_set.hpp"
#include "runtime/level_start.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <map>

namespace chase::runtime {
namespace {

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

std::string referenceName(const assets::LevelObject& object, const assets::Level& cls, std::uint32_t hash, const std::map<std::uint32_t, std::string>& names) {
    const assets::LevelParam* param = effectiveParam(object, cls, hash);
    if (!param) return {};
    const auto found = names.find(assets::objectReferenceHash(*param));
    return found == names.end() ? std::string() : found->second;
}

int brickCount(assets::FuseArchive& archive, const std::string& modelPath) {
    const assets::FuseEntry* entry = archive.find(modelPath);
    if (!entry) return 0;
    const assets::Model model = assets::parseModel(archive.read(*entry));
    return static_cast<int>(std::count_if(model.nodeMesh.begin(), model.nodeMesh.end(), [](int mesh) { return mesh >= 0; }));
}

}

void BuilditSet::load(assets::FuseArchive& archive, const assets::Level& level, const assets::ClassLookup& classOf) {
    objects_.clear();
    std::map<std::uint32_t, std::string> names;
    for (const assets::LevelObject& object : level.objects) names[assets::nameHash(object.instance)] = lower(object.instance);
    const std::uint32_t finalProp = assets::nameHash("BuilditFinalProp"), triggerObject = assets::nameHash("TriggerObject"),
                        studValue = assets::nameHash("StudValue");
    for (const assets::LevelObject& object : level.objects) {
        if (object.className != kBuilditClass) continue;
        const assets::Level* cls = classOf(object.className);
        const std::optional<assets::Placement> placed = assets::objectStart(level, object.instance);
        if (!cls || !placed) continue;
        BuilditObject build;
        build.instance = lower(object.instance);
        build.finalProp = referenceName(object, *cls, finalProp, names);
        build.triggerObject = referenceName(object, *cls, triggerObject, names);
        if (const assets::LevelParam* param = effectiveParam(object, *cls, studValue); param && param->type == 0) {
            std::memcpy(&build.studValue, &param->value, 4);
        }
        std::copy_n(placed->position, 3, build.position);
        int bricks = 0;
        if (!placed->mesh.empty()) {
            const std::string path = assets::meshModelPath(placed->mesh);
            float centre[3], half[3];
            if (assets::modelBounds(archive, path, centre, half)) {
                build.hasZone = true;
                for (int a = 0; a < 3; ++a) {
                    build.zoneCentre[a] = centre[a];
                    build.zoneHalf[a] = half[a] + kBuilditZoneMargin;
                }
            }
            bricks = brickCount(archive, path);
        }
        build.build = BuilditState(bricks);
        objects_.push_back(std::move(build));
    }
}

const BuilditObject* BuilditSet::find(const std::string& lowerInstance) const {
    for (const BuilditObject& object : objects_)
        if (object.instance == lowerInstance) return &object;
    return nullptr;
}

bool BuilditSet::inZone(const BuilditObject& object, const float player[3]) {
    if (!object.hasZone) return false;
    for (int a = 0; a < 3; ++a)
        if (!(std::fabs(player[a] - object.position[a] - object.zoneCentre[a]) < object.zoneHalf[a])) return false;
    return true;
}

std::vector<BuilditAction> BuilditSet::update(const assets::PlacedObjectState& state, const float player[3], bool actionHeld) {
    std::vector<BuilditAction> actions;
    for (std::size_t i = 0; i < objects_.size(); ++i) {
        BuilditObject& object = objects_[i];
        if (state.isDisabled(object.instance) && object.build.state == static_cast<std::uint16_t>(BuilditPhase::Ready)) continue;
        bool built = false;
        for (const BuilditEvent event : builditUpdate(object.build)) {
            built = built || event == BuilditEvent::Built;
            BuilditAction action;
            action.index = i;
            switch (event) {
            case BuilditEvent::BrickStarted: action.kind = BuilditActionKind::BrickStarted; break;
            case BuilditEvent::BrickDone: action.kind = BuilditActionKind::BrickDone; break;
            case BuilditEvent::FinalPropShown: action.kind = BuilditActionKind::Reveal; break;
            case BuilditEvent::Built: action.kind = BuilditActionKind::Built; break;
            }
            actions.push_back(action);
        }
        if (!built && inZone(object, player)) builditUse(object.build, actionHeld);
    }
    return actions;
}

void applyBuilditActions(const BuilditSet& set, const std::vector<BuilditAction>& actions, assets::PlacedObjectState& state, ScriptRunner& scripts) {
    for (const BuilditAction& action : actions) {
        const BuilditObject& object = set.objects()[action.index];
        if (action.kind == BuilditActionKind::Reveal) {
            if (!object.finalProp.empty()) state.disabled.erase(object.finalProp);
            state.disabled.insert(object.instance);
        } else if (action.kind == BuilditActionKind::Built) {
            if (!object.finalProp.empty()) state.disabled.erase(object.finalProp);
            state.disabled.insert(object.instance);
            if (!object.triggerObject.empty()) scripts.fireObjectTriggered(object.triggerObject);
        }
    }
}

}
