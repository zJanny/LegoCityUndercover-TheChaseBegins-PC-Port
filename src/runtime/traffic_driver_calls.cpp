#include "runtime/traffic_driver_calls.hpp"

#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string>

namespace chase::runtime {
namespace {

std::uint32_t floatBits(float value) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof bits);
    return bits;
}

template <typename Hook>
const Hook& need(const Hook& hook, const char* name) {
    if (!hook) throw std::logic_error(std::string("traffic driver call needs the hook ") + name);
    return hook;
}

void holdTrafficCar(TrafficRaceTrimCar& car, TrafficHeldCar& held) {
    car.steering->launch = 1;
    if (held.entity != 0) return;
    if (car.meshNode) {
        car.entityFade = kTrafficHeldFade;
        car.entityFadeFlags = static_cast<std::uint8_t>(car.entityFadeFlags & ~kTrafficHeldFadeBits);
    }
    held.entity = car.entity;
}

}

TrafficRaceTrimResult setTrafficRaceTrim(TrafficRaceTrimCar& car, float flag, TrafficHeldCar& held) {
    TrafficRaceTrimResult result;
    if (car.driver == nullptr) return result;
    if (car.steering == nullptr) throw std::logic_error("VehicleSetRaceTrim needs the driver's steering bytes");
    const bool on = floatBits(flag) != 0;
    if (on) car.driver->flags = static_cast<std::uint8_t>(car.driver->flags | kAiDriverFlagFullSpeed);
    else car.driver->flags = static_cast<std::uint8_t>(car.driver->flags & ~kAiDriverFlagFullSpeed);
    if (on) {
        result.hideTargetHearts = true;
        holdTrafficCar(car, held);
    } else {
        releaseTrafficHeldCar(car, held);
    }
    return result;
}

void releaseTrafficHeldCar(TrafficRaceTrimCar& car, TrafficHeldCar& held) {
    if (held.entity == 0 || held.entity != car.entity) return;
    if (car.driver == nullptr) throw std::logic_error("the held car has no driver record");
    if (car.meshNode) car.nodeFadeFlags = static_cast<std::uint8_t>(car.nodeFadeFlags | kTrafficHeldFadeBits);
    car.driver->flags = static_cast<std::uint8_t>(car.driver->flags & ~kAiDriverFlagAi);
    car.driver->requestedState = kAiStateHold;
    car.vehicleRequest = kTrafficReleasedVehicleRequest;
    held.entity = 0;
}

TrafficPlacementResult placeTrafficBody(TrafficPlacedBody& body, const AiMatrix& matrix, std::uint32_t room, bool trackCells,
                                        const TrafficPlacementHooks& hooks) {
    TrafficPlacementResult result;
    body.flags &= ~kTrafficPlacementClearedFlags;
    if (room == 0) room = need(hooks.roomAt, "roomAt")(matrix[3]);
    if (room == 0) return result;
    result.room = room;
    if ((body.rigidFlags & kTrafficRigidKinematic) != 0) result.resetFromNode = true;
    else result.movedToMatrix = true;
    body.rigid.velocity = {0.0f, 0.0f, 0.0f};
    body.rigid.angularVelocity = {0.0f, 0.0f, 0.0f};
    body.interpolationVelocity = {0.0f, 0.0f, 0.0f};
    body.interpolationAngularVelocity = {0.0f, 0.0f, 0.0f};
    body.rigid.force = {0.0f, 0.0f, 0.0f};
    body.rigid.torque = {0.0f, 0.0f, 0.0f};
    body.drivetrain.rpm = body.idleRpm;
    body.field28c = 0.0f;
    for (WheelSpinState& wheel : body.drivetrain.wheels) wheel.rate = 0.0f;
    body.groundedWheels = 0;
    result.queueCommand9 = true;
    if ((body.rigidFlags & (kTrafficRigidStatic | kTrafficRigidKinematic)) == 0) {
        if (body.activationState != 4 && body.activationState != 5) body.activationState = 1;
        body.rigid.deactivationTime = 0.0f;
    }
    const std::uint32_t node = need(hooks.roomNode, "roomNode")(room);
    if (body.nodeParent != node) {
        body.nodeParent = node;
        result.relinked = true;
    }
    result.trackCell = trackCells;
    result.placed = true;
    return result;
}

