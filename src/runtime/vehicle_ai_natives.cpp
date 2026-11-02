#include "runtime/vehicle_ai_natives.hpp"

namespace chase::runtime {

VehicleAiRecord& VehicleAiNatives::create(const std::string& lowerVehicle, const AiMatrix& node) {
    VehicleAiRecord record;
    AiDriverHooks hooks;
    hooks.setKinematic = [&record](bool on) { record.kinematic = on; };
    record.driver = createAiDriverRecord(kAiDriverDefaultHeightOffset, node, hooks);
    return records_[lowerVehicle] = record;
}

void VehicleAiNatives::setAi(const std::string& lowerVehicle, const AiMatrix& node) { create(lowerVehicle, node); }

void VehicleAiNatives::setChased(const std::string& lowerVehicle, const AiMatrix& node, std::uint32_t routeHash, float speed) {
    const auto found = records_.find(lowerVehicle);
    VehicleAiRecord& record = found != records_.end() ? found->second : create(lowerVehicle, node);
    AiDriverRecord& driver = record.driver;
    driver.flags = static_cast<std::uint8_t>(driver.flags | kAiDriverFlagAi);
    record.kinematic = false;
    record.requestedVehicleState = kVehicleChasedRequestedState;
    if (driver.state == kAiStateSteer) driver.state = kAiStateRoute;
    driver.requestedState = kAiDriverChasedRequestedState;
    driver.cruiseSpeed = speed;
    driver.speed = 0.0f;
    driver.targetSpeed = speed;
    driver.route = routeHash;
    driver.routeDistance = 0.0f;
    driver.stateUpdates = 0;
    record.routeHash = routeHash;
    record.chased = true;
}

const VehicleAiRecord* VehicleAiNatives::find(const std::string& lowerVehicle) const {
    const auto found = records_.find(lowerVehicle);
    return found == records_.end() ? nullptr : &found->second;
}

VehicleAiRecord* VehicleAiNatives::mutableRecord(const std::string& lowerVehicle) {
    const auto found = records_.find(lowerVehicle);
    return found == records_.end() ? nullptr : &found->second;
}

}
