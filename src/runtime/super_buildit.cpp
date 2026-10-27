#include "runtime/super_buildit.hpp"
#include "assets/object_classes.hpp"
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

const assets::LevelParam* param(const assets::LevelObject& object, const assets::Level& cls, const char* name) {
    return effectiveParam(object, cls, assets::nameHash(name));
}

// 0x2451d0: a float parameter is truncated to an integer, types 5 and 7 give the raw word, anything else the default.
std::int32_t intParam(const assets::LevelObject& object, const assets::Level& cls, const char* name, std::int32_t fallback) {
    const assets::LevelParam* found = param(object, cls, name);
    if (!found) return fallback;
    if (found->type == 5 || found->type == 7) return std::int32_t(found->value);
    if (found->type == 0) {
        float value;
        std::memcpy(&value, &found->value, 4);
        return std::int32_t(value);
    }
    return fallback;
}

std::string stringParam(const assets::LevelObject& object, const assets::Level& cls, const char* name) {
    const assets::LevelParam* found = param(object, cls, name);
    if (!found || found->type != 1 || found->blocks.empty()) return {};
    const assets::Bytes& block = found->blocks[0];
    return std::string(block.begin(), std::find(block.begin(), block.end(), std::uint8_t(0)));
}

using Names = std::map<std::uint32_t, std::string>;

// A reference names its target by name hash: an object of the same class file first ("parent.child"), then a level object.
std::string referenceName(const assets::LevelObject& object, const assets::Level& cls, const char* name, const Names& siblings, const Names& levelNames) {
    const assets::LevelParam* found = param(object, cls, name);
    if (!found) return {};
    const std::uint32_t hash = assets::objectReferenceHash(*found);
    if (hash == 0) return {};
    if (const auto sibling = siblings.find(hash); sibling != siblings.end()) return sibling->second;
    if (const auto placed = levelNames.find(hash); placed != levelNames.end()) return placed->second;
    return {};
}

void setZone(assets::FuseArchive& archive, SuperBuilditObject& object) {
    if (object.mesh.empty()) return;
    float centre[3], half[3];
    if (!assets::modelBounds(archive, assets::meshModelPath(object.mesh), centre, half)) return;
    const std::array<float, 3>& margin = object.type == kSuperBuilditTypeNarrowZone ? kSuperBuilditZoneMarginType12 : kSuperBuilditZoneMargin;
    for (int a = 0; a < 3; ++a) {
        object.zoneCentre[a] = centre[a];
        object.zoneHalf[a] = half[a] + margin[std::size_t(a)];
    }
    object.hasZone = true;
}

}

