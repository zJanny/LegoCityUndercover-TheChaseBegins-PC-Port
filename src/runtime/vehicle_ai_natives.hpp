#pragma once
// VehicleSetAI 0x1bfb50 -> 0x1796e8 and VehicleSetChased 0x26fe54 -> 0x1afd38 (docs/re/scripts-missions-1-4.md): the vehicle gets the AI driver record
// at record +0x2a4 (ai_driver_input.hpp) and VehicleSetChased puts it into the chase: vehicle state request 2, driver state 5 requested, the speeds and
// the route. The driver update, the route follower and the vehicle body are not connected to these records, so the stolen car of mission 1-4 does not
// drive (STAND-IN SCRIPT-S323). No SDL dependency.
#include "runtime/ai_driver_input.hpp"
#include "runtime/traffic_driver_calls.hpp"
#include "runtime/traffic_driver_steering.hpp"
#include "runtime/vehicle_drivetrain.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <utility>

namespace chase::runtime {

constexpr unsigned kVehicleSetAiArguments = 1;
constexpr unsigned kVehicleSetChasedArguments = 3;
constexpr unsigned kVehicleSetThrowObjectsArguments = 2;
constexpr std::uint16_t kVehicleChasedRequestedState = 2;
constexpr std::uint8_t kAiDriverChasedRequestedState = kAiStateSteer;
constexpr float kAiDriverDefaultHeightOffset = 1.0f;

struct VehicleAiRecord {
    AiDriverRecord driver;
    bool kinematic = false;                      // 0x22a270(entity, on): on after 0x1796e8, off after 0x1afd38
    std::uint16_t requestedVehicleState = 0;     // vehicle record +4
    std::uint32_t routeHash = 0;                 // name hash of the level path (driver +4)
    bool chased = false;
    AiDriverVehicle vehicle;                     // the vehicle record fields the driver writes
    TrafficSteeringState steering;               // the record's bytes +0x7a..+0xa3
    TrafficBrakeLights lights;                   // vehicle record +0x2ac..+0x2d8 (no parts set up, TRAFFIC-33)
    BodyInputs bodyInputs;                       // body +0x1bc..+0x1c8 as the last input copy 0x23019c wrote them
};

constexpr unsigned kVehicleChasedNearestSamples = 10;
constexpr float kVehicleChasedNearestScale = 1.0f;

class VehicleAiNatives {
public:
    // 0x1796e8: a new zero-filled record each call, the node matrix from the vehicle's placement.
    void setAi(const std::string& lowerVehicle, const AiMatrix& node);
    // 0x1afd38: a vehicle without a record gets one first (0x1afe3c).
    void setChased(const std::string& lowerVehicle, const AiMatrix& node, std::uint32_t routeHash, float speed);
    // 0x21d5b0(path + 8, node position, 10, 1.0) of VehicleSetChased for the route distance +0x64; unset, the distance stays 0.
    void setRouteNearest(std::function<float(std::uint32_t route, const AiVec3& position, unsigned samples, float scale)> nearest) {
        nearest_ = std::move(nearest);
    }
    const VehicleAiRecord* find(const std::string& lowerVehicle) const;
    VehicleAiRecord* mutableRecord(const std::string& lowerVehicle);
    std::size_t size() const { return records_.size(); }

private:
    VehicleAiRecord& create(const std::string& lowerVehicle, const AiMatrix& node);

    std::map<std::string, VehicleAiRecord> records_;
    std::function<float(std::uint32_t, const AiVec3&, unsigned, float)> nearest_;
};

}