bool slideTrafficCarContacts(const TrafficContactCar& car, AiVec3& move, std::vector<TrafficContactEntry>& entries,
                             const TrafficContactSlideHooks& hooks) {
    for (TrafficContactEntry& entry : entries) {
        if (need(hooks.ignorePair, "ignorePair")(entry.owner)) entry.word |= kTrafficContactMarkBit;
    }
    const bool slide = !car.pusher;
    bool pushed = false;
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const TrafficContactEntry& entry = entries[i];
        if (entry.ownerNode == 0) continue;
        if (car.node == entry.ownerNodeParent || car.nodeParent == entry.ownerNode) continue;
        if ((entry.word & kTrafficContactSkipBit) != 0) continue;
        if (!need(hooks.push, "push")(move, i, entries, slide)) continue;
        pushed = true;
        if (!slide) break;
    }
    if (pushed) {
        const float length = std::sqrt(move[0] * move[0] + move[1] * move[1] + move[2] * move[2]);
        const float limit = car.radius * kTrafficContactMoveScale;
        if (length > limit) {
            const float scale = limit / length;
            move = {move[0] * scale, move[1] * scale, move[2] * scale};
        }
    }
    for (TrafficContactEntry& entry : entries) entry.word &= ~kTrafficContactMarkBit;
    return pushed;
}

void setTrafficChaseMarkers(ObjectiveState& objectives, const std::string& carryItWaypoint, const std::string& car, bool on, bool carEnabled) {
    objectives.setArrow(kTrafficChaseArrowList, car, on);
    objectives.setSubObjective(carryItWaypoint, car, kTrafficChaseGroupType, on, carEnabled);
}

void startTrafficWhistleStop(TrafficWhistleStop& stop, std::uint32_t clock, GameRandom& random) {
    const std::uint32_t extra = random.next() % kTrafficWhistleStopRandom;
    stop.until = extra + clock + kTrafficWhistleStopBase;
}

bool trafficWhistleStops(const TrafficWhistleStop& stop, std::uint32_t clock) {
    return clock < stop.until;
}

TrafficBrakeLightResult runTrafficBrakeLights(TrafficBrakeLights& lights, const TrafficBrakeLightFrame& frame) {
    TrafficBrakeLightResult result;
    if (lights.parts[0] < 0) return result;
    float level = 0.0f;
    bool lit = false;
    if ((!frame.hasBody || frame.gear != 0) && frame.brake > 0.0f) {
        level = 1.0f;
        lit = true;
    } else {
        level = lights.level - frame.dt * kTrafficBrakeLightFade;
        if (level < 0.0f) level = 0.0f;
    }
    for (std::size_t i = 0; i < kTrafficBrakeGlows; ++i) {
        if (lights.glowParts[i] < 0) continue;
        if (lit && !frame.nodeCulled) {
            if (lights.glows[i] == 0) result.glowStarts.push_back(i);
        } else if (lights.glows[i] != 0) {
            result.glowStops.push_back(i);
        }
    }
    if (lights.level == level) return result;
    lights.level = level;
    const std::uint8_t alpha = static_cast<std::uint8_t>(static_cast<std::int32_t>(kTrafficBrakeLightAlpha * level) & 0xff);
    for (std::size_t j = 0; j < kTrafficBrakeLightParts; ++j) {
        if (lights.parts[j] < 0) continue;
        result.parts.push_back({lights.parts[j], alpha != 0, alpha});
    }
    return result;
}

void endTrafficBrakeGlow(TrafficBrakeLights& lights, std::uint32_t handle, int event) {
    if (event != 2) return;
    for (std::uint32_t& glow : lights.glows) {
        if (glow == handle) glow = 0;
    }
}

}