void SuperBuilditSet::load(assets::FuseArchive& archive, const assets::Level& level, const assets::ClassLookup& classOf) {
    objects_.clear();
    prompt_.reset();
    owner_.reset();
    confirmOpen_ = false;
    dialogResult_ = kSuperBuilditDialogNone;
    const assets::Level* superClass = classOf(kSuperBuilditClass);
    if (!superClass) return;
    Names levelNames;
    for (const assets::LevelObject& object : level.objects) levelNames[assets::nameHash(object.instance)] = lower(object.instance);
    std::map<std::size_t, assets::Placement> placedByIndex;
    for (const assets::Placement& placed : assets::objectPlacements(level, classOf)) placedByIndex[placed.objectIndex] = placed;

    const auto fill = [&](SuperBuilditObject& out, const assets::LevelObject& object, const Names& siblings) {
        out.type = intParam(object, *superClass, "SuperBuildType", 0);
        const std::int32_t value = intParam(object, *superClass, "SuperBuildValue", 0);
        out.value = std::uint32_t(std::max(value, 0));
        out.textPopup = std::uint32_t(intParam(object, *superClass, "TextPopup", -1));
        out.fmv = stringParam(object, *superClass, "FMVToPlay");
        out.soundBuilt = std::uint16_t(intParam(object, *superClass, "ATTR_SFXBuilt", 0));
        out.playerFaceObject = intParam(object, *superClass, "PlayerFaceObject", 0) & 1;
        out.saveState = intParam(object, *superClass, "SaveState", 1) & 1;
        out.propToReveal = referenceName(object, *superClass, "PropToReveal", siblings, levelNames);
        out.useObject = referenceName(object, *superClass, "UseObject", siblings, levelNames);
        out.hideObject = referenceName(object, *superClass, "HideObject", siblings, levelNames);
        out.buildCompleteTrigger = referenceName(object, *superClass, "BuildCompleteTriggerObject", siblings, levelNames);
    };

    for (std::size_t i = 0; i < level.objects.size(); ++i) {
        const assets::LevelObject& object = level.objects[i];
        const assets::Level* cls = classOf(object.className);
        if (!cls) continue;
        const auto placed = placedByIndex.find(i);
        const std::optional<assets::Placement> start = assets::objectStart(level, object.instance);
        const assets::Placement holderPlacement = placed != placedByIndex.end() ? placed->second : start.value_or(assets::Placement{});
        const assets::Matrix holderMatrix = assets::placementMatrix(holderPlacement);
        if (object.className == kSuperBuilditClass) {
            SuperBuilditObject out;
            out.instance = lower(object.instance);
            fill(out, object, Names{});
            out.mesh = holderPlacement.mesh;
            out.world = holderMatrix;
            setZone(archive, out);
            objects_.push_back(std::move(out));
            continue;
        }
        Names siblings;
        for (const assets::LevelObject& child : cls->objects) siblings[assets::nameHash(child.instance)] = lower(object.instance) + "." + lower(child.instance);
        std::map<std::size_t, assets::Placement> childPlaced;
        for (const assets::Placement& child : assets::objectPlacements(*cls, classOf)) childPlaced[child.objectIndex] = child;
        const bool holder = assets::objectKind(object, *cls) == kSuperBuilditHolderKind;
        for (std::size_t c = 0; c < cls->objects.size(); ++c) {
            const assets::LevelObject& child = cls->objects[c];
            if (child.className != kSuperBuilditClass) continue;
            SuperBuilditObject out;
            out.parent = lower(object.instance);
            out.instance = out.parent + "." + lower(child.instance);
            fill(out, child, siblings);
            if (holder) {
                const std::int32_t holderValue = intParam(object, *cls, "SuperBuildValue", 0);
                if (holderValue > 0) out.value = std::uint32_t(holderValue);
                if (holderValue < 0) out.value = 0;
            }
            out.parentTrigger = referenceName(object, *cls, "TriggerObject", Names{}, levelNames);
            if (intParam(object, *cls, "StartBuilt", 0) != 0) out.requested = std::uint16_t(SuperBuilditPhase::Built);
            const auto local = childPlaced.find(c);
            out.mesh = local != childPlaced.end() ? local->second.mesh : std::string();
            out.world = local != childPlaced.end() ? assets::multiply(assets::placementMatrix(local->second), holderMatrix) : holderMatrix;
            setZone(archive, out);
            objects_.push_back(std::move(out));
        }
    }
}

const SuperBuilditObject* SuperBuilditSet::find(const std::string& lowerInstance) const {
    for (const SuperBuilditObject& object : objects_)
        if (object.instance == lowerInstance) return &object;
    return nullptr;
}

SuperBuilditObject* SuperBuilditSet::findMutable(const std::string& lowerInstance) {
    for (SuperBuilditObject& object : objects_)
        if (object.instance == lowerInstance) return &object;
    return nullptr;
}

bool SuperBuilditSet::inZone(const SuperBuilditObject& object, const float player[3]) {
    if (!object.hasZone) return false;
    float offset[3];
    for (int a = 0; a < 3; ++a) offset[a] = player[a] - object.world[std::size_t(12 + a)];
    for (int a = 0; a < 3; ++a) {
        const float* axis = object.world.data() + 4 * a;
        const float local = offset[0] * axis[0] + offset[1] * axis[1] + offset[2] * axis[2];
        if (!(std::fabs(local - object.zoneCentre[a]) < object.zoneHalf[a])) return false;
    }
    return true;
}

void SuperBuilditSet::answer(int result) {
    confirmOpen_ = false;
    dialogResult_ = result;
}

