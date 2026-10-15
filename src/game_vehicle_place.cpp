#include "game_level.hpp"

#include <algorithm>

namespace game_level {
namespace runtime = chase::runtime;

bool LevelSession::placeVehicle(const std::string& instance, const chase::assets::Placement& target) {
    for (std::size_t i = 0; i < boardingVehicles.size(); ++i) {
        runtime::BoardingVehicle& vehicle = boardingVehicles[i];
        if (vehicle.instance != instance) continue;
        VehicleDrive& drive = vehicleDrives[i];
        vehicle.matrix = placementMatrix(target);
        drive.rigid = runtime::vehicleRigidBody(levelScripts.vehicles().records()[boardingRecords[i]], {vehicle.matrix[12], vehicle.matrix[13], vehicle.matrix[14]},
                                                runtime::vehicleHeading(vehicle));
        scene.setTransform(placedSceneIndex[drive.placedIndex], vehicle.matrix);
        for (std::size_t w = 0; w < wheelSceneIndex.size(); ++w)
            if (wheelSceneIndex[w].first == drive.placedIndex)
                scene.setTransform(wheelSceneIndex[w].second, chase::assets::multiply(placementMatrix(wheelLocalPlacements[w]), vehicle.matrix));
        return true;
    }
    return false;
}

bool LevelSession::placeSeatedVehicle(const float position[3]) {
    const std::optional<std::size_t> seated = boarding ? boarding->seatedVehicle() : std::nullopt;
    if (!seated) return false;
    const std::string& instance = levelScripts.vehicles().records()[boardingRecords[*seated]].instance;
    const auto placed = std::find_if(drawable.objects.begin(), drawable.objects.end(), [&](const chase::assets::Placement& p) { return p.instance == instance; });
    if (placed == drawable.objects.end()) return false;
    chase::assets::Placement target = *placed;
    std::copy(position, position + 3, target.position);
    return placeVehicle(boardingVehicles[*seated].instance, target);
}

}
