#include "game_level.hpp"

namespace game_level {
namespace runtime = chase::runtime;

namespace {
constexpr std::uint16_t kDrivingState = 0x11c;
constexpr std::uint8_t kVehicleObjectType = 183;
}

void LevelSession::loadVehicleCameras() {
    vehicleCameras.clear();
    const std::vector<runtime::VehicleRecord>& records = levelScripts.vehicles().records();
    for (std::size_t i = 0; i < boardingVehicles.size(); ++i) {
        const auto placement = std::find_if(drawable.objects.begin(), drawable.objects.end(), [&](const chase::assets::Placement& p) {
            return p.instance == records[boardingRecords[i]].instance;
        });
        vehicleCameras.push_back(runtime::vehicleCameraSubject(
            chase::assets::parseModel(shared.archive.read(*shared.archive.find(chase::assets::meshModelPath(placement->mesh)))), records[boardingRecords[i]]));
    }
}

void LevelSession::selectCameraSubject(runtime::FollowCameraInput& input) {
    const std::optional<std::size_t> seated = boarding ? boarding->seatedVehicle() : std::nullopt;
    if (seated) {
        const runtime::BoardingVehicle& vehicle = boardingVehicles[*seated];
        input.target[0] = vehicle.matrix[12];
        input.target[1] = vehicle.matrix[13];
        input.target[2] = vehicle.matrix[14];
        input.targetFacing = runtime::vehicleHeading(vehicle);
        input.stickTurn = runtime::stickTurnInput(pad.slot(kSlotStickX).value, kDrivingState, kVehicleObjectType);
    }
    if (seated == cameraVehicle) return;
    cameraVehicle = seated;
    const runtime::VehicleCameraSubject subject = seated ? vehicleCameras[*seated] : runtime::onFootCameraSubject();
    follow->setSubject(subject.preset, subject.bodyHeight, subject.modelHalfHeight);
    follow->setExcludedOwner(seated ? boardingVehicles[*seated].instance : std::string());
}

}
