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
constexpr std::uint32_t kHashVehicleType = 0x5be19715, kHashGetIntoVehicle = 0xcae408a0;
constexpr const char* kRemoteControlVehicle = "5-8_vehicle_rc";
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
        waypoint.badgeFadeIn = floatOf(effectiveParam(object, *cls, assets::nameHash("BadgeFadeInTime")), 0.0f);
        waypoint.takePlayerControl = intOf(effectiveParam(object, *cls, kHashTakePlayerControl), 0) != 0;
        waypoint.autoTrigger = intOf(effectiveParam(object, *cls, kHashAutoTrigger), 0) != 0;
        waypoint.getIntoVehicle = intOf(effectiveParam(object, *cls, kHashGetIntoVehicle), 0) != 0;
        if (const assets::LevelParam* vehicleString = effectiveParam(object, *cls, kHashVehicleString)) {
            waypoint.kind = WaypointKind::Vehicle;
            const std::uint32_t named = assets::objectReferenceHash(*vehicleString);
            if (named)
                for (const assets::LevelObject& other : level.objects)
                    if (assets::nameHash(other.instance) == named) {
                        waypoint.namedVehicle = lower(other.instance);
                        break;
                    }
            if (waypoint.namedVehicle.empty()) waypoint.vehicleType = std::int8_t(intOf(effectiveParam(object, *cls, kHashVehicleType), 0));
        }
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
        const bool wasActive = waypoint.active;
        waypoint.active = enable;
        if (!enable) {
            if (wasActive && waypoint.state != 3) {
                waypoint.columnTween = NodeTween::start(1.0f, 0.0f, waypoint.fadeScaleIn);
                waypoint.badgeTween = NodeTween::start(1.0f, 0.0f, kWaypointBadgeHideSeconds);
                waypoint.disableFade = true;
            }
            return false;
        }
        waypoint.disableFade = false;
        {
            waypoint.fired = false;
            waypoint.state = 0;
            waypoint.timer = kWaypointEnableTimer;
            waypoint.columnTween = NodeTween::start(0.0f, 0.0f, 0.0f);
            waypoint.badgeTween = NodeTween::start(0.0f, 0.0f, 0.0f);
        }
        return waypoint.active;
    }
    return enable;
}

namespace {

bool boundContains(const WaypointBound& bound, const float local[3]) {
    if (!bound.active) return false;
    const float x = local[0] - bound.centre[0], y = local[1] - bound.centre[1], z = local[2] - bound.centre[2];
    switch (bound.shape) {
    case 4:
    case 6: return std::sqrt(x * x + z * z) < bound.half[0] && std::fabs(y) < bound.half[1];
    case 3: return std::fabs(x) < bound.half[0] && std::fabs(y) < bound.half[1] && std::fabs(z) < bound.half[2];
    case 2: return std::sqrt(x * x + y * y + z * z) < bound.half[0];
    default: return false;
    }
}

assets::Matrix waypointMatrix(const Waypoint& waypoint) {
    const float yaw = waypoint.yawDegrees * 3.14159265358979f / 180.0f;
    const float c = std::cos(yaw), s = std::sin(yaw);
    return {c, 0, -s, 0, 0, 1, 0, 0, s, 0, c, 0, waypoint.position[0], waypoint.position[1], waypoint.position[2], 1};
}

assets::Matrix relativeTo(const assets::Matrix& a, const assets::Matrix& b) {
    float inverseTranslation[3];
    for (int i = 0; i < 3; ++i) {
        float value = -b[12] * b[i * 4];
        value -= b[13] * b[i * 4 + 1];
        value -= b[14] * b[i * 4 + 2];
        inverseTranslation[i] = value;
    }
    assets::Matrix out{};
    for (int k = 0; k < 4; ++k) {
        for (int i = 0; i < 3; ++i) {
            float value = a[k * 4] * b[i * 4];
            value += a[k * 4 + 1] * b[i * 4 + 1];
            value += a[k * 4 + 2] * b[i * 4 + 2];
            value += a[k * 4 + 3] * inverseTranslation[i];
            out[k * 4 + i] = value;
        }
        out[k * 4 + 3] = a[k * 4 + 3];
    }
    return out;
}

float clampToBox(float value, float centre, float half) {
    const float low = centre - half, high = centre + half;
    if (value < low) return low;
    return value <= high ? value : high;
}

}

bool WaypointSet::inBound(const Waypoint& waypoint, const float player[3]) {
    if (!waypoint.hasBound) return false;
    const float yaw = waypoint.yawDegrees * 3.14159265358979f / 180.0f;
    const float dx = player[0] - waypoint.position[0], dy = player[1] - waypoint.position[1], dz = player[2] - waypoint.position[2];
    const float c = std::cos(yaw), s = std::sin(yaw);
    const float local[3] = {dx * c - dz * s, dy, dx * s + dz * c};
    return boundContains(waypoint.bound, local);
}

