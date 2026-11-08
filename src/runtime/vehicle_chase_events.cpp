#include "runtime/vehicle_chase_events.hpp"

namespace chase::runtime {

std::optional<DamageMessage> chasedCarContactDamage(const AiDriverRecord& driver, const ChasedCarContact& contact) {
    if (driver.state != kAiStateSteer || driver.hold != 0) return std::nullopt;
    if (!contact.otherIsPlayerVehicle || contact.otherDestroyed) return std::nullopt;
    if (!(contact.speed > kChasedCarContactSpeed)) return std::nullopt;
    DamageMessage message;
    message.hearts = kChasedCarContactHearts;
    message.fraction = 0.0f;
    message.kind = kVehicleDamageKindContact;
    message.blocked = 0;
    message.type = 0;
    return message;
}

ChasedCarDamageReply chasedCarDamage(AiDriverRecord& driver, VehicleRecord& record, const DamageMessage& message) {
    ChasedCarDamageReply reply;
    if (driver.requestedState == kAiStateStopping) {
        reply.consumed = true;
        return reply;
    }
    if (record.hearts == 0) return reply;
    const bool lastHeart = std::int32_t(record.hearts) <= std::int32_t(message.hearts);
    if (lastHeart && message.type != kVehicleDamageTypeScripted && driver.state == kAiStateSteer) {
        driver.requestedState = kAiStateStopping;
        record.hearts = 0;
        reply.consumed = true;
        reply.chaseEnded = true;
        return reply;
    }
    if (message.kind == kVehicleDamageKindContact) driver.hold = kChasedCarHitHold;
    return reply;
}

ChaseEndEntry chaseEndEntry(VehicleRecord& record) {
    record.forcedStop = 1;
    ChaseEndEntry entry;
    entry.event = kVehicleChaseEndedEvent;
    entry.smokeEffect = kVehicleSmokeLastHeart;
    entry.kinematic = false;
    return entry;
}

}
