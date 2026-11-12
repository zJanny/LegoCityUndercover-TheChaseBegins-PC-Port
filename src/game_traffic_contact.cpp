#include "game_level.hpp"

#include "runtime/traffic_driver_states.hpp"
#include "runtime/vehicle_wheel_pose.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace game_level {
namespace runtime = chase::runtime;

namespace {
// STAND-IN TRAFFIC-S37: the game clock 0x2355c0 in seconds, from the update count at 30 updates per second.
constexpr float kTrafficClockSecondsPerUpdate = 1.0f / 30.0f;
}

float LevelSession::trafficClock() const { return float(updatesForLog) * kTrafficClockSecondsPerUpdate; }

void LevelSession::loadTrafficContact() {
    trafficContactBodies.assign(traffic.carCount(), TrafficContactBody{});
    trafficBodyInputs.assign(traffic.carCount(), runtime::BodyInputs{});
    trafficOfVehicle.assign(boardingVehicles.size(), std::nullopt);
    for (std::size_t car = 0; car < trafficBoarding.size(); ++car)
        if (trafficBoarding[car]) trafficOfVehicle[*trafficBoarding[car]] = car;
    trafficBodyWorld.copyInputs = [this](std::size_t car) {
        const std::optional<std::size_t> index = trafficBoarding[car];
        if (!index) return;
        const runtime::VehicleRecord& record = levelScripts.vehicles().records()[boardingRecords[*index]];
        const VehicleDrive& drive = vehicleDrives[*index];
        const runtime::Vector3& velocity = drive.rigid.velocity;
        const float speed = std::sqrt(velocity[0] * velocity[0] + velocity[1] * velocity[1] + velocity[2] * velocity[2]);
        trafficBodyInputs[car] = runtime::copyVehicleRecordInputs(traffic.driverVehicle(car), record.state, drive.drivetrain.gear, speed);
    };
}

bool LevelSession::trafficBodyDynamic(std::size_t car) const {
    const runtime::TrafficBodyState& body = trafficBodies.body(car);
    return (body.rigidFlags & (runtime::kRigidFlagKinematic | runtime::kRigidFlagOutOfWorld)) == 0 && (body.world & runtime::kTrafficBodyWanted) != 0;
}

void LevelSession::syncTrafficDriverFields(bool toDriver) {
    for (std::size_t car = 0; car < trafficBoarding.size(); ++car) {
        if (!trafficBoarding[car]) continue;
        runtime::VehicleRecord& record = *levelScripts.vehicles().mutableRecord(boardingRecords[*trafficBoarding[car]]);
        if (toDriver) runtime::driverFieldsFromRecord(traffic.driverVehicle(car), record);
        else runtime::recordFieldsFromDriver(record, traffic.driverVehicle(car));
    }
}

void LevelSession::addTrafficContactBodies(std::vector<runtime::VehicleWorldBody>& bodies, std::vector<runtime::VehicleRecord>& scaled,
                                           std::vector<std::optional<runtime::VehicleFloor>>& floors) {
    const chase::assets::CollisionItem* floorMesh = floorItem();
    if (!floorMesh || bodies.empty()) return;
    for (std::size_t car = 0; car < trafficBodies.size(); ++car) {
        const std::optional<std::size_t> index = trafficBoarding[car];
        if (!index) continue;
        VehicleDrive& drive = vehicleDrives[*index];
        const runtime::TrafficBodyState& state = trafficBodies.body(car);
        const runtime::BoardingVehicle& vehicle = boardingVehicles[*index];
        if (drive.started || (state.rigidFlags & (runtime::kRigidFlagKinematic | runtime::kRigidFlagOutOfWorld)) != runtime::kRigidFlagKinematic) continue;
        if (objectState.isDisabled(vehicle.instance)) continue;
        TrafficContactBody& contact = trafficContactBodies[car];
        const runtime::VehicleRecord& record = levelScripts.vehicles().records()[boardingRecords[*index]];
        if (!contact.shapeReady) {
            contact.shape = runtime::vehicleShapeBoxes(record, vehicle.boxCentre, vehicle.boxHalf);
            contact.boxContacts = runtime::vehicleBoxContacts(contact.shape.collisionLow, contact.shape.collisionHigh, &vehicleBoxes);
            contact.shapeReady = true;
        }
        contact.rigid = runtime::trafficKinematicRigidBody(state);
        contact.awake = true;
        scaled.push_back(runtime::vehicleRecordAtMassScale(record, 1.0f));
        floors.emplace_back(runtime::VehicleFloor{runtime::FloorMesh(*floorMesh), runtime::FloorTracker{},
                                                  runtime::vehicleCollisionBody(scaled.back(), contact.shape.collisionLow, contact.shape.collisionHigh, 1.0f), &contact.boxContacts});
        runtime::VehicleWorldBody body{*index, &contact.rigid, &contact.drivetrain, &contact.awake, &scaled.back(), runtime::BodyInputs{}, &*floors.back(), nullptr};
        body.controls.driver = false;
        body.kinematic = true;
        body.groundedWheels = state.wheelsOnGround;
        body.impact = [this, car](float, const runtime::ContactOther& other, const runtime::Vector3&) { trafficCarContact(car, other); };
        bodies.push_back(body);
    }
}

