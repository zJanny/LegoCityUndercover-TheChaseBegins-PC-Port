#include "runtime/carry.hpp"
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

int objectType(const assets::Level& cls) {
    const std::uint32_t hash = assets::nameHash("ObjectType");
    for (const assets::LevelParam& param : cls.classParams)
        if (param.hash == hash) return int(param.value);
    return -1;
}

std::string referenceName(const assets::LevelObject& object, const assets::Level& cls, const char* name, const std::map<std::uint32_t, std::string>& names) {
    const assets::LevelParam* param = effectiveParam(object, cls, assets::nameHash(name));
    if (!param) return {};
    const auto found = names.find(assets::objectReferenceHash(*param));
    return found == names.end() ? std::string() : found->second;
}

bool boolParam(const assets::LevelObject& object, const assets::Level& cls, const char* name, bool fallback) {
    const assets::LevelParam* param = effectiveParam(object, cls, assets::nameHash(name));
    return param ? param->value != 0 : fallback;
}

}

void CarrySet::load(assets::FuseArchive& archive, const assets::Level& level, const assets::ClassLookup& classOf) {
    objects_.clear();
    targets_.clear();
    phase_ = CarryPhase::Free;
    carried_ = placingTarget_ = pickingUp_ = -1;
    std::map<std::uint32_t, std::string> names;
    for (const assets::LevelObject& object : level.objects) names[assets::nameHash(object.instance)] = lower(object.instance);
    for (const assets::LevelObject& object : level.objects) {
        if (object.className.rfind("props/prop_carry", 0) != 0) continue;
        const assets::Level* cls = classOf(object.className);
        const std::optional<assets::Placement> placed = assets::objectStart(level, object.instance);
        if (!cls || !placed) continue;
        const int type = objectType(*cls);
        if (type == kCarryObjectType) {
            CarryObject carry;
            carry.instance = lower(object.instance);
            carry.triggerPickup = referenceName(object, *cls, "TriggerPickup", names);
            carry.triggerOnRespawn = referenceName(object, *cls, "TriggerOnRespawn", names);
            std::copy_n(placed->position, 3, carry.home);
            std::copy_n(placed->position, 3, carry.position);
            carry.small = boolParam(object, *cls, "ATTR_SmallObject", true);
            carry.strengthOnly = boolParam(object, *cls, "ATTR_StrengthOnly", false);
            float seconds = kCarryDefaultResetSeconds;
            if (const assets::LevelParam* param = effectiveParam(object, *cls, assets::nameHash("ResetTime"))) {
                if (param->type == 0) std::memcpy(&seconds, &param->value, 4);
                else seconds = float(param->value);
            }
            carry.resetUpdates = unsigned(seconds * float(kCarryUpdatesPerSecond));
            if (!placed->mesh.empty()) {
                float centre[3], half[3];
                if (assets::modelBounds(archive, assets::meshModelPath(placed->mesh), centre, half)) {
                    carry.hasZone = true;
                    for (int a = 0; a < 3; ++a) {
                        carry.zoneCentre[a] = centre[a];
                        carry.zoneHalf[a] = half[a] + kCarryZoneMargin[a];
                    }
                }
            }
            objects_.push_back(std::move(carry));
        } else if (type == kCarryTargetType) {
            CarryTarget target;
            target.instance = lower(object.instance);
            target.useObject = referenceName(object, *cls, "UseObject", names);
            target.placeObject = referenceName(object, *cls, "PlaceObject", names);
            target.triggerObject = referenceName(object, *cls, "TriggerObject", names);
            std::copy_n(placed->position, 3, target.position);
            if (const assets::LevelParam* param = effectiveParam(object, *cls, assets::nameHash("ATTR_SFX_DROP"))) target.sfx = param->value;
            for (const assets::LevelBound& bound : cls->bounds)
                if (lower(bound.name) == "usebound") {
                    target.hasZone = true;
                    std::copy_n(bound.centre, 3, target.zoneCentre);
                    std::copy_n(bound.half, 3, target.zoneHalf);
                }
            targets_.push_back(std::move(target));
        }
    }
}

const CarryObject* CarrySet::find(const std::string& lowerInstance) const {
    for (const CarryObject& object : objects_)
        if (object.instance == lowerInstance) return &object;
    return nullptr;
}

bool CarrySet::position(const std::string& lowerInstance, float out[3]) const {
    for (const CarryObject& object : objects_) {
        if (object.instance != lowerInstance) continue;
        std::copy_n(object.position, 3, out);
        return true;
    }
    return false;
}

int CarrySet::carryingMode() const {
    if (carried_ < 0) return 0;
    return objects_[std::size_t(carried_)].small ? 1 : 2;
}

bool CarrySet::inZone(const float zoneCentre[3], const float zoneHalf[3], const float origin[3], const float point[3]) {
    for (int a = 0; a < 3; ++a)
        if (!(std::fabs(point[a] - origin[a] - zoneCentre[a]) < zoneHalf[a])) return false;
    return true;
}

