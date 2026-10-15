#include "runtime/vehicle_drivetrain.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace chase::runtime {
namespace {

constexpr float kRpmToRadians = 0.10471975f;
constexpr float kRadiansToRpm = 9.549297f;
constexpr float kPi = 3.14159250f;
constexpr float kTwoPi = 6.28318500f;

float dot(const Vector3& a, const Vector3& b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
Vector3 scaled(const Vector3& v, float s) { return {v[0] * s, v[1] * s, v[2] * s}; }
Vector3 minus(const Vector3& a, const Vector3& b) { return {a[0] - b[0], a[1] - b[1], a[2] - b[2]}; }
Vector3 plus(const Vector3& a, const Vector3& b) { return {a[0] + b[0], a[1] + b[1], a[2] + b[2]}; }
float length(const Vector3& v) { return std::sqrt(dot(v, v)); }
Vector3 normalised(const Vector3& v) {
    const float l = length(v);
    return l > 0.0f ? scaled(v, 1.0f / l) : v;
}
std::int32_t bits(float value) {
    std::int32_t result;
    std::memcpy(&result, &value, sizeof result);
    return result;
}
std::uint32_t unsignedBits(float value) { return std::uint32_t(bits(value)); }

}

float engineFinalDrive(float maxRpm, float topSpeed, float wheelRadius) {
    return maxRpm / (std::fabs(kGearRatios[kGearCount - 1] * kGearRatioScale) * topSpeed) * kRpmToRadians * wheelRadius;
}

EngineSpec makeEngineSpec(float friction, float idleRpm, float inertia, float maxRpm, float redline, float power, float topSpeed,
                          float wheelRadius, float mass, float slowdown, const SteeringSpec& steering) {
    EngineSpec engine;
    for (std::size_t i = 0; i < engine.curve.size(); ++i) engine.curve[i] = kEngineTorqueCurve[i] + friction * (float(i) * 500.0f);
    engine.redline = redline;
    engine.maxRpm = maxRpm;
    engine.idleRpm = idleRpm;
    engine.inertia = inertia;
    engine.friction = friction;
    engine.power = power;
    engine.finalDrive = engineFinalDrive(maxRpm, topSpeed, wheelRadius);
    engine.mass = mass;
    engine.slowdown = slowdown;
    engine.steering = steering;
    return engine;
}

float engineTorque(const EngineSpec& engine, float rpm) {
    if (rpm < 0.0f) return 0.0f;
    const int whole = int(rpm);
    const int index = whole / 500;
    const float t = float(whole - index * 500) * 0.002f;
    const float before = index > 0 ? engine.curve[std::size_t(index - 1)] : 0.0f;
    const float at = engine.curve[std::size_t(index)];
    const float next = index < 31 ? engine.curve[std::size_t(index + 1)] : 0.0f;
    const float after = index < 30 ? engine.curve[std::size_t(index + 2)] : 0.0f;
    const float a = after - next - before + at;
    const float b = before - at - a;
    const float c = next - before;
    const float t2 = t * t;
    return (a * t * t2 + b * t2 + c * t + at) * engine.power;
}

BodyInputs bodyInputs(const DriveCommand& command) {
    return {command.steer, command.accelerate ? 1.0f : 0.0f, command.brake ? 1.0f : 0.0f, command.handBrake ? 1.0f : 0.0f};
}

void steerVehicle(DrivetrainState& state, const SteeringSpec& steering, float input, const Vector3& forward, const Vector3& velocity) {
    float steer = input;
    float speed = dot(velocity, forward);
    if (std::fabs(steer) > 0.5f) {
        if (state.steerFlick * steer < 0.0f) state.steerFlick = 0.0f;
        state.steerFlick += steer;
        state.steerFlickUpdates = -1;
    } else {
        int updates = state.steerFlickUpdates;
        if (updates < 0) {
            float flick = state.steerFlick * 0.25f;
            if (bits(flick) >= bits(0.8f)) flick = 0.8f;
            else if (unsignedBits(flick) >= unsignedBits(-0.8f)) flick = -0.8f;
            state.steerFlick = flick;
            updates = 1;
        }
        if (updates > 0) {
            state.steerFlickUpdates = updates - 1;
            steer = -state.steerFlick;
            state.steerFlick *= 0.9f;
        }
    }
    float lock = 1.0f;
    if (bits(speed) > bits(88.888885f)) speed = 88.888885f;
    if (!(speed < 0.0f)) lock = steering.lockMin + std::pow(1.0f - speed * 0.01125f, steering.lockPower) * (1.0f - steering.lockMin);
    float drift = 0.0f;
    if (bits(speed) >= bits(0.5f)) {
        float difference = std::atan2(-forward[0], -forward[2]) - std::atan2(-velocity[0], -velocity[2]);
        if (bits(kPi) < bits(difference)) difference -= kTwoPi;
        else if (unsignedBits(difference) > unsignedBits(-kPi)) difference += kTwoPi;
        drift = difference * 1.1f / steering.lockMax;
    }
    float current = -state.steerAngle / steering.lockMax;
    float out;
    if (std::fabs(steer) > 0.02f) {
        if (steer < 0.0f) {
            if (drift > 0.0f) lock = std::max(lock, drift * 0.9f);
            if (bits(lock) > bits(1.2f)) lock = 1.2f;
            current += lock * (current < 0.0f ? steering.speedNeg : steering.speed);
            if (current > lock) current = lock;
            out = -(current * steer);
        } else {
            if (drift < 0.0f) lock = std::max(lock, drift * -0.9f);
            if (bits(lock) > bits(1.2f)) lock = 1.2f;
            current -= lock * (current > 0.0f ? steering.speedNeg : steering.speed);
            if (current < -lock) current = -lock;
            out = current * steer;
        }
    } else {
        const float rate = steering.speedCentre * std::max(std::fabs(current), lock);
        out = current > 0.0f ? std::max(current - rate, -0.0f) : std::min(current + rate, 0.0f);
    }
    state.steerAngle = -(out * steering.lockMax);
}

bool controlDrivetrain(DrivetrainState& state, const EngineSpec& engine, const BodyInputs& controls, float forwardSpeed, float speedSquared) {
    state.throttle = 0.0f;
    state.brakeOut = 0.0f;
    state.handBrakeOut = 0.0f;
    float brake = controls.brake;
    if (!controls.driver && bits(forwardSpeed) > bits(5.0f)) brake = 0.7f;
    if (brake != 0.0f) {
        if (bits(forwardSpeed) > bits(6.0f) && state.gear > kGearNeutral) {
            if (std::fabs(state.steerAngle) > 0.2f && engine.steering.lockMax > 0.0f) {
                state.handBrakeOut = 1.0f;
            } else {
                state.brakeOut = brake;
                if (bits(forwardSpeed) < bits(1.5f)) state.gear = kGearReverse;
            }
        } else if (state.gear < kGearNeutral) {
            state.throttle = 1.0f;
            state.rpm = std::max(state.rpm, engine.idleRpm);
        } else if (state.gear > kGearNeutral) {
            state.brakeOut = brake;
            if (bits(forwardSpeed) < bits(1.5f)) state.gear = kGearReverse;
        }
    } else if (controls.accelerate == 0.0f) {
        state.brakeOut = speedSquared >= 4.0f ? engine.slowdown : 1.0f;
    } else if (state.gear > kGearNeutral) {
        state.throttle = controls.accelerate;
    } else if (state.gear < kGearNeutral) {
        state.brakeOut = 0.5f;
        if (unsignedBits(forwardSpeed) < unsignedBits(-3.5f)) state.gear = kGearFirst;
    }
    bool withinLimit = true;
    if (std::fabs(forwardSpeed) > engine.speedLimit) {
        state.throttle = 0.0f;
        withinLimit = false;
    }
    if (controls.handBrake != 0.0f) state.handBrakeOut = 1.0f;
    return withinLimit;
}

bool vehicleWakes(const BodyInputs& controls, float forwardSpeed, float speedSquared, float previousSteer) {
    return !(bits(forwardSpeed) < bits(0.5f) && bits(speedSquared) < bits(1.5f) && controls.accelerate == 0.0f && controls.brake == 0.0f &&
             previousSteer == controls.steer);
}

void sleepVehicle(DrivetrainState& state, const BodyInputs& controls) {
    state.previousSteer = controls.steer;
    for (WheelSpinState& wheel : state.wheels) wheel.rate = 0.0f;
}

void setupWheel(WheelSpinState& wheel, const WheelGripSpec& spec, std::size_t index, const DrivetrainState& state, const std::array<Vector3, 3>& axes) {
    wheel.handBrake = state.handBrakeOut;
    wheel.brake = state.brakeOut;
    if (index > 1 && state.handBrakeOut != 0.0f) wheel.rate = 0.0f;
    if (state.brakeOut != 0.0f) {
        if (wheel.rate > 0.0f) wheel.rate = std::max(wheel.rate - state.brakeOut * spec.brakeForce, 0.0f);
        else wheel.rate = std::min(wheel.rate + state.brakeOut * spec.brakeForce, 0.0f);
    }
    wheel.steerAngle = spec.steered ? spec.steerFactor * state.steerAngle : 0.0f;
    const float s = std::sin(wheel.steerAngle), c = std::cos(wheel.steerAngle);
    wheel.forward = plus(scaled(axes[0], s), scaled(axes[2], c));
}

void applyTyreForce(VehicleRigidBody& body, WheelSpinState& wheel, const WheelGripSpec& spec, float radius, const TyreContact& contact,
                    float mass) {
    if (!(bits(contact.suspensionForce) > 0x38d1b716) || !(std::fabs(contact.normal[1]) > 0.1f)) return;
    const Vector3& normal = contact.normal;
    const Vector3 rolling = normalised(minus(wheel.forward, scaled(normal, dot(wheel.forward, normal))));
    const Vector3 rim = scaled(rolling, wheel.rate * radius);
    Vector3 slip = minus(pointVelocity(body, contact.point), rim);
    slip = minus(slip, scaled(normal, dot(slip, normal)));
    slip = scaled(slip, -1.0f);
    const float load = std::min(contact.suspensionForce * body.inverseMass, 7.0f);

    float surface = kTyreSurfaceFriction;
    if (bits(normal[1]) < bits(0.8f)) surface *= 0.1f;
    const float slipSpeed = length(slip);
    const float brakes = (spec.brakeFriction + (1.0f - spec.brakeFriction) * (1.0f - wheel.brake)) *
                         (spec.handbrakeScale + (1.0f - spec.handbrakeScale) * (1.0f - wheel.handBrake));
    const float limit = spec.frictionStatic * wheel.grip * brakes * load * surface;
    Vector3 grip;
    if (slipSpeed <= limit) {
        grip = slip;
        wheel.skid = limit == 0.0f ? 0.0f : slipSpeed / limit;
    } else {
        const float t = std::clamp((slipSpeed - limit) * 0.01f, 0.0f, 1.0f);
        const float sliding = spec.frictionDynamic * load * brakes * surface * wheel.grip;
        grip = scaled(normalised(slip), limit + (sliding - limit) * t);
        wheel.skid = 1.0f;
    }
    const Vector3 along = scaled(rolling, dot(grip, rolling));
    grip = plus(scaled(minus(grip, along), spec.driftScale), along);

    float stiffness = (2.0f - slipSpeed) * 2.0f + 4.0f;
    if (bits(stiffness) >= bits(8.0f)) stiffness = 8.0f;
    else if (bits(stiffness) < 0x40800001) stiffness = 4.0f;
    addForceAtPoint(body, scaled(grip, mass * stiffness), contact.point);
    wheel.rate += (-dot(grip, rolling) / spec.inertia) * 0.1f;
}

void spinWheel(WheelSpinState& wheel, const DrivetrainState& state, float dt) {
    if (bits(wheel.grip) < bits(1.0f)) wheel.grip = std::min(wheel.grip + kTyreGripRecovery * dt, 1.0f);
    if (state.steerAngle == 0.0f) {
        if (state.brakeOut != 0.0f && bits(wheel.rate) > bits(62.831856f)) wheel.skid += state.brakeOut;
        else if (state.brakeOut != 0.0f) wheel.skid += wheel.rate * (state.brakeOut * 0.015915494f);
        else if (state.throttle != 0.0f && bits(18.849556f) > bits(wheel.rate)) wheel.skid += state.throttle;
        else if (state.throttle != 0.0f && bits(37.699112f) > bits(wheel.rate))
            wheel.skid += state.throttle * (1.0f - (wheel.rate - 18.849556f) * 0.053051647f);
    }
    wheel.angle += wheel.rate * dt;
    if (std::isnan(wheel.angle)) wheel.angle = 0.0f;
}

void stepEngine(DrivetrainState& state, const EngineSpec& engine, float forwardSpeed) {
    const float throttle = engine.minimumThrottle + (1.0f - engine.minimumThrottle) * state.throttle;
    const float omega = state.rpm * kRpmToRadians;
    const float torque = engineTorque(engine, state.rpm) * state.clutchScale;
    float change = throttle * 0.016666668f * torque - (0.1f + engine.friction * omega) * 0.016666668f;
    if (bits(forwardSpeed) < bits(20.0f) && 0.0f < forwardSpeed) change *= 1.0f + (20.0f - forwardSpeed) * 0.05f;
    float rpm = state.rpm + change / engine.inertia * kRadiansToRpm;
    if (rpm <= 0.0f) rpm = 0.0f;
    if (rpm <= engine.idleRpm) rpm = engine.idleRpm;
    if (engine.maxRpm < rpm) rpm = engine.maxRpm;
    state.rpm = rpm;
}

void stepDrivetrain(DrivetrainState& state, const EngineSpec& engine, const std::vector<WheelGripSpec>& wheels, float forwardSpeed) {
    if (state.gear == kGearNeutral) {
        state.clutch = 0.0f;
        state.gear = kGearTop;
    } else {
        if (state.rpm >= 3500.0f || state.throttle != 0.0f) state.clutch += 0.4f;
        else state.clutch = std::max(state.clutch - 0.05f, 0.0f);
        state.clutch = std::min(state.clutch, 1.0f);
        if (state.gear > kGearReverse) state.gear = kGearTop;
    }
    stepEngine(state, engine, forwardSpeed);
    if (wheels.empty()) return;
    const bool reverse = state.gear == kGearReverse;
    const float ratio = kGearRatios[std::size_t(state.gear)] * engine.finalDrive * kGearRatioScale / state.clutchScale;
    const float gearing = std::fabs(ratio);
    float engineMomentum = state.rpm * engine.inertia * kRpmToRadians;
    if (reverse) engineMomentum = -engineMomentum;
    float wheelMomentum = 0.0f, wheelInertia = 0.0f;
    for (std::size_t i = 0; i < wheels.size(); ++i) {
        wheelMomentum += state.wheels[i].rate * wheels[i].inertia * (wheels[i].driveRatio * 0.5f);
        wheelInertia += 0.5f * (wheels[i].inertia * wheels[i].driveRatio);
    }
    float omega = gearing * (engineMomentum + wheelMomentum) / (wheelInertia + engine.inertia * gearing);
    float transfer = engine.inertia * omega - engineMomentum;
    const float limit = state.clutch * (kClutchScale * kClutchTorque);
    if (std::fabs(transfer) > limit) {
        const float excess = transfer < 0.0f ? -(std::fabs(transfer) - limit) : std::fabs(transfer) - limit;
        transfer = transfer < 0.0f ? -limit : limit;
        omega -= excess / engine.inertia;
    }
    if (reverse) omega = -omega;
    state.rpm = omega * kRadiansToRpm;
    for (std::size_t i = 0; i < wheels.size(); ++i) state.wheels[i].rate -= wheels[i].driveRatio * (transfer * 0.5f) / wheels[i].inertia;
    state.clutchScale = 1.0f;
}

}
