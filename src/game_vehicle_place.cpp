#include "game_level.hpp"

#include <algorithm>

namespace game_level {
namespace runtime = chase::runtime;

bool LevelSession::placeVehicle(const std::string& instance, const chase::assets::Placement& target) {
    for (std::size_t i = 0; i < boardingVehicles.size(); ++i) {
        if (boardingVehicles[i].instance != instance) continue;
        placeVehicleAt(i, placementMatrix(target));
        return true;
    }
    return false;
}

void LevelSession::placeVehicleAt(std::size_t index, const chase::render::Mat4& matrix) {
    runtime::BoardingVehicle& vehicle = boardingVehicles[index];
    VehicleDrive& drive = vehicleDrives[index];
    vehicle.matrix = matrix;
    const runtime::VehicleRecord& record = levelScripts.vehicles().records()[boardingRecords[index]];
    if (!drive.started) drive.shape = runtime::vehicleShapeBoxes(record, vehicle.boxCentre, vehicle.boxHalf);
    drive.rigid = runtime::vehicleRigidBody(record, {vehicle.matrix[12], vehicle.matrix[13], vehicle.matrix[14]}, runtime::vehicleHeading(vehicle),
                                            drive.shape.shapeLow, drive.shape.shapeHigh, drive.massScale);
    scene.setTransform(placedSceneIndex[drive.placedIndex], vehicle.matrix);
    drive.wheels.clear();
    poseVehicleWheels(index);
}

bool LevelSession::placeSeatedVehicle(const float position[3], std::optional<float> facingDegrees) {
    const std::optional<std::size_t> seated = boarding ? boarding->seatedVehicle() : std::nullopt;
    if (!seated) return false;
    const std::string& instance = levelScripts.vehicles().records()[boardingRecords[*seated]].instance;
    const auto placed = std::find_if(drawable.objects.begin(), drawable.objects.end(), [&](const chase::assets::Placement& p) { return p.instance == instance; });
    if (placed == drawable.objects.end()) return false;
    chase::assets::Placement target = *placed;
    std::copy(position, position + 3, target.position);
    if (facingDegrees) target.orientation[1] = *facingDegrees;
    vehicleDrives[*seated].floorTracker = {};
    return placeVehicle(boardingVehicles[*seated].instance, target);
}

}
