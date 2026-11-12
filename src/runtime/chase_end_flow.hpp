#pragma once
// The contact of the player's vehicle on the chased car (docs/re/traffic-ai-driver-chase-end.md): the handler 0x19293c sends the car its damage message
// (0x244fc8 calls the kind's handler at once) and then scales the body's linear velocity (0x24092c, 0x248850, 0x229920) by a factor of the closing speed.
// No SDL dependency.
#include "runtime/vehicle_chase_events.hpp"

#include <optional>

namespace chase::runtime {

constexpr float kChasedCarVelocityBand = 12.0f;
constexpr float kChasedCarVelocityScaleSlow = 0.9f;
constexpr float kChasedCarVelocityScaleMedium = 0.8f;
constexpr float kChasedCarVelocityScaleFast = 0.7f;
constexpr float kChasedCarVelocityScaleFastest = 0.4f;

// 0x192a64..0x192a98: the band is trunc(speed / 12); 0 gives 0.9, 1 gives 0.8, 2 gives 0.7, any other 0.4.
float chasedCarVelocityScale(float closingSpeed);

struct ChasedCarContactResult {
    DamageMessage message;
    ChasedCarDamageReply reply;
    float velocityScale = 1.0f;
};

// 0x1929c0..0x192adc: nothing unless the contact sends the damage message; then the reply of message 0 and the velocity factor that follows it whatever the reply.
std::optional<ChasedCarContactResult> chasedCarContact(AiDriverRecord& driver, VehicleRecord& record, const ChasedCarContact& contact);

}
