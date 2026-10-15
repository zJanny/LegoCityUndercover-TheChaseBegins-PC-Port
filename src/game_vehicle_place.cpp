#include "game_level.hpp"

namespace game_level {
namespace runtime = chase::runtime;

bool LevelSession::placeVehicle(const std::string& instance, const chase::assets::Placement& target) {
    for (std::size_t i = 0; i < boardingVehicles.size(); ++i) {
        runtime::BoardingVehicle& vehicle = boardingVehicles[i];
        if (vehicle.instance != instance) continue;
        VehicleDrive& drive = vehicleDrives[i];
        vehicle.matrix = placementMatrix(target);
        drive.body.position = {vehicle.matrix[12], vehicle.matrix[13], vehicle.matrix[14]};
        drive.body.heading = runtime::vehicleHeading(vehicle);
        drive.rigid = runtime::vehicleRigidBody(levelScripts.vehicles().records()[boardingRecords[i]], drive.body.position, drive.body.heading);
        scene.setTransform(placedSceneIndex[drive.placedIndex], vehicle.matrix);
        for (std::size_t w = 0; w < wheelSceneIndex.size(); ++w)
            if (wheelSceneIndex[w].first == drive.placedIndex)
                scene.setTransform(wheelSceneIndex[w].second, chase::assets::multiply(placementMatrix(wheelLocalPlacements[w]), vehicle.matrix));
        return true;
    }
    return false;
}

}
