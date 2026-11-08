#pragma once
// The end of a car chase (docs/re/traffic-ai-driver.md): the vehicle handler 0x19293c of a car with an AI driver record turns a hard contact of the player's
// vehicle into a damage message (0x1929c0) and, when that message would take the last heart of a car in the driver's state 5, requests the driver's state 6
// (0x192ae0, 0x192b34) instead of destroying it. Entering state 6 (0x1cb52c) fires VehicleChaseEnded at the car. No SDL dependency.
#include "runtime/ai_driver_input.hpp"
#include "runtime/vehicle.hpp"
#include "runtime/vehicle_damage.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace chase::runtime {

constexpr float kChasedCarContactSpeed = 3.0f;
constexpr std::uint8_t kChasedCarHitHold = 128;
constexpr std::uint32_t kChasedCarContactHearts = 1;
constexpr const char* kVehicleChaseEndedEvent = "vehiclechaseended";

struct ChasedCarContact {
    bool otherIsPlayerVehicle = false;  // the other entity is 0x24255c(*0x3772b8), the vehicle the player drives
    bool otherDestroyed = false;        // its entity word +0xc bit 0x10
    float speed = 0.0f;                 // 0x19b5c4
};

// 0x1929c0..0x192a60: the damage message the car sends itself (word +0xc = 1 heart, kind 11, type 0). None unless the driver is in state 5 with +0xac = 0.
std::optional<DamageMessage> chasedCarContactDamage(const AiDriverRecord& driver, const ChasedCarContact& contact);

struct ChasedCarDamageReply {
    bool consumed = false;     // the handler returned 1: the base handler 0x1797c4 does not run
    bool chaseEnded = false;   // the driver requested state 6 and the car was left with 0 hearts
};

// Message 0 of 0x19293c (0x192ae0..0x192b88, 0x192b20). The special case of the car named 7-9_RexCar (0x192b3c..0x192b78) is not ported (SCRIPT-S336).
ChasedCarDamageReply chasedCarDamage(AiDriverRecord& driver, VehicleRecord& record, const DamageMessage& message);

struct ChaseEndEntry {
    std::string event;            // the event fired at the car (0x23cee8 with *0x375fe0)
    int smokeEffect = 0;          // 0x22a124(entity, 0x11f)
    bool kinematic = false;       // 0x22a270(entity, 0)
};

// Entering the driver's state 6 (0x1cb52c): the forced stop of the record is set (0x229be4(entity, 1)). The second event of 0x1cb568 (only with a global
// object at *0x376c78) is not ported (SCRIPT-S336).
ChaseEndEntry chaseEndEntry(VehicleRecord& record);

}
