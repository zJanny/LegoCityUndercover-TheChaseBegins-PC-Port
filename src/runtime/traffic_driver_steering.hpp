#pragma once
// The steering of the traffic AI driver in states 5 and 8 (docs/re/traffic-ai-driver-steering.md): the controller 0x27872c that turns a
// target matrix into the steer, accelerate and brake inputs of the vehicle record, the contact step 0x29cc38 of the post step 0x1bc79c, and
// the top gear speed 0x20362c of a vehicle body. No SDL dependency.
#include "runtime/ai_driver_input.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace chase::runtime {

constexpr float kTrafficSteerProportional = 5.0f;        // 0x278a9c
constexpr float kTrafficSteerDerivative = 1.0f;          // 0x278aa4, target speed 0 or more
constexpr float kTrafficSteerReverseDerivative = 5.0f;   // 0x278a9c, target speed below 0
constexpr float kTrafficSpeedProportional = 0.9f;        // 0x278ac4
constexpr float kTrafficSpeedDerivative = 0.002f;        // 0x278ac8
constexpr float kTrafficLaunchProportional = 100.0f;     // 0x278acc
constexpr float kTrafficPlaceHeight = 10.0f;             // 0x278a98
constexpr float kTrafficStuckSpeed = 5.0f;               // 0x278a9c
constexpr std::uint32_t kTrafficStuckUpdates = 90;       // 0x2787e8
constexpr float kTrafficStraightAngle = 0.0872664675f;    // 0x278aa8 (0x3db2b8c3, 5 degrees)
constexpr float kTrafficSlowTurnSpeed = 20.0f;           // 0x278aac
constexpr float kTrafficSlowTurnCruise = 0.5f;           // 0x278ab0
constexpr float kTrafficContactMoveLimit = 20.0f;        // 0x29cd94
constexpr unsigned kTrafficContactMaxEntries = 128;      // 0x29cd34

// One controller of five floats in the driver record: +0x7c..+0x8c for the steering, +0x90..+0xa0 for the speed.
struct TrafficPid {
    float previousError = 0.0f;  // +0
    float integral = 0.0f;       // +4
    float proportional = 0.0f;   // +8
    float integralGain = 0.0f;   // +0xc
    float derivative = 0.0f;     // +0x10
};

// The driver record bytes +0x7a..+0xa3, which AiDriverRecord does not hold yet (zero-filled by 0x1796e8 like the rest of the record).
struct TrafficSteeringState {
    std::uint8_t launch = 0;     // +0x7a: bit 0 is set by 0x26d8c4 and cleared by 0x27872c once the speed reaches the target speed
    TrafficPid steer;            // +0x7c
    TrafficPid speed;            // +0x90
};

struct TrafficSteerFrame {
    AiMatrix node{};             // the car's node matrix (0x250810 of entity +0x3c)
    float dt = 0.0f;             // s0 of the call (0x23602c in both callers)
    float bodyBase = 0.0f;       // body +0x54 (0x20af24 = driver +0x74 - body +0x54)
    bool forcedStop = false;     // vehicle record byte +0x310 not 0
    float gearSpeed = 0.0f;      // 0x20362c(entity +0x84)
};

struct TrafficSteerResult {
    std::vector<AiMatrix> placements;  // 0x23091c(body, matrix, 0) in call order
    float steer = 0.0f;                // vehicle record +0x288
    std::optional<float> accelerate;   // vehicle record +0x28c, written when the speed output is above 0
    std::optional<float> brake;        // vehicle record +0x290, written otherwise
};

// 0x27872c(entity, target, dt). Writes driver +0x70 and +0xa4 and the controllers; the caller applies the result to the vehicle record
// and the body.
TrafficSteerResult steerTrafficCar(AiDriverRecord& driver, TrafficSteeringState& state, const AiMatrix& target,
                                   const TrafficSteerFrame& frame);

// The descriptor 0x29cc38 builds at sp+0x210 for the grid gather 0x237f30 (word order of docs/re/collision.md "The query layers").
struct TrafficContactDescriptor {
    std::uint32_t include = 4;
    std::uint32_t exclude = 0;
    std::uint32_t kindsInclude = 0xffffffffu;
    std::uint32_t kindsExclude = 0;
    std::uint32_t ownerFlagsInclude = 0;
    std::uint32_t ownerFlagsExclude = 16;
    std::uint32_t ownerStateInclude = 512;
    std::uint32_t ownerStateExclude = 3;
    std::uint8_t mode = 1;
};

struct TrafficContactStep {
    AiVec3 moved{};              // node position - the old driver +0x58
    AiVec3 centre{};             // the box centre: the node position, now also driver +0x58
    AiVec3 halfExtents{};        // entity +0x5c plus the moved distance when it is above 0 and below 20, on every axis
};

// 0x29cc38 up to the gather: driver +0x58 becomes the node position.
TrafficContactStep trafficContactStep(AiDriverRecord& driver, const AiVec3& nodePosition, float entityRadius);

struct TrafficContactHooks {
    // 0x237f30(0x578dfc, centre and half extents, list, 128, descriptor): the number of entries listed; the car itself is the excluded
    // owner.
    std::function<std::uint32_t(const AiVec3& centre, const AiVec3& halfExtents, const TrafficContactDescriptor& descriptor,
                                unsigned maxEntries)>
        gather;
    // 0x21a73c(entity, (moved, 0.0), list, count, 255, the unwritten word sp+4, 0, 0); its result is not used.
    std::function<void(const AiVec3& moved, std::uint32_t count)> slide;
};

// 0x29cc38(entity): the step, the gather, and the slide when the gather listed anything.
void runTrafficContactStep(AiDriverRecord& driver, const AiVec3& nodePosition, float entityRadius, const TrafficContactHooks& hooks);

// 0x20362c(body) = +0x268 / |ratio[+0x298 - 1] x +0x29c x scale| x 0.10472 x +0x2f8: the speed at MaxRPM in the last gear, which equals
// TopSpeed (+0x2a0) for the final drive 0x2435d0 computes. `gearCount` is body +0x298 (1..7, the table 0x376d84 has 7 ratios).
float vehicleTopGearSpeed(float maxRpm, int gearCount, float finalDrive, float wheelRadius);

}