bool WaypointSet::vehicleInBound(const Waypoint& waypoint, const WaypointVehicle& vehicle) {
    if (!waypoint.hasBound) return false;
    const assets::Matrix m = relativeTo(waypointMatrix(waypoint), vehicle.matrix);
    const float centred[3] = {waypoint.bound.centre[0] - m[12], waypoint.bound.centre[1] - m[13], waypoint.bound.centre[2] - m[14]};
    float turned[3];
    for (int k = 0; k < 3; ++k) {
        float value = centred[0] * m[k * 4];
        value += centred[1] * m[k * 4 + 1];
        value += centred[2] * m[k * 4 + 2];
        turned[k] = value;
    }
    float clamped[3];
    for (int k = 0; k < 3; ++k) clamped[k] = clampToBox(turned[k], vehicle.boxCentre[k], vehicle.boxHalf[k]);
    float local[3];
    local[0] = (clamped[1] * m[4] + clamped[2] * m[8]) + (m[12] + clamped[0] * m[0]);
    local[1] = (clamped[1] * m[5] + clamped[2] * m[9]) + (m[13] + clamped[0] * m[1]);
    local[2] = (clamped[1] * m[6] + clamped[2] * m[10]) + (m[14] + clamped[0] * m[2]);
    return boundContains(waypoint.bound, local);
}

bool WaypointSet::vehicleMatches(const Waypoint& waypoint, const WaypointVehicle* vehicle) {
    if (waypoint.kind != WaypointKind::Vehicle) return true;
    if (!waypoint.namedVehicle.empty()) return vehicle && vehicle->instance == waypoint.namedVehicle;
    if (!vehicle) return false;
    return waypoint.vehicleType == 0 || vehicle->type == waypoint.vehicleType;
}

WaypointUpdate WaypointSet::update(float dt, const float player[3], const WaypointVehicle* vehicle,
                                   const std::function<bool(const std::string&)>& roomLoaded) {
    WaypointUpdate result;
    for (Waypoint& waypoint : waypoints_) {
        if (!waypoint.active) {
            if (waypoint.disableFade) {
                waypoint.columnTween.step(dt);
                waypoint.badgeTween.step(dt);
                if (waypoint.columnTween.value() <= 0.0f) waypoint.disableFade = false;
            }
            continue;
        }
        if (roomLoaded && !waypoint.room.empty() && !roomLoaded(waypoint.room)) continue;
        waypoint.columnTween.step(dt);
        waypoint.badgeTween.step(dt);
        if (waypoint.timer > 0.0f) {
            waypoint.timer -= dt;
            if (waypoint.timer <= 0.0f) waypoint.timer = 0.0f;
            if (waypoint.timer == 0.0f) {
                switch (waypoint.state) {
                case 0:
                    waypoint.state = 1;
                    waypoint.timer = waypoint.fadeScaleIn;
                    waypoint.columnTween = NodeTween::start(0.0f, 1.0f, waypoint.fadeScaleIn);
                    waypoint.badgeTween = NodeTween::start(0.0f, 0.0f, 0.0f);
                    break;
                case 1:
                    waypoint.badgeTween = NodeTween::start(0.0f, 1.0f, waypoint.badgeFadeIn);
                    break;
                case 2:
                    waypoint.state = 3;
                    waypoint.columnTween = NodeTween::start(1.0f, 0.0f, waypoint.fadeScaleOut);
                    waypoint.timer = waypoint.fadeScaleOut;
                    if (waypoint.takePlayerControl) controlTaken_ = true;
                    break;
                case 3:
                    if (!waypoint.triggerScript.empty()) result.startScripts.push_back(waypoint.triggerScript);
                    if (waypoint.takePlayerControl) {
                        controlTaken_ = false;
                        if (!waypoint.firedVehicle.empty()) result.vehicleActions.push_back({waypoint.firedVehicle, false});
                    }
                    waypoint.active = false;
                    result.deactivated.push_back(waypoint.instance);
                    break;
                default: break;
                }
            }
        }
        if (!waypoint.active || !waypoint.autoTrigger || !waypoint.hasBound || waypoint.fired) continue;
        if (!vehicleMatches(waypoint, vehicle)) continue;
        if (!inBound(waypoint, player) && !(vehicle && vehicleInBound(waypoint, *vehicle))) continue;
        if (waypoint.kind == WaypointKind::OnFoot && vehicle) {
            if (vehicle->instance != kRemoteControlVehicle) result.vehicleHint = true;
            continue;
        }
        waypoint.fired = true;
        waypoint.state = 2;
        waypoint.timer = kWaypointHitTimer;
        waypoint.firedVehicle = vehicle ? vehicle->instance : std::string();
        if (vehicle && waypoint.takePlayerControl && !waypoint.getIntoVehicle) result.vehicleActions.push_back({vehicle->instance, true});
    }
    return result;
}

}
