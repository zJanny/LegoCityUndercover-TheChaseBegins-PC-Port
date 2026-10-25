#pragma once
// The police trooper's vehicle record as the oracle holds it (Azahar, save state sitting in the police car of mission 1-1, body 0x08e16100 and rigid body 0x08e16f40,
// memtrace of 2026-10-15 in agent VEH32 trace tA frame 7 and agent VPHYS trace body0): class values after the loaders
// 0x243f90, 0x243e9c, 0x243c50, 0x243628 and 0x2435d0. Shared by the vehicle tests.
#include "runtime/vehicle.hpp"
#include "runtime/vehicle_box_contact.hpp"
#include "runtime/vehicle_wheels.hpp"

#include <optional>
#include <vector>

namespace trooper {

inline chase::runtime::VehicleWheel wheel(std::size_t index) {
    using namespace chase::runtime;
    const bool rear = index >= 2;
    const float side = index == 0 || index == 3 ? -1.0f : 1.0f;
    VehicleWheel wheel;
    wheel.suspension.mount = {0.85f * side, rear ? 0.067f : 0.0685f, rear ? -1.52f : 1.52f};
    wheel.suspension.sideOffset = (rear ? 0.1055f : 0.07825f) * side;
    wheel.suspension.radius = 0.48f;
    wheel.suspension.travel = 1.0f;
    wheel.suspension.stiffness = 18.2f;
    wheel.suspension.dampingExtend = rear ? 2.8f : 5.2f;
    wheel.suspension.dampingCompress = rear ? 2.1f : 3.9f;
    wheel.grip.driveRatio = rear ? 1.4f : 0.6f;
    wheel.grip.steered = !rear;
    wheel.grip.inertia = rear ? 0.351f : 0.35f;
    wheel.grip.handbrakeScale = rear ? 0.8f : 1.0f;
    wheel.grip.brakeForce = rear ? 15.0f : 12.0f;
    wheel.grip.brakeFriction = rear ? 1.0f : 3.0f;
    wheel.grip.frictionStatic = 1.6f;
    wheel.grip.frictionDynamic = 0.0f;
    wheel.grip.driftScale = rear ? 0.9f : 0.95f;
    return wheel;
}

// Oracle words of the trooper's probe points (save state sitting in the police car of mission 1-1, body+0xdc..+0x158): the box equals its entity box minus CentreOfMass.
inline void box(chase::runtime::Vector3& low, chase::runtime::Vector3& high) {
    low = {-0x1.f06dd8p-1f, 0x1.0a46p-8f, -0x1.6e0004p+1f};
    high = {0x1.f06dd8p-1f, 0x1.a7f832p+0f, 0x1.55999cp+1f};
}

inline chase::runtime::VehicleCollisionBody collisionBody(const chase::runtime::VehicleRecord& record) {
    chase::runtime::Vector3 low, high;
    box(low, high);
    return chase::runtime::vehicleCollisionBody(record, low, high, chase::runtime::kVehicleBodyMassFactor);
}

inline chase::runtime::VehicleBoxContacts boxContacts(const std::vector<chase::runtime::BulletBox>* boxes) {
    chase::runtime::Vector3 low, high;
    box(low, high);
    return chase::runtime::vehicleBoxContacts(low, high, boxes);
}

inline chase::runtime::VehicleRecord record() {
    using namespace chase::runtime;
    VehicleRecord record;
    record.className = "vehicles/vehicle_police_trooper";
    record.centreOfMass = {0.0f, -0.75f, 0.0f};
    record.topSpeed = 28.0f;
    record.dampingLinear = 0.02f;
    record.dampingAngular = 0.1f;
    record.suspensionScale = 1.0f;
    record.mass = 1300.0f;
    for (std::size_t i = 0; i < 4; ++i) record.wheels.push_back(wheel(i));
    SteeringSpec steering{0.2f, 0.5f, 0.4f, 0.8f, 0.1f, 4.7f};
    record.engine = makeEngineSpec(0.025f, 900.0f, 0.15f, 7500.0f, 6900.0f, 0.3f, record.topSpeed, 0.48f, 2600.0f, 0.2f, steering);
    return record;
}

// A flat street at height 0.
inline std::optional<chase::runtime::WheelHit> flatGround(const chase::runtime::Vector3& from, const chase::runtime::Vector3& to) {
    if (from[1] < 0.0f || to[1] >= 0.0f) return std::nullopt;
    const float t = from[1] / (from[1] - to[1]);
    return chase::runtime::WheelHit{{from[0] + (to[0] - from[0]) * t, 0.0f, from[2] + (to[2] - from[2]) * t}, {0.0f, 1.0f, 0.0f}};
}

}
