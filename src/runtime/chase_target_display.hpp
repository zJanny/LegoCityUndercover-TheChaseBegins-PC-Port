#pragma once
// Which calls of 0x230f04 the vehicle code makes besides VehicleSetChased (docs/re/traffic-driver-states.md): the vehicle return 0x22b224 for a
// vehicle that is not the player's current one, and VehicleSetDCamPathFollow 0x28548c (entity 0, off). No SDL dependency.
#include "runtime/ai_driver_input.hpp"
#include "runtime/target_hearts_display.hpp"

#include <cstdint>
#include <optional>

namespace chase::runtime {

// 0x22b258..0x22b30c: a vehicle other than the one the player drives (0x24255c of the global at 0x22b584) with an AI driver record whose state
// is 5 shows its hearts; any other state or no record leaves the display alone.
bool vehicleReturnShowsTarget(bool playersVehicle, std::optional<std::uint8_t> driverState);

// 0x28548c after 0x1afd38: 0x230f04(0), the display off.
TargetHeartsSource targetHeartsOff();

}
