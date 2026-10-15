#include "runtime/vehicle_drive.hpp"

#include <cmath>

namespace chase::runtime {

DriveInput readDriveInput(const DriveControls& controls) {
    DriveInput input;
    input.steer = controls.stick;
    if (controls.dpadLeft != 0.0f) input.steer = -1.0f;
    if (controls.dpadRight != 0.0f) input.steer = 1.0f;
    input.accelerate = controls.accelerate > 0.0f;
    input.brake = controls.brake > 0.0f;
    return input;
}

DriveCommand commandForState(std::uint16_t state, const DriveInput& input, bool forcedStop, float speed) {
    DriveCommand command;
    command.steer = input.steer;
    command.accelerate = input.accelerate;
    command.brake = input.brake;
    if (state == kVehicleStateHandBrake || state == 4 || state == kVehicleStateParkedHandBrake) command.handBrake = true;
    if (forcedStop) {
        command.handBrake = true;
        command.accelerate = false;
        command.brake = std::fabs(speed) > 0.1f;
    }
    return command;
}

std::optional<std::uint16_t> requestedStateOnPlayerEntered(std::uint16_t state) {
    if (state == kVehicleStateDestroyed || state == kVehicleStateDestroyedB) return std::nullopt;
    return kVehicleStateDriven;
}

std::optional<std::uint16_t> requestedStateOnPlayerExited(std::uint16_t state) {
    if (state == 10 || state == 4) return std::nullopt;
    return kVehicleStateHandBrake;
}

bool updateStuckTimer(float& timer, float upright, bool frozen, float speed, float dt) {
    if (upright >= kStuckUprightLimit || frozen) {
        timer = 0.0f;
        return false;
    }
    if (speed >= kStuckSpeedLimit) return false;
    timer += dt;
    return timer > kStuckSeconds;
}

}
