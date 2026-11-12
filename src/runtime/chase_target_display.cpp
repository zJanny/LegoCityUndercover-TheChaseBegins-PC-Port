#include "runtime/chase_target_display.hpp"

namespace chase::runtime {

bool vehicleReturnShowsTarget(bool playersVehicle, std::optional<std::uint8_t> driverState) {
    return !playersVehicle && driverState && *driverState == kAiStateSteer;
}

TargetHeartsSource targetHeartsOff() { return TargetHeartsSource{}; }

}
