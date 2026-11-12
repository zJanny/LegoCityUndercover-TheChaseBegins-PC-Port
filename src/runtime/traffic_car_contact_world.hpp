#pragma once
// A traffic car in the vehicle world step (docs/re/traffic-car-body.md): the kinematic body as a rigid body for the contacts with the dynamic
// bodies, and the driver's part of the contact message 0x80000002 (0x1797c4 at 0x179cec..0x179db0) that makes a hit car dynamic. No SDL.
#include "runtime/ai_driver_input.hpp"
#include "runtime/traffic_car_body.hpp"
#include "runtime/vehicle.hpp"
#include "runtime/vehicle_body.hpp"

#include <array>
#include <cstdint>
#include <functional>

namespace chase::runtime {

// The rotation whose columns are the body axes (right, up, forward) as the quaternion of a rigid body (the rows of the node matrix are the
// axes; 0x1eb950 converts the transposed rows, the world keeps the other convention).
std::array<float, 4> orientationFromAxes(const std::array<Vector3, 3>& axes);

// The rigid body of a kinematic traffic body for the contact solver: the centre of mass and the axes of rigid +4..+0x40, the velocity the
// route mover stored (0x229920, rigid +0x13c), no spin, no inverse mass and no inverse inertia (0x1f3124 with mass 0, command 4).
// STAND-IN TRAFFIC-S37: the solver uses the stored velocity as the body's velocity; Bullet's own kinematic velocity (saveKinematicState, the
// difference of the transform and the interpolation transform that 0x2429fc writes equal) is not read.
VehicleRigidBody trafficKinematicRigidBody(const TrafficBodyState& body);

struct TrafficCarContactMessage {
    bool otherEntity = false;           // the message's other entity (data +0) is not 0; a floor or a static box has none
    float now = 0.0f;                   // 0x2355c0()
    bool otherBodyFlag20000 = false;    // the other entity's body +0xc4 bit 0x20000
};

struct TrafficCarContactHooks {
    std::function<void()> dynamicNow;                          // 0x2e2f50(world, rigid): the body dynamic at once
    std::function<void()> dynamicQueued;                       // 0x23f0fc(body): command 3 of the body queue
    std::function<void(std::uint32_t signal)> leaveSignalQueue;  // 0x22a080(driver +0, entity)
};

// The branch of 0x1797c4 for message 0x80000002 of a vehicle that has a driver record, as far as the driver is concerned (not the entity of
// the vehicle the player drives, whose tail 0x179db4 is vehicleContactReply). In driver state 2 or 3: the request +0x79 = 4; with another
// entity and a kinematic rigid body the body goes dynamic (at once when the other entity's body +0xc4 has bit 0x20000, else queued); a
// car with +0xaf bit 3 leaves its junction's queue. With another entity, in every state: +0xa8 = now.
void trafficCarContact(AiDriverRecord& driver, bool rigidKinematic, const TrafficCarContactMessage& message, const TrafficCarContactHooks& hooks);

// 0x2e2f50 for a body: the Dynamic command of 0x2e3390 at once, past the queue.
void makeTrafficBodyDynamicNow(TrafficBodyState& body, float globalGravity);

// The fields the driver reads and writes in the vehicle record (+0x30c health points, +0x30d hearts, +0x30e the table row's hearts, +0x310
// the forced stop) against the port's vehicle record, where they have other names. The driver keeps its own copy (AiDriverVehicle); the port
// copies the record's fields in before the driver runs and the ones the driver writes back after it. The steering and pedal inputs
// (+0x288..+0x294) are consumed by copyVehicleRecordInputs and stay in the driver's copy.
void driverFieldsFromRecord(AiDriverVehicle& driver, const VehicleRecord& record);
void recordFieldsFromDriver(VehicleRecord& record, const AiDriverVehicle& driver);

}
