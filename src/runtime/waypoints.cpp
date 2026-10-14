#include "runtime/waypoints.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>

namespace chase::runtime {

namespace {

constexpr const char* kClassNames[] = {"props/map_waypoint", "props/map_vehiclewaypoint", "props/map_getintovehiclewaypoint"};
constexpr std::uint32_t kHashTriggerScript = 0xd3575d66, kHashFadeScaleIn = 0xd7954f4e, kHashFadeScaleOut = 0x190be976;
constexpr std::uint32_t kHashTakePlayerControl = 0x59521dda, kHashAutoTrigger = 0xc93e4625, kHashVehicleString = 0x7629ded9;
constexpr const char* kBoundName = "TriggerBound";

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

const assets::LevelParam* findParam(const std::vector<assets::LevelParam>& params, std::uint32_t hash) {
    for (const auto& param : params)
        if (param.hash == hash) return &param;
    return nullptr;
}

const assets::LevelParam* effectiveParam(const assets::LevelObject& object, const assets::Level& cls, std::uint32_t hash) {
    const assets::LevelParam* classParam = findParam(cls.defaults, hash);
    if (!classParam) classParam = findParam(cls.classParams, hash);
    if (!classParam) return nullptr;
    if (classParam->flags & 2)
        if (const assets::LevelParam* instance = findParam(object.params, hash)) return instance;
    return classParam;
}

float floatOf(const assets::LevelParam* param, float fallback) {
    if (!param || param->type != 0) return fallback;
    float value;
    std::memcpy(&value, &param->value, 4);
    return value;
}

int intOf(const assets::LevelParam* param, int fallback) {
    if (!param) return fallback;
    if (param->type == 5 || param->type == 7) return int(param->value);
    if (param->type == 0) return int(floatOf(param, 0));
    return fallback;
}

std::string stringOf(const assets::LevelParam* param) {
    if (!param || param->type != 1 || param->blocks.empty()) return {};
    const auto& block = param->blocks[0];
    return std::string(block.begin(), std::find(block.begin(), block.end(), std::uint8_t(0)));
}

}

void WaypointSet::load(const assets::Level& level, const assets::ClassLookup& classOf) {
    waypoints_.clear();
    controlTaken_ = false;
    for (const assets::LevelObject& object : level.objects) {
        if (std::find(std::begin(kClassNames), std::end(kClassNames), object.className) == std::end(kClassNames)) continue;
        const assets::Level* cls = classOf(object.className);
        if (!cls) continue;
        Waypoint waypoint;
        waypoint.instance = lower(object.instance);
        waypoint.room = object.room;
        waypoint.triggerScript = stringOf(effectiveParam(object, *cls, kHashTriggerScript));
        waypoint.fadeScaleIn = floatOf(effectiveParam(object, *cls, kHashFadeScaleIn), 0.0f);
        waypoint.fadeScaleOut = floatOf(effectiveParam(object, *cls, kHashFadeScaleOut), 0.0f);
        waypoint.takePlayerControl = intOf(effectiveParam(object, *cls, kHashTakePlayerControl), 0) != 0;
        waypoint.autoTrigger = intOf(effectiveParam(object, *cls, kHashAutoTrigger), 0) != 0;
        waypoint.kind = findParam(object.params, kHashVehicleString) ? WaypointKind::Vehicle : WaypointKind::OnFoot;
        for (const assets::LevelBound& bound : cls->bounds)
            if (bound.name == kBoundName) {
                waypoint.hasBound = true;
                std::copy_n(bound.centre, 3, waypoint.bound.centre);
                std::copy_n(bound.half, 3, waypoint.bound.half);
                waypoint.bound.shape = bound.shape;
                waypoint.bound.active = bound.active != 0;
                break;
            }
        const auto position = assets::objectStart(level, object.instance);
        if (position) {
            std::copy_n(position->position, 3, waypoint.position);
            waypoint.yawDegrees = position->orientation[1];
        }
        waypoints_.push_back(std::move(waypoint));
    }
}

const Waypoint* WaypointSet::find(const std::string& lowerName) const {
    for (const Waypoint& waypoint : waypoints_)
        if (waypoint.instance == lowerName) return &waypoint;
    return nullptr;
}

bool WaypointSet::known(const std::string& lowerName) const { return find(lowerName) != nullptr; }

bool WaypointSet::active(const std::string& lowerName) const {
    const Waypoint* waypoint = find(lowerName);
    return waypoint && waypoint->active;
}

bool WaypointSet::setActive(const std::string& lowerName, bool enable) {
    for (Waypoint& waypoint : waypoints_) {
        if (waypoint.instance != lowerName) continue;
        const bool notAutoTrigger = !waypoint.autoTrigger;
        if (!enable && !notAutoTrigger) return waypoint.active;
        waypoint.active = enable;
        if (enable) {
            waypoint.fired = false;
            waypoint.state = 0;
            waypoint.timer = kWaypointEnableTimer;
        }
        return waypoint.active;
    }
    return enable;
}

bool WaypointSet::inBound(const Waypoint& waypoint, const float player[3]) {
    if (!waypoint.hasBound || !waypoint.bound.active) return false;
    const float yaw = waypoint.yawDegrees * 3.14159265358979f / 180.0f;
    const float dx = player[0] - waypoint.position[0], dy = player[1] - waypoint.position[1], dz = player[2] - waypoint.position[2];
    const float c = std::cos(yaw), s = std::sin(yaw);
    const float x = dx * c - dz * s - waypoint.bound.centre[0];
    const float y = dy - waypoint.bound.centre[1];
    const float z = dx * s + dz * c - waypoint.bound.centre[2];
    switch (waypoint.bound.shape) {
    case 4:
    case 6: return std::sqrt(x * x + z * z) < waypoint.bound.half[0] && std::fabs(y) < waypoint.bound.half[1];
    case 3: return std::fabs(x) < waypoint.bound.half[0] && std::fabs(y) < waypoint.bound.half[1] && std::fabs(z) < waypoint.bound.half[2];
    case 2: return std::sqrt(x * x + y * y + z * z) < waypoint.bound.half[0];
    default: return false;
    }
}

WaypointUpdate WaypointSet::update(float dt, const float player[3], bool playerInVehicle,
                                   const std::function<bool(const std::string&)>& roomLoaded) {
    WaypointUpdate result;
    for (Waypoint& waypoint : waypoints_) {
        if (!waypoint.active) continue;
        if (roomLoaded && !waypoint.room.empty() && !roomLoaded(waypoint.room)) continue;
        if (waypoint.timer > 0.0f) {
            waypoint.timer -= dt;
            if (waypoint.timer <= 0.0f) waypoint.timer = 0.0f;
            if (waypoint.timer == 0.0f) {
                switch (waypoint.state) {
                case 0:
                    waypoint.state = 1;
                    waypoint.timer = waypoint.fadeScaleIn;
                    break;
                case 2:
                    waypoint.state = 3;
                    waypoint.timer = waypoint.fadeScaleOut;
                    if (waypoint.takePlayerControl) controlTaken_ = true;
                    break;
                case 3:
                    if (!waypoint.triggerScript.empty()) result.startScripts.push_back(waypoint.triggerScript);
                    if (waypoint.takePlayerControl) controlTaken_ = false;
                    waypoint.active = false;
                    result.deactivated.push_back(waypoint.instance);
                    break;
                default: break;
                }
            }
        }
        if (!waypoint.active || !waypoint.autoTrigger || !waypoint.hasBound || waypoint.fired) continue;
        const bool eligible = waypoint.kind == WaypointKind::OnFoot || playerInVehicle;
        if (!eligible || (waypoint.kind == WaypointKind::OnFoot && playerInVehicle)) continue;
        if (!inBound(waypoint, player)) continue;
        waypoint.fired = true;
        waypoint.state = 2;
        waypoint.timer = kWaypointHitTimer;
    }
    return result;
}

}
