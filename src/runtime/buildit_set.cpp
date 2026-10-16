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

std::uint16_t soundParam(const assets::LevelObject& object, const assets::Level& cls, const char* name) {
    const assets::LevelParam* param = effectiveParam(object, cls, assets::nameHash(name));
    return param ? std::uint16_t(param->value) : std::uint16_t(0);
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
    references_.clear();
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
        build.soundBitDone = soundParam(object, *cls, "ATTR_SFXBitDone");
        build.soundBuilt = soundParam(object, *cls, "ATTR_SFXBuilt");
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
        build.wobble = BuilditWobble(static_cast<std::size_t>(bricks), [this] { return gameRandomUnit(random_); });
        references_.add(build.instance, build.triggerObject);
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

void BuilditSet::updateWobble(BuilditObject& object, const float player[3]) {
    const BuilditState& build = object.build;
    const bool checkRuns = build.state == build.target;
    if (!checkRuns || (build.state != static_cast<std::uint16_t>(BuilditPhase::Ready) && build.state != static_cast<std::uint16_t>(BuilditPhase::Building))) return;
    std::vector<bool> mayHop(build.progress.size());
    for (std::size_t i = 0; i < mayHop.size(); ++i) {
        const bool active = build.state == static_cast<std::uint16_t>(BuilditPhase::Building) && build.bit == static_cast<int>(i);
        mayHop[i] = !active && build.progress[i] != 1.0f;
    }
    object.wobble.update(inZone(object, player), mayHop, [this] { return gameRandomUnit(random_); });
}

std::vector<BuilditAction> BuilditSet::update(const assets::PlacedObjectState& state, const float player[3], bool actionHeld, bool playerCarrying) {
    std::vector<BuilditAction> actions;
    lastUse_ = BuilditUse{};
    for (std::size_t i = 0; i < objects_.size(); ++i) {
        BuilditObject& object = objects_[i];
        if (state.isDisabled(object.instance) && object.build.state == static_cast<std::uint16_t>(BuilditPhase::Ready)) continue;
        bool built = false;
        updateWobble(object, player);
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
        if (built || playerCarrying || !inZone(object, player)) continue;
        const bool takes = builditTakesAction(object.build) && actionHeld;
        const int reply = builditUse(object.build, actionHeld);
        if (!lastUse_.inZone) lastUse_ = BuilditUse{true, reply, takes, i};
    }
    return actions;
}

std::vector<SoundCall> builditSounds(const BuilditSet& set, const std::vector<BuilditAction>& actions, std::uint16_t ownerBase) {
    std::vector<SoundCall> calls;
    for (const BuilditAction& action : actions) {
        const BuilditObject& object = set.objects()[action.index];
        const std::uint16_t id = action.kind == BuilditActionKind::BrickDone ? object.soundBitDone : action.kind == BuilditActionKind::Built ? object.soundBuilt : 0;
        if (id == 0) continue;
        calls.push_back({id, {object.position[0], object.position[1], object.position[2]}, std::uint16_t(ownerBase + action.index)});
    }
    return calls;
}

void applyBuilditActions(BuilditSet& set, const std::vector<BuilditAction>& actions, assets::PlacedObjectState& state, ScriptRunner& scripts) {
    for (const BuilditAction& action : actions) {
        const BuilditObject& object = set.objects()[action.index];
        if (action.kind == BuilditActionKind::Reveal) {
            if (!object.finalProp.empty()) state.disabled.erase(object.finalProp);
            state.disabled.insert(object.instance);
        } else if (action.kind == BuilditActionKind::Built) {
            if (!object.finalProp.empty()) state.disabled.erase(object.finalProp);
            state.disabled.insert(object.instance);
            const ReferenceActivation activation = set.references().activate(action.index);
            if (activation.fired) scripts.triggerObjectEnabling(activation.target);
        }
    }
}

}