void SuperBuilditSet::stepState(std::size_t index, std::vector<SuperBuilditAction>& actions) {
    SuperBuilditObject& object = objects_[index];
    if (object.state == object.requested) return;
    switch (SuperBuilditPhase(object.requested)) {
    case SuperBuilditPhase::Built:
        actions.push_back({SuperBuilditActionKind::Built, index, object.state != std::uint16_t(SuperBuilditPhase::Created)});
        break;
    case SuperBuilditPhase::Confirm:
        if (dialogResult_ == kSuperBuilditDialogCancel) {
            object.requested = std::uint16_t(SuperBuilditPhase::Waiting);
        } else if (dialogResult_ == kSuperBuilditDialogConfirm) {
            actions.push_back({SuperBuilditActionKind::MovieStarted, index, false});
            object.built = true;
            object.zoneActive = false;
            object.requested = std::uint16_t(SuperBuilditPhase::Built);
            return;
        }
        break;
    default:
        break;
    }
    object.state = object.requested;
}

std::vector<SuperBuilditAction> SuperBuilditSet::update(const assets::PlacedObjectState& state, const SuperBuilditInput& input) {
    std::vector<SuperBuilditAction> actions;
    std::optional<std::size_t> queried;
    const auto release = [&](std::size_t index) {
        if (owner_ == index) {
            owner_.reset();
            actions.push_back({SuperBuilditActionKind::PromptHidden, index, false});
        }
    };
    for (std::size_t i = 0; i < objects_.size(); ++i) {
        SuperBuilditObject& object = objects_[i];
        if (state.isDisabled(object.instance) || (!object.parent.empty() && state.isDisabled(object.parent))) continue;
        bool inside = false;
        if (input.playerFree && object.zoneActive && object.state == std::uint16_t(SuperBuilditPhase::Waiting) &&
            object.requested != std::uint16_t(SuperBuilditPhase::Confirm)) {
            inside = inZone(object, input.player);
            if (inside) {
                prompt_ = SuperBuilditPrompt{object.type, object.value, i, object.value <= input.superBricks};
                if (!object.promptShown && (!owner_ || *owner_ == i)) {
                    if (!owner_) actions.push_back({SuperBuilditActionKind::PromptShown, i, false});
                    owner_ = i;
                    object.promptShown = true;
                }
            }
        }
        if (object.promptShown && !inside) {
            release(i);
            object.promptShown = false;
        }
        stepState(i, actions);
        if (inside && !queried) queried = i;
    }
    if (queried && input.actionHeld && !input.playerCarrying) {
        SuperBuilditObject& object = objects_[*queried];
        if (object.state == std::uint16_t(SuperBuilditPhase::Waiting) && object.promptShown) {
            object.countSeen = input.superBricks;
            if (object.value <= input.superBricks && !object.built) {
                object.promptShown = false;
                dialogResult_ = kSuperBuilditDialogNone;
                confirmOpen_ = true;
                object.requested = std::uint16_t(SuperBuilditPhase::Confirm);
                actions.push_back({SuperBuilditActionKind::ConfirmOpened, *queried, false});
                release(*queried);
            }
        }
    }
    return actions;
}

std::string superBuilditPicture(const std::vector<std::string>& tableNames, int type) {
    if (type < 0 || std::size_t(type) >= tableNames.size()) return {};
    return lower(tableNames[std::size_t(type)]);
}

void applySuperBuilditLink(const SuperBuilditSet& set, assets::PlacedObjectState& state) {
    for (const SuperBuilditObject& object : set.objects()) {
        if (!object.propToReveal.empty()) state.disabled.insert(object.propToReveal);
        if (!object.useObject.empty()) state.disabled.insert(object.useObject);
    }
}

SuperBuilditSpend applySuperBuilditActions(const SuperBuilditSet& set, const std::vector<SuperBuilditAction>& actions, assets::PlacedObjectState& state,
                                           ScriptRunner& scripts) {
    SuperBuilditSpend spend;
    for (const SuperBuilditAction& action : actions) {
        if (action.kind != SuperBuilditActionKind::Built) continue;
        const SuperBuilditObject& object = set.objects()[action.index];
        if (!object.hideObject.empty()) state.disabled.insert(object.hideObject);
        state.disabled.insert(object.instance);
        if (!object.propToReveal.empty()) state.disabled.erase(object.propToReveal);
        if (!object.buildCompleteTrigger.empty()) scripts.triggerObjectEnabling(object.buildCompleteTrigger);
        if (!object.useObject.empty()) state.disabled.erase(object.useObject);
        if (!action.countsBricks) continue;
        if (!object.parentTrigger.empty()) scripts.triggerObjectEnabling(object.parentTrigger);
        spend.bricks += object.value;
        ++spend.saveRequests;
    }
    return spend;
}

}
