#include "game_level.hpp"
#include "runtime/vehicle_wheel_pose.hpp"

#include <iostream>

namespace game_level {
namespace runtime = chase::runtime;

void LevelSession::applyVehicleDamage(std::size_t vehicleIndex, const runtime::DamageMessage& message, float speed) {
    runtime::VehicleRecord& record = *levelScripts.vehicles().mutableRecord(boardingRecords[vehicleIndex]);
    const runtime::DamageOutcome outcome = runtime::vehicleDamage(record, message, true);
    if (!outcome.heartLost) return;
    std::cout << "vehicle " << record.instance << ": heart lost (type " << unsigned(message.type) << ", " << speed << " m/s), " << unsigned(record.hearts)
              << " left" << (outcome.destroyed ? ", destroyed" : "") << '\n';
    if (hud) hud->setHearts(record.hearts, true);
    if (outcome.destroyed) breakVehicleNode(vehicleIndex);
}

chase::assets::Matrix LevelSession::wheelWorldMatrix(std::size_t wheelEntry) const {
    const std::size_t placedIndex = wheelSceneIndex[wheelEntry].first;
    for (std::size_t i = 0; i < vehicleDrives.size(); ++i) {
        const VehicleDrive& drive = vehicleDrives[i];
        if (drive.placedIndex != placedIndex) continue;
        const runtime::VehicleRecord& record = levelScripts.vehicles().records()[boardingRecords[i]];
        const std::size_t number = wheelNumbers[wheelEntry];
        if (number < drive.wheels.size() && number < drive.drivetrain.wheels.size())
            return runtime::wheelPoseMatrix(record.wheels[number].suspension, drive.drivetrain.wheels[number], number, drive.wheels[number].distance,
                                            runtime::bodyOriginMatrix(drive.rigid));
        return chase::assets::multiply(placementMatrix(wheelLocalPlacements[wheelEntry]), boardingVehicles[i].matrix);
    }
    return chase::assets::multiply(placementMatrix(wheelLocalPlacements[wheelEntry]), chase::assets::Matrix{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1});
}

void LevelSession::breakVehicleNode(std::size_t vehicleIndex) {
    VehicleDrive& drive = vehicleDrives[vehicleIndex];
    std::vector<chase::assets::Matrix> wheels(runtime::kWheelLaunchCount);
    std::size_t found = 0;
    for (std::size_t w = 0; w < wheelSceneIndex.size(); ++w)
        if (wheelSceneIndex[w].first == drive.placedIndex && wheelNumbers[w] < runtime::kWheelLaunchCount) {
            wheels[wheelNumbers[w]] = wheelWorldMatrix(w);
            ++found;
        }
    wheels.resize(found);
    drive.wreck = runtime::breakVehicle(boardingVehicles[vehicleIndex].matrix, wheels, breakRandomStandIn);
    drive.stuck = {};
    levelScripts.vehicles().requestState(boardingRecords[vehicleIndex], runtime::kVehicleStateDestroyed);
    std::cout << "vehicle " << boardingVehicles[vehicleIndex].instance << ": broken at update " << updatesForLog
              << " (stand-in VEH-S98: no effect 0x23aebc, no sound, no wheel fade 0x29cf70, wheel ray instead of 0x21ab20)\n";
}

bool LevelSession::stepVehicleWreck(std::size_t vehicleIndex, float dt) {
    VehicleDrive& drive = vehicleDrives[vehicleIndex];
    runtime::VehicleWreck& wreck = drive.wreck;
    if (wreck.returnDue) {
        returnVehicle(vehicleIndex);
        return false;
    }
    runtime::stepWreckTimer(wreck, dt);
    const runtime::WheelPieceSweep sweepStandIn = [this](const runtime::Vector3& from, const runtime::Vector3& to) -> std::optional<runtime::WheelPieceHit> {
        const std::optional<runtime::WheelHit> hit = runtime::castWheelRay(collision, from, to, [this](const runtime::CollisionWorld::Entry& entry) {
            return std::any_of(boardingVehicles.begin(), boardingVehicles.end(), [&](const runtime::BoardingVehicle& vehicle) { return vehicle.instance == entry.owner; });
        });
        if (!hit) return std::nullopt;
        return runtime::WheelPieceHit{hit->point, hit->normal};
    };
    for (std::size_t w = 0; w < wheelSceneIndex.size(); ++w) {
        const std::size_t number = wheelNumbers[w];
        if (wheelSceneIndex[w].first != drive.placedIndex || number >= wreck.wheels) continue;
        if (runtime::wheelPieceMoving(wreck.launch[number]))
            runtime::stepWheelPiece(wreck.wheelWorld[number], wheelBounds[w].first, wheelBounds[w].second, wreck.launch[number], sweepStandIn, breakRandomStandIn);
        scene.setTransform(wheelSceneIndex[w].second, wreck.wheelWorld[number]);
    }
    return true;
}

void LevelSession::returnVehicle(std::size_t vehicleIndex) {
    VehicleDrive& drive = vehicleDrives[vehicleIndex];
    runtime::BoardingVehicle& vehicle = boardingVehicles[vehicleIndex];
    runtime::VehicleRecord& record = *levelScripts.vehicles().mutableRecord(boardingRecords[vehicleIndex]);
    runtime::restoreVehicleRecord(record);
    drive.wreck = {};
    vehicle.matrix = drive.history.returnMatrix();
    drive.rigid = runtime::vehicleRigidBody(record, {vehicle.matrix[12], vehicle.matrix[13], vehicle.matrix[14]}, runtime::vehicleHeading(vehicle), drive.shape.shapeLow,
                                            drive.shape.shapeHigh, drive.massScale);
    drive.awake = true;
    drive.wheels.clear();
    drive.floorTracker = {};
    scene.setTransform(placedSceneIndex[drive.placedIndex], vehicle.matrix);
    poseVehicleWheels(vehicleIndex);
    const std::optional<std::size_t> seated = boarding ? boarding->seatedVehicle() : std::nullopt;
    if (seated && *seated == vehicleIndex) {
        if (hud) hud->setHearts(record.hearts, true);
        cameraSnapRequest = true;
    }
    std::cout << "vehicle " << vehicle.instance << ": back with " << unsigned(record.hearts) << " hearts at update " << updatesForLog
              << " (stand-in VEH-S99: no fade-in 0x252068, other vehicles not pushed away 0x231108, body at the heading only)\n";
}

void LevelSession::recordVehicleReturnPlace(std::size_t vehicleIndex) {
    VehicleDrive& drive = vehicleDrives[vehicleIndex];
    const runtime::VehicleRecord& record = levelScripts.vehicles().records()[boardingRecords[vehicleIndex]];
    if (record.state == runtime::kVehicleStateDestroyed || record.state == runtime::kVehicleStateDestroyedB || !drive.returnPlaceGood) return;
    drive.history.record(boardingVehicles[vehicleIndex].matrix);
}

bool LevelSession::wreckHides(std::size_t placedIndex, std::optional<std::size_t> wheelNumber) const {
    for (const VehicleDrive& drive : vehicleDrives)
        if (drive.placedIndex == placedIndex && drive.wreck.active) return !wheelNumber || *wheelNumber >= runtime::kWheelLaunchCount;
    return false;
}

}
