#include "game_level.hpp"
#include "runtime/vehicle_chase_events.hpp"

#include <iostream>

namespace game_level {
namespace runtime = chase::runtime;

void LevelSession::chasedCarImpact(std::size_t vehicleIndex, float speed, const runtime::ContactOther& other) {
    const std::string& instance = boardingVehicles[vehicleIndex].instance;
    runtime::VehicleAiRecord* ai = levelScripts.vehicleAi().mutableRecord(instance);
    if (!ai || !ai->chased) return;
    runtime::VehicleRecord& record = *levelScripts.vehicles().mutableRecord(boardingRecords[vehicleIndex]);
    const std::optional<runtime::DamageMessage> message =
        runtime::chasedCarContactDamage(ai->driver, {other.playerVehicle, other.destroyed, speed});
    if (!message) return;
    const runtime::ChasedCarDamageReply reply = runtime::chasedCarDamage(ai->driver, record, *message);
    if (!reply.consumed) {
        const runtime::DamageOutcome outcome = runtime::vehicleDamage(record, *message, false);
        if (outcome.heartLost) {
            vehicleDamageEffects(vehicleIndex, outcome);
            std::cout << "vehicle " << instance << ": heart lost (" << speed << " m/s), " << unsigned(record.hearts) << " left\n";
        }
        return;
    }
    if (!reply.chaseEnded) return;
    ai->driver.state = runtime::kAiStateStopping;
    ai->driver.stateUpdates = 0;
    const runtime::ChaseEndEntry entry = runtime::chaseEndEntry(record);
    ai->kinematic = entry.kinematic;
    runtime::DamageOutcome smoke;
    smoke.smokeEffect = entry.smokeEffect;
    vehicleDamageEffects(vehicleIndex, smoke);
    std::cout << "vehicle " << instance << ": chase ended at update " << updatesForLog << '\n';
    levelScripts.fireObjectEvent(entry.event, instance);
}

}
