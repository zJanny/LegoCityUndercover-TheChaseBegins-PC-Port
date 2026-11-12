#include "runtime/chase_end_flow.hpp"

namespace chase::runtime {

float chasedCarVelocityScale(float closingSpeed) {
    if (!(closingSpeed < 3.0f * kChasedCarVelocityBand)) return kChasedCarVelocityScaleFastest;
    const int band = static_cast<int>(closingSpeed / kChasedCarVelocityBand);
    switch (band) {
    case 0: return kChasedCarVelocityScaleSlow;
    case 1: return kChasedCarVelocityScaleMedium;
    case 2: return kChasedCarVelocityScaleFast;
    default: return kChasedCarVelocityScaleFastest;
    }
}

std::optional<ChasedCarContactResult> chasedCarContact(AiDriverRecord& driver, VehicleRecord& record, const ChasedCarContact& contact) {
    const std::optional<DamageMessage> message = chasedCarContactDamage(driver, contact);
    if (!message) return std::nullopt;
    ChasedCarContactResult result;
    result.message = *message;
    result.reply = chasedCarDamage(driver, record, *message);
    result.velocityScale = chasedCarVelocityScale(contact.speed);
    return result;
}

}
