#pragma once
// Driving a vehicle (docs/re/vehicles-driving.md): the player input 0x29cd9c, the state rules of the input copy 0x23019c, the writers of the
// record states in the vehicle message handler (messages 84 and 85) and a kinematic body that stands in for the physics body (STAND-IN
// VEH-S3: no suspension, tyres, gears, tilt or collision; its speed and turn tables are oracle measurements). No SDL dependency.
#include <array>
#include <cstdint>
#include <optional>

namespace chase::runtime {

constexpr std::uint16_t kVehicleStateHandBrake = 3;
constexpr std::uint16_t kVehicleStateDriven = 5;
constexpr std::uint16_t kVehicleStateDestroyed = 6;
constexpr std::uint16_t kVehicleStateDestroyedB = 7;
constexpr std::uint16_t kVehicleStateParkedHandBrake = 11;

// 0x29cd9c: steer is the stick x value, replaced by -1 or +1 while the left or right D-pad channel (14, 15) is not zero; accelerate is
// channel 16 (A) above zero, brake channel 17 (B) above zero.
struct DriveControls {
    float stick = 0.0f;
    float dpadLeft = 0.0f;
    float dpadRight = 0.0f;
    float accelerate = 0.0f;
    float brake = 0.0f;
};

struct DriveInput {
    float steer = 0.0f;
    bool accelerate = false;
    bool brake = false;
};

DriveInput readDriveInput(const DriveControls& controls);

// The four inputs the body receives after 0x23019c.
struct DriveCommand {
    float steer = 0.0f;
    bool accelerate = false;
    bool brake = false;
    bool handBrake = false;
};

// 0x23019c: states 3, 4 and 11 set the hand brake (6 and 7 leave the inputs alone); a forced stop sets hand brake and brake, clears accelerate
// and keeps the steering; the brake is released once the body speed is 0.1 or less.
DriveCommand commandForState(std::uint16_t state, const DriveInput& input, bool forcedStop, float speed);

// Record state rules of the vehicle message handler 0x17a458 (message 84, the player got in) and 0x17a530 (message 85, the player jumped out).
std::optional<std::uint16_t> requestedStateOnPlayerEntered(std::uint16_t state);
std::optional<std::uint16_t> requestedStateOnPlayerExited(std::uint16_t state);

constexpr std::uint16_t kVehicleStateStuck = 8;
constexpr float kStuckUprightLimit = 0.2f;
constexpr float kStuckSpeedLimit = 2.0f;
constexpr float kStuckSeconds = 2.0f;

// The stuck rule of 0x16ce9c: while the node's upright value (the float at matrix + 0x14) is below 0.2, the entity is not frozen (flag +0xc bit 4)
// and the body speed is below 2.0 the timer record +0x31c adds dt; above 2.0 seconds the requested state becomes 8. A body that is not tipped over
// resets the timer. Returns true when state 8 is requested.
bool updateStuckTimer(float& timer, float upright, bool frozen, float speed, float dt);

struct DriveBody {
    std::array<float, 3> position = {0, 0, 0};
    float heading = 0.0f;  // atan2(forward x, forward z); a positive steer turns it up
    float speed = 0.0f;    // along the heading, negative in reverse
    int stoppingUpdates = 0;  // consecutive updates with brake or hand brake
};

// STAND-IN VEH-S3. The tables hold the measurements of the police trooper (docs/re/vehicles-driving.md); the top speed is the class value TopSpeed.
void stepDriveBody(DriveBody& body, const DriveCommand& command, float topSpeed, float dt);

float driveAcceleration(float speed);
float driveBrakeDeceleration(int stoppingUpdates);
float driveHandBrakeDeceleration(int stoppingUpdates);
float driveReverseAcceleration();
float driveCoastDeceleration(float speed);
float driveReverseCoastDeceleration(float speed);
float driveCurvature(float steerMagnitude);
float driveCurvatureSpeedScale(float speed);

constexpr float kDriveReverseSpeedLimit = 10.0f;

}