void LevelSession::trafficCarContact(std::size_t car, const runtime::ContactOther& other) {
    runtime::TrafficBodyState& body = trafficBodies.body(car);
    runtime::TrafficCarContactMessage message;
    message.otherEntity = other.entity;
    message.now = trafficClock();
    runtime::TrafficCarContactHooks hooks;
    // STAND-IN TRAFFIC-S37: the other entity's body flag +0xc4 bit 0x20000 (the choice between the queued and the immediate dynamic switch) is taken as clear.
    hooks.dynamicQueued = [&body] { body.queue.push_back(runtime::TrafficBodyCommand::Dynamic); };
    hooks.dynamicNow = [&body] { runtime::makeTrafficBodyDynamicNow(body, runtime::kVehicleBodyGravity); };
    const std::uint8_t requestedBefore = traffic.driverRecord(car).requestedState;
    traffic.carContact(car, (body.rigidFlags & runtime::kRigidFlagKinematic) != 0, message, hooks);
    if (requestedBefore != 4 && traffic.driverRecord(car).requestedState == 4)
        std::cout << "traffic car " << traffic.instance(car) << ": hit, driver requests state 4 at update " << updatesForLog << '\n';
}

void LevelSession::readTrafficBody(std::size_t car, runtime::TrafficCarBody& body) const {
    const std::size_t index = *trafficBoarding[car];
    const VehicleDrive& drive = vehicleDrives[index];
    const runtime::TrafficBodyState& state = trafficBodies.body(car);
    body.wheelCount = std::uint32_t(state.wheels.size());
    if (drive.started) {
        const runtime::Vector3& velocity = drive.rigid.velocity;
        body.forwardSpeed = runtime::forwardSpeed(drive.rigid);
        body.moveSpeed = std::sqrt(velocity[0] * velocity[0] + velocity[1] * velocity[1] + velocity[2] * velocity[2]);
        body.activationState = drive.awake ? runtime::kRigidActive : runtime::kRigidSleeping;
        body.groundedWheels = std::uint32_t(std::count_if(drive.wheels.begin(), drive.wheels.end(), [](const runtime::WheelState& wheel) { return wheel.contact; }));
        return;
    }
    body.forwardSpeed = state.forwardSpeed;
    body.moveSpeed = std::sqrt(state.velocity[0] * state.velocity[0] + state.velocity[1] * state.velocity[1] + state.velocity[2] * state.velocity[2]);
    body.activationState = state.activation;
    body.groundedWheels = std::uint32_t(state.wheelsOnGround);
}

bool LevelSession::poseTrafficWheel(std::size_t vehicleIndex, std::size_t wheel) {
    const std::optional<std::size_t> car = trafficCarOf(vehicleIndex);
    if (!car || vehicleDrives[vehicleIndex].started) return false;
    const runtime::TrafficBodyState& body = trafficBodies.body(*car);
    const std::size_t number = wheelNumbers[wheel];
    if (number >= body.wheels.size()) return false;
    const chase::assets::Matrix local = placementMatrix(wheelLocalPlacements[wheel]);
    scene.setTransform(wheelSceneIndex[wheel].second,
                       runtime::kinematicWheelPoseMatrix(body.wheels[number].spinAngle, body.wheels[number].mountX, {local[12], local[13], local[14]}, boardingVehicles[vehicleIndex].matrix));
    return true;
}

}
