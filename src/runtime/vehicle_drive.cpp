#include "runtime/vehicle_drive.hpp"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <utility>

namespace chase::runtime {
namespace {

using Point = std::pair<float, float>;

float interpolate(std::initializer_list<Point> table, float x) {
    const Point* previous = table.begin();
    for (const Point* point = table.begin(); point != table.end(); previous = point++) {
        if (x <= point->first) {
            if (point == table.begin()) return point->second;
            const float t = (x - previous->first) / (point->first - previous->first);
            return previous->second + (point->second - previous->second) * t;
        }
    }
    return previous->second;
}

constexpr float kStopSpeed = 1.7f;
constexpr int kStrongBrakeUpdates = 3;
constexpr float kStrongBrakeDeceleration = 118.0f;
constexpr float kBrakeDeceleration = 30.0f;
constexpr int kStrongHandBrakeUpdates = 2;
constexpr float kStrongHandBrakeDeceleration = 102.0f;
constexpr float kHandBrakeDeceleration = 50.0f;
constexpr float kReverseAcceleration = 3.9f;
constexpr float kLaunchSpeed = 2.1f;
constexpr float kLaunchAcceleration = 63.0f;

}

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

float driveAcceleration(float speed) {
    return interpolate({{0.0f, kLaunchAcceleration}, {kLaunchSpeed - 0.2f, kLaunchAcceleration}, {kLaunchSpeed - 0.1f, 8.5f}, {4.2f, 8.5f}, {6.7f, 11.1f},
                        {10.6f, 12.5f}, {14.6f, 11.4f}, {18.1f, 9.9f}, {21.3f, 8.9f}, {24.0f, 6.9f}, {25.8f, 4.0f}, {27.0f, 1.5f}, {28.0f, 1.5f}},
                       speed);
}

float driveBrakeDeceleration(int stoppingUpdates) { return stoppingUpdates < kStrongBrakeUpdates ? kStrongBrakeDeceleration : kBrakeDeceleration; }

float driveHandBrakeDeceleration(int stoppingUpdates) { return stoppingUpdates < kStrongHandBrakeUpdates ? kStrongHandBrakeDeceleration : kHandBrakeDeceleration; }

float driveReverseAcceleration() { return kReverseAcceleration; }

float driveCoastDeceleration(float speed) {
    return interpolate({{0.0f, 14.0f}, {2.0f, 14.0f}, {5.0f, 10.1f}, {8.0f, 10.0f}, {12.0f, 10.3f}, {14.0f, 12.4f}, {16.0f, 10.4f}, {18.0f, 11.2f},
                        {20.0f, 10.5f}},
                       speed);
}

float driveReverseCoastDeceleration(float speed) {
    return interpolate({{2.0f, 5.1f}, {5.0f, 3.4f}, {7.0f, 5.5f}, {9.0f, 8.5f}}, speed);
}

float driveCurvature(float steerMagnitude) {
    return interpolate({{0.0f, 0.0f}, {0.1005f, 0.0035f}, {0.291f, 0.01075f}, {0.5238f, 0.02905f}, {0.7354f, 0.0732f}, {0.9471f, 0.116f}, {1.0f, 0.1207f}},
                       std::min(steerMagnitude, 1.0f));
}

float driveCurvatureSpeedScale(float speed) {
    return interpolate({{0.0f, 1.0f}, {8.0f, 1.0f}, {11.6f, 0.93f}, {15.6f, 0.84f}, {17.0f, 0.80f}, {24.0f, 0.57f}, {28.0f, 0.5f}}, speed);
}

void stepDriveBody(DriveBody& body, const DriveCommand& command, float topSpeed, float dt) {
    float speed = body.speed;
    const float magnitude = std::fabs(speed);
    const auto towardsZero = [&](float rate) {
        const float change = rate * dt;
        speed = speed > 0.0f ? std::max(speed - change, 0.0f) : std::min(speed + change, 0.0f);
    };
    const bool stopping = command.handBrake || command.brake || (command.accelerate && speed < 0.0f);
    body.stoppingUpdates = stopping ? body.stoppingUpdates + 1 : 0;
    if (command.handBrake) {
        if (magnitude < kStopSpeed) speed = 0.0f;
        else towardsZero(driveHandBrakeDeceleration(body.stoppingUpdates - 1));
    } else if (command.brake) {
        if (speed > 0.0f) towardsZero(driveBrakeDeceleration(body.stoppingUpdates - 1));
        else speed = std::max(speed - driveReverseAcceleration() * dt, -kDriveReverseSpeedLimit);
    } else if (command.accelerate) {
        if (speed >= 0.0f) speed = std::min(speed + driveAcceleration(speed) * dt, topSpeed);
        else towardsZero(driveBrakeDeceleration(body.stoppingUpdates - 1));
    } else if (magnitude < kStopSpeed) {
        speed = 0.0f;
    } else {
        towardsZero(speed > 0.0f ? driveCoastDeceleration(speed) : driveReverseCoastDeceleration(magnitude));
    }
    body.speed = speed;
    const float steerSign = command.steer < 0.0f ? -1.0f : 1.0f;
    const float curvature = driveCurvature(std::fabs(command.steer)) * driveCurvatureSpeedScale(std::fabs(speed));
    body.heading += steerSign * curvature * speed * dt;
    body.position[0] += std::sin(body.heading) * speed * dt;
    body.position[2] += std::cos(body.heading) * speed * dt;
}

}