std::vector<CarryAction> CarrySet::update(const assets::PlacedObjectState& state, const CarryInput& input) {
    std::vector<CarryAction> actions;
    std::copy_n(input.player, 3, player_);
    std::copy_n(input.facing, 3, facing_);
    for (std::size_t i = 0; i < targets_.size(); ++i) {
        CarryTarget& target = targets_[i];
        if (!target.usePending) continue;
        target.usePending = false;
        target.used = true;
        actions.push_back({CarryActionKind::Used, 0, i});
    }
    for (std::size_t i = 0; i < objects_.size(); ++i) {
        CarryObject& object = objects_[i];
        if (object.carried || object.resetCountdown == 0) continue;
        if (--object.resetCountdown == 0) {
            std::copy_n(object.home, 3, object.position);
            actions.push_back({CarryActionKind::Respawned, i, 0});
        }
    }
    if (carried_ >= 0) {
        CarryObject& held = objects_[std::size_t(carried_)];
        for (int a = 0; a < 3; ++a) held.position[a] = player_[a] + facing_[a] * kCarryHeldForward;
        held.position[1] += kCarryHeldHeight;
    }
    if (phase_ != CarryPhase::Free) {
        ++phaseUpdates_;
        if (phase_ == CarryPhase::PickingUp) {
            if (phaseUpdates_ == kCarryPickupAttachUpdates) {
                carried_ = pickingUp_;
                objects_[std::size_t(carried_)].carried = true;
                objects_[std::size_t(carried_)].resetCountdown = 0;
                actions.push_back({CarryActionKind::PickedUp, std::size_t(carried_), 0});
            }
            if (phaseUpdates_ >= kCarryPickupEndUpdates) phase_ = CarryPhase::Free;
        } else if (phase_ == CarryPhase::Placing) {
            if (phaseUpdates_ == kCarryPlaceReleaseUpdates && carried_ >= 0) {
                CarryObject& object = objects_[std::size_t(carried_)];
                CarryTarget& target = targets_[std::size_t(placingTarget_)];
                object.carried = false;
                std::copy_n(target.position, 3, object.position);
                target.usePending = true;
                actions.push_back({CarryActionKind::Placed, std::size_t(carried_), std::size_t(placingTarget_)});
                carried_ = -1;
            }
            if (phaseUpdates_ >= kCarryPlaceEndUpdates) phase_ = CarryPhase::Free;
        } else {
            if (phaseUpdates_ == kCarryDropReleaseUpdates && carried_ >= 0) {
                CarryObject& object = objects_[std::size_t(carried_)];
                object.carried = false;
                std::copy_n(player_, 3, object.position);
                object.resetCountdown = object.resetUpdates;
                actions.push_back({CarryActionKind::Dropped, std::size_t(carried_), 0});
                carried_ = -1;
            }
            if (phaseUpdates_ >= kCarryDropEndUpdates) phase_ = CarryPhase::Free;
        }
        return actions;
    }
    if (!input.playerFree || !input.actionPressed) return actions;
    if (carried_ >= 0) {
        const CarryObject& carriedObject = objects_[std::size_t(carried_)];
        phase_ = CarryPhase::Dropping;
        for (std::size_t i = 0; i < targets_.size(); ++i) {
            const CarryTarget& target = targets_[i];
            if (target.used || !target.hasZone || target.useObject != carriedObject.instance) continue;
            if (!inZone(target.zoneCentre, target.zoneHalf, target.position, player_)) continue;
            phase_ = CarryPhase::Placing;
            placingTarget_ = int(i);
            break;
        }
        phaseUpdates_ = 0;
        return actions;
    }
    for (std::size_t i = 0; i < objects_.size(); ++i) {
        const CarryObject& object = objects_[i];
        if (object.carried || !object.hasZone || object.strengthOnly || state.isDisabled(object.instance)) continue;
        if (!inZone(object.zoneCentre, object.zoneHalf, object.position, player_)) continue;
        phase_ = CarryPhase::PickingUp;
        phaseUpdates_ = 0;
        pickingUp_ = int(i);
        break;
    }
    return actions;
}

void applyCarryActions(const CarrySet& set, const std::vector<CarryAction>& actions, assets::PlacedObjectState& state, ScriptRunner& scripts) {
    for (const CarryAction& action : actions) {
        if (action.kind == CarryActionKind::PickedUp) {
            const CarryObject& object = set.objects()[action.object];
            if (!object.triggerPickup.empty()) scripts.triggerObjectEnabling(object.triggerPickup);
        } else if (action.kind == CarryActionKind::Respawned) {
            const CarryObject& object = set.objects()[action.object];
            if (!object.triggerOnRespawn.empty()) scripts.triggerObjectEnabling(object.triggerOnRespawn);
        } else if (action.kind == CarryActionKind::Used) {
            const CarryTarget& target = set.targets()[action.target];
            if (!target.useObject.empty()) state.disabled.insert(target.useObject);
            if (!target.placeObject.empty()) state.disabled.erase(target.placeObject);
            if (!target.triggerObject.empty()) scripts.triggerObjectEnabling(target.triggerObject);
        }
    }
}

}
