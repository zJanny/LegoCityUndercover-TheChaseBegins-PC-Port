#pragma once
// Engine, gears, steering and tyres of the vehicle body update 0x33aea8 (docs/re/vehicles-physics.md): the steering 0x33cb1c, the brake and
// gear rules 0x33b234..0x33b4fc, the wheel setup 0x3396cc, the tyre force 0x339374 / 0x339538, the wheel spin 0x339a30, the engine 0x339db0
// with its torque curve 0x339eb0 and the drivetrain coupling 0x33b604..0x33ba60. Most of these rates are per update, not per second, as the
// original has them. No SDL dependency.
#include "runtime/vehicle_body.hpp"
#include "runtime/vehicle_drive.hpp"

#include <array>
#include <cstddef>
#include <vector>

namespace chase::runtime {

// The table at 0x376d74 (ELF data, unchanged in the oracle): +4 ratio scale, +8 clutch scale, +0xc clutch torque, +0x10 the gear ratios
// (0 reverse, 1 neutral, 2..6 forward).
constexpr float kGearRatioScale = 1.0f;
constexpr float kClutchScale = 1.0f;
constexpr float kClutchTorque = 50.0f;
constexpr std::array<float, 7> kGearRatios = {-2.0f, 0.0f, 3.23f, 2.105f, 1.458f, 1.107f, 0.848f};
constexpr int kGearReverse = 0;
constexpr int kGearNeutral = 1;
constexpr int kGearFirst = 2;
// 0x33b73c: every gear above 0 becomes 6 in the drivetrain step; the body constructor 0x33ce74 starts in gear 2 with 7 gears (body+0x298).
constexpr int kGearTop = 6;
constexpr int kGearCount = 7;
// The base torque curve at 0x376da4 (32 entries, one per 500 rpm); 0x243e9c adds Friction * rpm to each entry.
constexpr std::array<float, 32> kEngineTorqueCurve = {50, 85, 150, 220, 260, 290, 300, 300, 300, 295, 285, 260, 230, 140, 50, 0,
                                                      0,  0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0,  0};
// The tyre surface factor *(0x376c48)[surface]: 0.5 for every surface in the oracle (a run-time table, its writer is not read; STAND-IN
// VEH-S40). Steep ground (normal y below 0.8) takes a tenth of it.
constexpr float kTyreSurfaceFriction = 0.5f;
// 0x376e58 +0: the grip of a wheel recovers by this per second (1 in the oracle, written at run time).
constexpr float kTyreGripRecovery = 1.0f;

// SteeringSpeed, SteeringSpeedNeg, SteeringSpeedCentre, SteeringLockMax, SteeringLockMin, SteeringLockPower (0x243c50, body+0xd6c; the
// Alt... set at +0xd84 is used while body flag +0xc4 bit 4 is set).
struct SteeringSpec {
    float speed = 2.0f;
    float speedNeg = 0.4f;
    float speedCentre = 0.5f;
    float lockMax = 0.9f;
    float lockMin = 0.4f;
    float lockPower = 3.0f;
};

// The class values of 0x243e9c, 0x243f90 and 0x243c50 and the final drive of 0x2435d0.
struct EngineSpec {
    std::array<float, 32> curve{};  // body+0x1e4: kEngineTorqueCurve[i] + Friction * 500 * i
    float redline = 0;              // body+0x264 Redline
    float maxRpm = 0;               // +0x268 MaxRPM
    float idleRpm = 0;              // +0x26c IdleSpeed
    float inertia = 0;              // +0x270 Inertia
    float friction = 0;             // +0x274 Friction
    float minimumThrottle = 0;      // +0x280 (0 from the constructor)
    float power = 0;                // +0x284 dtPower
    float finalDrive = 0;           // +0x29c: 0x2435d0 replaces dtFinalDrive
    float speedLimit = 1000.0f;     // +0xdbc (constructor)
    float slowdown = 0.2f;          // +0xdc0 Slowdown
    float mass = 0;                 // +0xc8: the class Mass times the mass scale
    SteeringSpec steering;
};

// 0x243e9c and 0x2435d0: the curve with friction and the final drive MaxRPM / (|ratio of the top gear| * TopSpeed) * (2 pi / 60) * the
// radius of wheel 0.
EngineSpec makeEngineSpec(float friction, float idleRpm, float inertia, float maxRpm, float redline, float power, float topSpeed,
                          float wheelRadius, float mass, float slowdown, const SteeringSpec& steering);
float engineFinalDrive(float maxRpm, float topSpeed, float wheelRadius);

// 0x339eb0: the cubic through the curve entries around rpm / 500 (fraction of the integer rpm), times dtPower; 0 below 0 rpm.
float engineTorque(const EngineSpec& engine, float rpm);

// Per-wheel values of 0x243628 and of the wheel constructor 0x339bec.
struct WheelGripSpec {
    float driveRatio = 0;       // w+0x64: 2 * WheelDriveRatio front, 2 - 2 * WheelDriveRatio rear
    bool steered = false;       // w+0x68: 1 for the front wheels
    float steerFactor = 1.0f;   // w+0x6c (0x339bec)
    float inertia = 0;          // w+0x70 %sWheelInertia
    float handbrakeScale = 0;   // w+0x94 %sHandbrakeScale
    float brakeForce = 0;       // w+0x9c %sBrakeForce
    float brakeFriction = 0;    // w+0xa0 %sBrakeFriction
    float frictionStatic = 0;   // w+0xa4: 2 * %sWheelFrictionStatic
    float frictionDynamic = 0;  // w+0xa8: 2 * %sWheelFrictionDynamic
    float driftScale = 1.0f;    // w+0xb0 %sDriftScale
};

struct WheelSpinState {
    float angle = 0;        // w+0x80
    float steerAngle = 0;   // w+0x84
    float rate = 0;         // w+0x88: rad/s
    float handBrake = 0;    // w+0x90
    float brake = 0;        // w+0x98
    float grip = 1.0f;      // w+0xac
    float skid = 0;         // w+0xb8
    Vector3 forward = {0, 0, 1};  // w+0xd4: rolling direction in the world
};

// body+0x1bc..+0x1c8, and `driver` for the body flag +0xc4 bit 3: without it 0x33b364 sets the brake input to 0.7 above 5 m/s.
// STAND-IN VEH-S41: the writer of bit 3 is not read; it is taken as set while the player drives (the seated trooper's flags are 0x800b).
struct BodyInputs {
    float steer = 0;
    float accelerate = 0;
    float brake = 0;
    float handBrake = 0;
    bool driver = true;
};

// 0x23019c copies the steer value and 1 or 0 for accelerate, brake and hand brake to body+0x1bc..+0x1c8.
BodyInputs bodyInputs(const DriveCommand& command);

struct DrivetrainState {
    float rpm = 0;                // body+0x27c (IdleSpeed after 0x243e9c)
    float clutch = 0;             // +0x290
    int gear = kGearFirst;        // +0x294
    float clutchScale = 1.0f;     // +0x288
    float steerAngle = 0;         // +0x2a8: radians, positive turns right
    float brakeOut = 0;           // +0x2ac
    float handBrakeOut = 0;       // +0x2b0
    float throttle = 0;           // s19 of 0x33aea8
    float steerFlick = 0;         // +0xd60
    int steerFlickUpdates = 0;    // +0xd68
    float previousSteer = 0;      // +0xd64
    std::vector<WheelSpinState> wheels;
};

// 0x33cb1c: steer input to the steer angle. `forwardSpeed` is the velocity along the body's forward axis.
void steerVehicle(DrivetrainState& state, const SteeringSpec& steering, float input, const Vector3& forward, const Vector3& velocity);

// 0x33b234..0x33b4fc for a body with a driver (flag +0xc4 bit 3): throttle, brake and hand brake outputs and the gear (reverse below 1.5 m/s
// with the brake, first gear again with A above -3.5 m/s); a brake above 6 m/s with the wheels turned more than 0.2 becomes the hand brake.
// Returns false when the speed limit +0xdbc is exceeded (no throttle, linear damping 0.2).
bool controlDrivetrain(DrivetrainState& state, const EngineSpec& engine, const BodyInputs& controls, float forwardSpeed, float speedSquared);

// 0x33b4a0: the body wakes (mode 2 to 1, 0x2308e4) unless it is slow (forward speed below 0.5, speed squared below 1.5), has no accelerate
// or brake input and the steer input did not change.
bool vehicleWakes(const BodyInputs& controls, float forwardSpeed, float speedSquared, float previousSteer);

// A body asleep (mode 2) keeps the steer input as the previous one and its wheels stop spinning (0x33b564).
void sleepVehicle(DrivetrainState& state, const BodyInputs& controls);

// 0x3396cc without the matrix: brake and hand brake on the wheel's spin, the steer angle and the rolling direction.
void setupWheel(WheelSpinState& wheel, const WheelGripSpec& spec, std::size_t index, const DrivetrainState& state, const std::array<Vector3, 3>& axes);

struct TyreContact {
    Vector3 point = {0, 0, 0};   // w+0xbc
    Vector3 normal = {0, 1, 0};  // w+0xc8
    float suspensionForce = 0;   // w+0xb4
};

// 0x339374 with 0x339538: the slip of the contact point against the wheel rim, limited by the friction circle, as a force at the contact
// point (mass * stiffness 4..8 * slip); the wheel spin takes a tenth of the force along the wheel over its inertia.
void applyTyreForce(VehicleRigidBody& body, WheelSpinState& wheel, const WheelGripSpec& spec, float radius, const TyreContact& contact,
                    float mass);

// 0x339a30 after the tyre force: grip recovery, the skid value for effects and the spin angle.
void spinWheel(WheelSpinState& wheel, const DrivetrainState& state, float dt);

// 0x33b604..0x33ba60: clutch, the gear rule of 0x33b73c, the engine 0x339db0 and the momentum exchange between engine and driven wheels.
void stepDrivetrain(DrivetrainState& state, const EngineSpec& engine, const std::vector<WheelGripSpec>& wheels, float forwardSpeed);

// 0x339db0: engine speed from the torque, the throttle and the friction; per update with the factor 1/60.
void stepEngine(DrivetrainState& state, const EngineSpec& engine, float forwardSpeed);

}
