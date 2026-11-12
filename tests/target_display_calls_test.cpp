// The calls of the target display besides VehicleSetChased (src/runtime/chase_target_display.hpp). The rules are transcribed from the vehicle return
// 0x22b258..0x22b30c and VehicleSetDCamPathFollow 0x28548c (llvm-objdump of game.arm.elf, 20261112-enemy-D, 2026-11-12,
// docs/re/traffic-driver-states.md).
#include "runtime/chase_target_display.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void run() {
    require(vehicleReturnShowsTarget(false, kAiStateSteer), "a returning vehicle that is not the player's, driver in state 5: shows the hearts");
    require(!vehicleReturnShowsTarget(true, kAiStateSteer), "the player's own vehicle takes the other branch of 0x22b224");
    require(!vehicleReturnShowsTarget(false, kAiStateStopping) && !vehicleReturnShowsTarget(false, kAiStateHold), "another driver state leaves the display alone");
    require(!vehicleReturnShowsTarget(false, std::nullopt), "no driver record leaves the display alone");

    TargetHeartsDisplay display;
    TargetHeartsSource car;
    car.kind = TargetHeartsKind::Vehicle;
    car.entity = "stolen_car";
    car.hearts = 5;
    car.most = 5;
    car.portraitName = "Vehicle_Sports";
    setTargetHearts(display, car);
    require(display.shown && display.target == "stolen_car", "a chased car is shown");
    setTargetHearts(display, targetHeartsOff());
    require(!display.shown && display.most == 0 && display.hearts == 0 && display.target.empty(), "VehicleSetDCamPathFollow turns the display off with entity 0");
}

}

int main() {
    try {
        run();
    } catch (const std::exception& error) {
        std::cerr << "target-display-calls: " << error.what() << '\n';
        return 1;
    }
    std::cout << "target-display-calls: ok\n";
    return 0;
}
