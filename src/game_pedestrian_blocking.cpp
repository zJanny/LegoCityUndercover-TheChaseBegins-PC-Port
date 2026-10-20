#include "game_pedestrian_blocking.hpp"
#include "game_level.hpp"
#include "runtime/room_layers.hpp"

namespace game_pedestrian_blocking {

namespace {
runtime::PedestrianLineTest dodgeRay(const Blocking& blocking) {
    if (!blocking.collision) return nullptr;
    const runtime::CollisionWorld* collision = blocking.collision;
    return [collision](const runtime::PedestrianVec3& from, const runtime::PedestrianVec3& to) {
        return runtime::pedestrianDodgeRayBlocked(*collision, from, to);
    };
}
}

void wireSystemStep(runtime::PedestrianSystemStep& step, const Blocking* blocking, const runtime::PedestrianSystem& system,
                    runtime::GameRandom& random) {
    static const runtime::RoomLayerTable startup = runtime::startupRoomLayers();
    step.layers = startup;
    if (!blocking) {
        step.spawnBlocked = [](std::size_t, const runtime::PedestrianVec3&) { return false; };
        return;
    }
    step.spawnBlocked = [blocking, &system, &random](std::size_t slot, const runtime::PedestrianVec3& position) {
        return runtime::pedestrianSpawnBlocked(system.body(slot), system.slots()[slot].arrivalRadius, position, blocking->vehicles,
                                               dodgeRay(*blocking), random);
    };
}

void wireCharacterStep(runtime::PedestrianCharacterStep& step, const Blocking* blocking, const runtime::PedestrianSystem& system,
                       runtime::GameRandom& random) {
    if (!blocking) return;
    step.blocked = [blocking, &system, &random](std::size_t slot, runtime::PedestrianBody& body, const runtime::PedestrianVec3& position,
                                                 runtime::PedestrianVec3& velocity, float dt) {
        return runtime::pedestrianBlocked(body, system.slots()[slot].arrivalRadius, position, velocity, dt, blocking->vehicles,
                                          dodgeRay(*blocking), random);
    };
}

}

namespace game_level {

// STAND-IN PED-S9: the list 0x5863b4 + 0x305c is the vehicles that have a body in the port's vehicle world (VEH-S82), not wrecked and
// not disabled, in the port's vehicle order, taken at the pedestrian update instead of at the physics step 0x2e2bc8; the body flag
// +0xc4 bit 0 and the entity word +0x10 bit 5 are not read, and no listed body is kinematic.
game_pedestrian_blocking::Blocking LevelSession::pedestrianBlocking() const {
    game_pedestrian_blocking::Blocking blocking;
    blocking.collision = &collision;
    for (std::size_t i = 0; i < boardingVehicles.size(); ++i) {
        const VehicleDrive& drive = vehicleDrives[i];
        const chase::runtime::BoardingVehicle& vehicle = boardingVehicles[i];
        if (!drive.started || drive.wreck.active || objectState.isDisabled(vehicle.instance)) continue;
        blocking.vehicles.push_back(chase::runtime::pedestrianBlockVehicle(vehicle.matrix, {vehicle.boxHalf[0], vehicle.boxHalf[1], vehicle.boxHalf[2]},
                                                                           chase::runtime::forwardSpeed(drive.rigid), false));
    }
    return blocking;
}

}
