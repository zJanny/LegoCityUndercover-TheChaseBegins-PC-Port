#include "runtime/vehicle_prop_smash.hpp"

#include "runtime/level_start.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>

namespace chase::runtime {

SmashBox worldSmashBox(const assets::Matrix& node, const SmashBox& local) {
    SmashBox world;
    for (std::size_t axis = 0; axis < 3; ++axis)
        world.centre[axis] = local.centre[0] * node[axis] + local.centre[1] * node[4 + axis] + local.centre[2] * node[8 + axis] + node[12 + axis];
    constexpr float kKeepsHalf = 0x1.ff7cecp-1f;
    if (node[0] > kKeepsHalf && node[5] > kKeepsHalf) {
        world.half = local.half;
        return world;
    }
    for (std::size_t axis = 0; axis < 3; ++axis)
        world.half[axis] = std::fabs(node[axis] * local.half[0]) + std::fabs(node[4 + axis] * local.half[1]) + std::fabs(node[8 + axis] * local.half[2]);
    return world;
}

bool smashBoxesOverlap(const SmashBox& a, const SmashBox& b) {
    for (std::size_t axis = 0; axis < 3; ++axis)
        if (std::fabs(b.centre[axis] - a.centre[axis]) > a.half[axis] + b.half[axis]) return false;
    return true;
}

bool vehicleSmashPassRuns(const SmashingVehicle& vehicle) {
    const float length = std::sqrt(vehicle.velocity[0] * vehicle.velocity[0] + vehicle.velocity[1] * vehicle.velocity[1] + vehicle.velocity[2] * vehicle.velocity[2]);
    return vehicle.dynamicBody && length > 0.0f;
}

bool vehicleSmashesProp(const SmashingVehicle& vehicle, const SmashableProp& prop) {
    const SmashBox& gridShape = prop.meshRoot ? *prop.meshRoot : prop.entityBox;
    if (!smashBoxesOverlap(worldSmashBox(vehicle.node, vehicle.nodeBounds), worldSmashBox(prop.node, gridShape))) return false;
    const SmashBox vehicleWorld = worldSmashBox(vehicle.node, vehicle.entityBox);
    SmashBox inPropFrame;
    inPropFrame.half = vehicleWorld.half;
    const float offset[3] = {vehicleWorld.centre[0] - prop.node[12], vehicleWorld.centre[1] - prop.node[13], vehicleWorld.centre[2] - prop.node[14]};
    for (std::size_t axis = 0; axis < 3; ++axis)
        inPropFrame.centre[axis] = offset[0] * prop.node[axis * 4] + offset[1] * prop.node[axis * 4 + 1] + offset[2] * prop.node[axis * 4 + 2];
    return smashBoxesOverlap(inPropFrame, prop.entityBox);
}

SmashOutcome applySmashDamage(std::int16_t health) {
    SmashOutcome outcome;
    outcome.health = std::int16_t(health - kVehicleSmashDamage);
    outcome.destroyed = outcome.health <= 0;
    return outcome;
}


namespace {

constexpr float kWobbleHealthDefault = 2.0f;
constexpr std::int16_t kAnyDamageHealth = 1;

bool wobbleClass(const std::string& className) { return className == "leprops/leprop_wobblydestructible" || className == "props/prop_trafficlight"; }
bool anyDamageClass(const std::string& className) { return className == "props/prop_respawnabledestructible" || className == "leprops/leprop_simpledestructible"; }
bool scriptDestroyedClass(const std::string& className) { return className == "props/ability_dynamitedestructible"; }

std::string lowerName(std::string text) {
    for (char& c : text) c = char(std::tolower(static_cast<unsigned char>(c)));
    return text;
}

std::int16_t smashHealth(const std::string& className, const assets::LevelObject& object, const assets::Level& cls) {
    if (anyDamageClass(className)) return kAnyDamageHealth;
    float value = kWobbleHealthDefault;
    if (const assets::LevelParam* param = effectiveParam(object, cls, assets::nameHash("WobbleHealth"))) {
        if (param->type == 0) std::memcpy(&value, &param->value, 4);
        else value = float(std::int32_t(param->value));
    }
    return std::int16_t(std::int32_t(value));
}

}

void VehiclePropSmasher::load(const assets::Level& level, const assets::ClassLookup& classOf, const std::vector<assets::ObjectCollision>& placed) {
    props_.clear();
    for (const assets::ObjectCollision& entry : placed) {
        const bool smashable = wobbleClass(entry.className) || anyDamageClass(entry.className);
        if (!smashable && !scriptDestroyedClass(entry.className)) continue;
        const assets::LevelObject* object = nullptr;
        for (const assets::LevelObject& candidate : level.objects)
            if (candidate.instance == entry.instance) object = &candidate;
        const assets::Level* cls = classOf(entry.className);
        if (!object || !cls) continue;
        Entry prop;
        prop.instance = entry.instance;
        prop.collisionDefault = entry.collisionDefault;
        prop.smashable = smashable;
        prop.prop.node = entry.matrix;
        prop.prop.entityBox = SmashBox{{entry.centre[0], entry.centre[1], entry.centre[2]}, {entry.half[0], entry.half[1], entry.half[2]}};
        if (entry.mesh && !entry.mesh->nodes.empty()) {
            const assets::CollisionNode& root = entry.mesh->nodes.front();
            prop.prop.meshRoot = SmashBox{{root.centre[0], root.centre[1], root.centre[2]}, {root.half[0], root.half[1], root.half[2]}};
        }
        prop.prop.health = smashHealth(entry.className, *object, *cls);
        prop.effects = propDestroyEffects(*object, *cls);
        props_.push_back(prop);
    }
}

std::vector<SmashedProp> VehiclePropSmasher::smash(const SmashingVehicle& vehicle, const assets::PlacedObjectState& state) {
    std::vector<SmashedProp> destroyed;
    if (!vehicleSmashPassRuns(vehicle)) return destroyed;
    for (Entry& entry : props_) {
        if (!entry.smashable || entry.destroyed || state.isDisabled(entry.instance) || !state.collisionEnabled(entry.instance, entry.collisionDefault)) continue;
        if (!vehicleSmashesProp(vehicle, entry.prop)) continue;
        const SmashOutcome outcome = applySmashDamage(entry.prop.health);
        entry.prop.health = outcome.health;
        if (!outcome.destroyed) continue;
        entry.destroyed = true;
        destroyed.push_back({entry.instance, entry.prop, entry.effects});
    }
    return destroyed;
}

std::size_t VehiclePropSmasher::propCount() const {
    return std::size_t(std::count_if(props_.begin(), props_.end(), [](const Entry& entry) { return entry.smashable; }));
}

std::optional<SmashedProp> VehiclePropSmasher::destroyData(const std::string& lowerInstance) const {
    for (const Entry& entry : props_)
        if (lowerName(entry.instance) == lowerInstance) return SmashedProp{entry.instance, entry.prop, entry.effects};
    return std::nullopt;
}

}
