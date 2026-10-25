#include "runtime/ai_driver_input.hpp"

#include "runtime/pedestrian_manager.hpp"

#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string>

namespace chase::runtime {
namespace {

template <typename Hook>
const Hook& need(const Hook& hook, const char* name) {
    if (!hook) throw std::logic_error(std::string("AI driver: hook ") + name + " is not set");
    return hook;
}

const AiRouteFunctions& needRoutes(const AiRouteFunctions* routes) {
    if (routes == nullptr) throw std::logic_error("AI driver: no route functions");
    return *routes;
}

std::int32_t floatBits(float value) {
    std::int32_t bits = 0;
    std::memcpy(&bits, &value, sizeof bits);
    return bits;
}

float dot(const AiVec3& a, const AiVec3& b) {
    float sum = a[0] * b[0];
    sum += a[1] * b[1];
    sum += a[2] * b[2];
    return sum;
}

float distanceSquared(const AiVec3& a, const AiVec3& b) {
    const float dx = a[0] - b[0];
    const float dy = a[1] - b[1];
    const float dz = a[2] - b[2];
    float sum = dx * dx;
    sum += dy * dy;
    sum += dz * dz;
    return sum;
}

AiVec3 rotate(const AiVec3& v, const AiMatrix& m) {
    AiVec3 out{};
    for (int i = 0; i < 3; ++i) {
        float sum = v[0] * m[0][i];
        sum += v[1] * m[1][i];
        sum += v[2] * m[2][i];
        out[i] = sum;
    }
    return out;
}

AiMatrix placement(const AiVec3& position, const AiVec3& forward) {
    return pedestrianPlacement(position, forward);
}

AiVec3 probeAhead(const AiMatrix& node) {
    AiVec3 probe{};
    for (int i = 0; i < 3; ++i) probe[i] = node[3][i] + node[2][i] * kAiReturnProbe;
    return probe;
}

float heightOffset(const AiDriverRecord& driver, float bodyBase) {
    return driver.heightOffset - bodyBase;
}

void leaveSignal(AiDriverRecord& driver, const AiDriverHooks& hooks) {
    if ((driver.flags & kAiDriverFlagSignal) == 0) return;
    need(hooks.leaveSignalQueue, "leaveSignalQueue")(driver.signal);
    driver.flags = static_cast<std::uint8_t>(driver.flags & ~kAiDriverFlagSignal);
}

float randomCruise(GameRandom& random) {
    return kAiCruiseBase + gameRandomUnit(random) * kAiCruiseRange;
}

void obstacleRule(AiDriverRecord& driver, const AiDriverVehicle& vehicle, const AiDriverInputFrame& frame, const AiDriverHooks& hooks) {
    bool holdEnded = false;
    bool resume = true;
    if (driver.hold != 0) {
        driver.hold = static_cast<std::uint8_t>(driver.hold - 1);
        holdEnded = driver.hold == 0;
        resume = holdEnded;
    }
    if (resume && vehicle.brakeInput == 0.0f) driver.targetSpeed = driver.cruiseSpeed;
    if (!holdEnded && driver.stateUpdates % kAiCheckPeriod != 0) return;
    if (driver.signal != 0 && (driver.flags & kAiDriverFlagSignal) != 0 && !need(hooks.signalLetsPass, "signalLetsPass")(driver.signal)) {
        driver.targetSpeed = 0.0f;
        driver.hold = kAiHoldUpdates;
        return;
    }
    const float stopping = std::sqrt(driver.speed);
    const AiVec3& position = frame.node[3];
    for (const AiObstacle& other : frame.obstacles) {
        AiVec3 delta{};
        for (int i = 0; i < 3; ++i) delta[i] = other.node[3][i] - position[i];
        const float forward = dot(delta, frame.node[2]);
        const float reach = frame.halfExtents[2] + other.halfExtents[2] + stopping + kAiObstacleMargin;
        if (!(forward > 0.0f) || !(forward < reach)) continue;
        const float width = std::fabs(dot(rotate(other.halfExtents, other.node), frame.node[0])) + kAiObstacleSideMargin + frame.halfExtents[0];
        if (std::fabs(dot(delta, frame.node[0])) < width) {
            driver.targetSpeed = 0.0f;
            driver.hold = kAiHoldUpdates;
        }
    }
}

AiDriverInputResult steerAlongRoute(AiDriverRecord& driver, const AiDriverInputFrame& frame, const AiDriverHooks& hooks) {
    AiDriverInputResult result;
    const AiRouteFunctions& routes = needRoutes(frame.routes);
    if (frame.playerInVehicle) {
        const float factor = (driver.flags & kAiDriverFlagFullSpeed) != 0 ? kAiSteerFullFactor : kAiSteerFactor;
        const float limit = frame.gearSpeed * factor;
        driver.targetSpeed = limit < driver.cruiseSpeed ? limit : driver.cruiseSpeed;
    }
    if (frame.hasBody && (frame.collisionFlags & 0x200u) != 0 && frame.layerBit) result.clearCollisionFlag200 = true;
    const float nearest = need(routes.nearest, "nearest")(driver.route, frame.node[3], kAiNearestSamples, kAiNearestScale);
    const float ahead = need(routes.ahead, "ahead")(driver.route, kAiSteerLookAhead, kAiSteerLookStep, nearest);
    AiVec3 point{};
    AiVec3 direction{};
    need(routes.evaluate, "evaluate")(driver.route, ahead, &point, &direction);
    AiMatrix target = placement(point, direction);
    driver.routeDistance = ahead;
    driver.path = target;
    target[3][1] += heightOffset(driver, frame.bodyBase);
    need(hooks.steerToward, "steerToward")(target, frame.dt);
    need(hooks.copyInputs, "copyInputs")();
    if (driver.hold != 0) driver.hold = static_cast<std::uint8_t>(driver.hold - 1);
    return result;
}

void returnToRoute(AiDriverRecord& driver, const AiDriverInputFrame& frame, const AiDriverHooks& hooks) {
    AiMatrix target = driver.path;
    target[3][1] += heightOffset(driver, frame.bodyBase);
    need(hooks.steerToward, "steerToward")(target, frame.dt);
    const float facing = dot(frame.node[2], driver.path[2]);
    if (facing < 0.0f) {
        const AiRouteFunctions& routes = needRoutes(frame.routes);
        AiMatrix node = frame.node;
        if (need(routes.follow, "follow")(probeAhead(node), node, driver.target, driver.route, driver.routeDistance)) {
            need(hooks.setNodeMatrix, "setNodeMatrix")(placement(node[3], node[2]));
            driver.signal = need(routes.signal, "signal")(driver.route);
            need(routes.evaluate, "evaluate")(driver.route, driver.routeDistance, &driver.path[3], &driver.path[2]);
            driver.path = placement(driver.path[3], driver.path[2]);
        }
        need(hooks.copyInputs, "copyInputs")();
        return;
    }
    const float radius = floatBits(facing) >= floatBits(kAiReturnAlignedDot) ? kAiReturnNearAligned : kAiReturnNear;
    if (distanceSquared(frame.node[3], driver.path[3]) < radius * radius) {
        need(hooks.copyInputs, "copyInputs")();
        driver.requestedState = kAiStateRoute;
        return;
    }
    need(hooks.copyInputs, "copyInputs")();
}

void enterRouteState(AiDriverRecord& driver, std::uint8_t state, const AiDriverStateFrame& frame, const AiDriverHooks& hooks, GameRandom& random) {
    const AiRouteFunctions& routes = needRoutes(frame.routes);
    if (state == kAiStateRoute) driver.cruiseSpeed = randomCruise(random);
    if (state == kAiStateSteer && (driver.flags & kAiDriverFlagListed) == 0) need(hooks.setListed, "setListed")(true);
    driver.entryPosition = frame.node[3];
    const float length = static_cast<float>(need(routes.length, "length")(driver.route));
    if (!(driver.routeDistance < length)) {
        driver.routeDistance = length;
        if (driver.targetSpeed > 0.0f) {
            std::uint32_t next = 0;
            if (need(hooks.chooseNextRoute, "chooseNextRoute")(driver, next)) {
                if (next != 0) driver.requestedState = kAiStateJunction;
                else need(hooks.leaveTraffic, "leaveTraffic")();
            }
        }
    }
    need(routes.evaluate, "evaluate")(driver.route, driver.routeDistance + kAiTargetStep, &driver.target, nullptr);
    driver.target[1] += heightOffset(driver, frame.bodyBase);
    need(hooks.snapTarget, "snapTarget")();
    driver.moveCounter = 0;
}

void enterStart(AiDriverRecord& driver, AiDriverVehicle& vehicle, const AiDriverStateFrame& frame, const AiDriverHooks& hooks, GameRandom& random) {
    const AiRouteFunctions& routes = needRoutes(frame.routes);
    driver.flags = static_cast<std::uint8_t>(driver.flags & ~kAiDriverFlagSignal);
    driver.cruiseSpeed = randomCruise(random);
    vehicle.hearts = vehicle.maxHearts;
    vehicle.health = kAiStartHearts;
    vehicle.brakeInput = 0.0f;
    need(routes.evaluate, "evaluate")(driver.route, driver.routeDistance + kAiTargetStep, &driver.target, nullptr);
    driver.target[1] += heightOffset(driver, frame.bodyBase);
    need(hooks.snapTarget, "snapTarget")();
    driver.moveCounter = 0;
    for (int row = 0; row < 3; ++row)
        for (int column = 0; column < 3; ++column) driver.path[row][column] = frame.body[column][row];
    driver.keep = static_cast<std::uint8_t>(random.next() % kAiKeepRandomRange + kAiKeepBase);
    vehicle.entityFlag20000 = false;
    need(hooks.setKinematic, "setKinematic")(true);
}

void stopRule(AiDriverRecord& driver, AiDriverVehicle& vehicle, const AiDriverHooks& hooks) {
    if (need(hooks.stopAtRouteEnd, "stopAtRouteEnd")()) {
        vehicle.brakeInput = 1.0f;
        driver.targetSpeed = 0.0f;
    } else if (driver.hold == 0 && floatBits(vehicle.brakeInput) == floatBits(1.0f)) {
        vehicle.brakeInput = 0.0f;
    }
}

void knockedStep(AiDriverRecord& driver, const AiDriverPostFrame& frame, const AiDriverHooks& hooks) {
    const float sinceContact = frame.now - driver.contactTime;
    if (frame.bodyMoveSpeed < kAiKnockedSpeed && !(kAiKnockedSeconds > sinceContact)) {
        const AiRouteFunctions& routes = needRoutes(frame.routes);
        AiMatrix node = frame.node;
        if (need(routes.follow, "follow")(probeAhead(node), node, driver.target, driver.route, driver.routeDistance)) {
            need(hooks.setNodeMatrix, "setNodeMatrix")(placement(node[3], node[2]));
            driver.signal = need(routes.signal, "signal")(driver.route);
            need(routes.evaluate, "evaluate")(driver.route, driver.routeDistance, &driver.path[3], &driver.path[2]);
            driver.path = placement(driver.path[3], driver.path[2]);
            if (static_cast<std::int32_t>(frame.groundedWheels) >= static_cast<std::int32_t>(frame.wheelCount) - 1) {
                driver.requestedState = kAiStateReturn;
            }
        }
    }
    need(hooks.copyInputs, "copyInputs")();
}

}

AiDriverInputResult runAiDriverInput(AiDriverRecord& driver, const AiDriverVehicle& vehicle, const AiDriverInputFrame& frame,
                                     const AiDriverHooks& hooks) {
    switch (driver.state) {
    case kAiStateRoute:
    case kAiStateJunction:
        obstacleRule(driver, vehicle, frame, hooks);
        return {};
    case kAiStateSteer:
        return steerAlongRoute(driver, frame, hooks);
    case kAiStateReturn:
        returnToRoute(driver, frame, hooks);
        return {};
    default:
        return {};
    }
}

void runAiDriverStateChange(AiDriverRecord& driver, AiDriverVehicle& vehicle, const AiDriverStateFrame& frame, const AiDriverHooks& hooks,
                            GameRandom& random) {
    if (driver.state != driver.requestedState) {
        if (driver.state == kAiStateSteer) {
            need(hooks.setListed, "setListed")(false);
            driver.flags = static_cast<std::uint8_t>(driver.flags & ~kAiDriverFlagListed);
        } else if (driver.state == kAiStateReturn) {
            need(hooks.setKinematic, "setKinematic")(true);
        }
        switch (driver.requestedState) {
        case kAiStateStart:
            enterStart(driver, vehicle, frame, hooks, random);
            break;
        case kAiStateRoute:
        case kAiStateSteer:
            enterRouteState(driver, driver.requestedState, frame, hooks, random);
            break;
        case kAiStateKnocked:
            leaveSignal(driver, hooks);
            need(hooks.setKinematic, "setKinematic")(false);
            break;
        case kAiStateStopping:
            need(hooks.setKinematic, "setKinematic")(false);
            need(hooks.enterStopping, "enterStopping")();
            break;
        case kAiStateBreak:
            need(hooks.destroy, "destroy")();
            break;
        case kAiStateReturn:
            driver.entryPosition = frame.node[3];
            break;
        default:
            break;
        }
        driver.stateUpdates = 0;
        driver.state = driver.requestedState;
    }
    switch (driver.state) {
    case kAiStateStart:
        driver.requestedState = kAiStateRoute;
        break;
    case kAiStateKnocked:
        if (frame.bodySpeed > 0.0f) {
            vehicle.brakeInput = 1.0f;
            vehicle.handBrakeInput = 1.0f;
        }
        if (frame.activationState == 2 || frame.activationState == 5) need(hooks.wakeBody, "wakeBody")();
        break;
    case kAiStateSteer:
        if ((driver.stateUpdates & kAiEffectPeriodMask) == 0 && (driver.flags & kAiDriverFlagEffect) != 0) {
            const float side = (gameRandomUnit(random) - 0.5f) * 2.0f;
            need(hooks.rearEffect, "rearEffect")({side, 0.5f, -2.0f});
        }
        break;
    default:
        break;
    }
    driver.stateUpdates += 1;
}

void runAiDriverPostStep(AiDriverRecord& driver, AiDriverVehicle& vehicle, const AiDriverPostFrame& frame, const AiDriverHooks& hooks,
                         GameRandom& random) {
    if (need(hooks.stopAll, "stopAll")()) driver.targetSpeed = 0.0f;
    const bool stopped = driver.targetSpeed == 0.0f || driver.speed == 0.0f;
    need(hooks.lights, "lights")(stopped, frame.dt);
    switch (driver.state) {
    case kAiStateHold:
        need(hooks.setNodeMatrix, "setNodeMatrix")(driver.path);
        break;
    case kAiStateRoute: {
        const float length = static_cast<float>(static_cast<std::int32_t>(need(needRoutes(frame.routes).length, "length")(driver.route)));
        if (!(length - kAiRouteEndMargin > driver.routeDistance)) stopRule(driver, vehicle, hooks);
        need(hooks.moveAlongRoute, "moveAlongRoute")(driver);
        break;
    }
    case kAiStateJunction:
        stopRule(driver, vehicle, hooks);
        need(hooks.moveAlongRoute, "moveAlongRoute")(driver);
        if (floatBits(driver.routeDistance) > floatBits(kAiJunctionLength)) {
            leaveSignal(driver, hooks);
            driver.route = driver.nextRoute;
            driver.signal = need(needRoutes(frame.routes).signal, "signal")(driver.route);
            driver.requestedState = kAiStateRoute;
            driver.routeDistance = gameRandomUnit(random) * kAiJunctionRandomStart;
        }
        break;
    case kAiStateKnocked:
        knockedStep(driver, frame, hooks);
        break;
    case kAiStateSteer:
    case kAiStateReturn:
        driver.speed = frame.bodySpeed;
        need(hooks.normalGravity, "normalGravity")();
        need(hooks.steerStep, "steerStep")();
        break;
    case kAiStateStopping:
        need(hooks.copyInputs, "copyInputs")();
        if (floatBits(frame.bodySpeed) <= floatBits(kAiStoppedSpeed) && driver.stateUpdates >= kAiStoppingUpdates) {
            driver.requestedState = kAiStateBreak;
        }
        break;
    default:
        break;
    }
    if (need(hooks.nodeHidden, "nodeHidden")()) {
        if (driver.keep != 0) driver.keep = static_cast<std::uint8_t>(driver.keep - 1);
    } else if (driver.keep < kAiKeepOffScreen) {
        driver.keep = kAiKeepOffScreen;
    }
}

AiDriverRecord createAiDriverRecord(float heightOffset, const AiMatrix& node, const AiDriverHooks& hooks) {
    AiDriverRecord driver;
    driver.heightOffset = heightOffset;
    driver.path = node;
    driver.flags = static_cast<std::uint8_t>(driver.flags | kAiDriverFlagAi);
    need(hooks.setKinematic, "setKinematic")(true);
    return driver;
}

}
