#include "runtime/traffic_driver_steering.hpp"

#include "runtime/player_intent.hpp"
#include "runtime/vehicle_drivetrain.hpp"

#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string>

namespace chase::runtime {
namespace {

constexpr float kRpmToRadians = 0.10471975f;  // 0x203670 (0x3dd6774f)
constexpr std::uint32_t kOneBits = 0x3f800000u;
constexpr std::uint32_t kMinusOneBits = 0xbf800000u;
constexpr AiVec3 kUnitZ = {0.0f, 0.0f, 1.0f};  // the float3 at 0x56d430

std::uint32_t floatBits(float value) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof bits);
    return bits;
}

float dot(const AiVec3& a, const AiVec3& b) {
    float sum = a[0] * b[0];
    sum += a[1] * b[1];
    sum += a[2] * b[2];
    return sum;
}

AiVec3 toNodeFrame(const AiVec3& point, const AiMatrix& node) {
    const AiVec3 d = {point[0] - node[3][0], point[1] - node[3][1], point[2] - node[3][2]};
    return {dot(d, node[0]), dot(d, node[1]), dot(d, node[2])};
}

float pidUpdate(TrafficPid& pid, float error, float dt) {
    pid.integral = pid.integral + error * dt;
    const float change = error - pid.previousError;
    pid.previousError = error;
    float out = pid.integralGain * pid.integral;
    out = out + pid.proportional * error;
    out = out + pid.derivative * (change / dt);
    return out;
}

float targetSpeedFor(const AiDriverRecord& driver, const AiVec3& local, float angle, float gearSpeed) {
    if (!(std::fabs(angle) >= kTrafficStraightAngle)) return driver.cruiseSpeed;
    if (!(kTrafficSlowTurnSpeed <= driver.speed)) return driver.cruiseSpeed;
    if (!(driver.cruiseSpeed * kTrafficSlowTurnCruise <= driver.speed)) return driver.cruiseSpeed;
    const float length = std::sqrt(dot(local, local));
    const float ahead = dot(local, kUnitZ) / length;
    const float factor = kTrafficSlowTurnCruise + ahead * kTrafficSlowTurnCruise;
    const float k = (driver.flags & kAiDriverFlagFullSpeed) != 0 ? kAiSteerFullFactor : kAiSteerFactor;
    const float limited = gearSpeed * k > driver.cruiseSpeed ? driver.cruiseSpeed : gearSpeed * k;
    return limited * factor;
}

}

TrafficSteerResult steerTrafficCar(AiDriverRecord& driver, TrafficSteeringState& state, const AiMatrix& target,
                                   const TrafficSteerFrame& frame) {
    TrafficSteerResult result;
    const float dt = frame.dt;
    AiVec3 local = toNodeFrame(target[3], frame.node);

    if (kTrafficPlaceHeight <= target[3][1] - frame.node[3][1]) result.placements.push_back(target);

    if (!frame.forcedStop) {
        const float difference = std::fabs(driver.speed - driver.targetSpeed);
        if (!(kTrafficStuckSpeed >= driver.speed) || !(kTrafficStuckSpeed < difference)) {
            driver.stateUpdates = 0;
        } else if (driver.stateUpdates >= kTrafficStuckUpdates) {
            AiMatrix placed = target;
            placed[3][1] = placed[3][1] - (driver.heightOffset - frame.bodyBase);
            result.placements.push_back(placed);
            driver.stateUpdates = 0;
        }
    }

    if (0.0f > driver.targetSpeed) {
        local[0] = -local[0];
        local[2] = -local[2];
    }
    const float angle = gameAtan2(local[0], local[2]);
    driver.targetSpeed = targetSpeedFor(driver, local, angle, frame.gearSpeed);
    const bool reverse = driver.targetSpeed < 0.0f;

    state.steer.proportional = kTrafficSteerProportional;
    state.steer.derivative = reverse ? kTrafficSteerReverseDerivative : kTrafficSteerDerivative;
    state.steer.integralGain = 0.0f;
    float steer = pidUpdate(state.steer, angle, dt);
    if (reverse) steer = -steer;
    if (std::int32_t(floatBits(steer)) >= std::int32_t(kOneBits))
        steer = 1.0f;
    else if (floatBits(steer) >= kMinusOneBits)
        steer = -1.0f;
    result.steer = steer;

    float proportional = kTrafficSpeedProportional;
    float derivative = kTrafficSpeedDerivative;
    if ((state.launch & 1) != 0) {
        if (!(driver.speed >= driver.targetSpeed)) {
            derivative = 0.0f;
            proportional = kTrafficLaunchProportional;
        } else {
            state.launch = 0;
        }
    }
    state.speed.proportional = proportional;
    state.speed.integralGain = 0.0f;
    state.speed.derivative = derivative;
    const float error = (driver.targetSpeed - driver.speed) * dt;
    float throttle = pidUpdate(state.speed, error, dt);
    if (std::int32_t(floatBits(throttle)) >= std::int32_t(kOneBits)) {
        throttle = 1.0f;
    } else {
        throttle = pidUpdate(state.speed, error, dt);
        if (floatBits(throttle) >= kMinusOneBits)
            throttle = -1.0f;
        else
            throttle = pidUpdate(state.speed, error, dt);
    }
    if (0.0f < throttle)
        result.accelerate = throttle;
    else
        result.brake = -throttle;
    return result;
}

TrafficContactStep trafficContactStep(AiDriverRecord& driver, const AiVec3& nodePosition, float entityRadius) {
    TrafficContactStep step;
    step.moved = {nodePosition[0] - driver.entryPosition[0], nodePosition[1] - driver.entryPosition[1],
                  nodePosition[2] - driver.entryPosition[2]};
    driver.entryPosition = nodePosition;
    float extra = 0.0f;
    if (dot(step.moved, step.moved) > 0.0f) {
        extra = std::sqrt(dot(step.moved, step.moved));
        if (kTrafficContactMoveLimit <= extra) extra = 0.0f;
    }
    const float half = entityRadius + extra;
    step.centre = driver.entryPosition;
    step.halfExtents = {half, half, half};
    return step;
}

void runTrafficContactStep(AiDriverRecord& driver, const AiVec3& nodePosition, float entityRadius, const TrafficContactHooks& hooks) {
    if (!hooks.gather || !hooks.slide) throw std::logic_error("traffic contact step: gather and slide hooks are required");
    const TrafficContactStep step = trafficContactStep(driver, nodePosition, entityRadius);
    const std::uint32_t count = hooks.gather(step.centre, step.halfExtents, TrafficContactDescriptor{}, kTrafficContactMaxEntries);
    if (count != 0) hooks.slide(step.moved, count);
}

float vehicleTopGearSpeed(float maxRpm, int gearCount, float finalDrive, float wheelRadius) {
    if (gearCount < 1 || gearCount > int(kGearRatios.size()))
        throw std::out_of_range("vehicle gear count " + std::to_string(gearCount) + " is outside the 7 ratios of 0x376d84");
    const float ratio = kGearRatios[std::size_t(gearCount - 1)] * finalDrive * kGearRatioScale;
    return maxRpm / std::fabs(ratio) * kRpmToRadians * wheelRadius;
}

}
