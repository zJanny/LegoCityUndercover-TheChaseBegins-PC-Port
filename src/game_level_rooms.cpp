#include "game_level.hpp"
#include "qol/view_distance.hpp"
#include "runtime/fog.hpp"
#include "runtime/room_locator.hpp"
#include "runtime/scene_start.hpp"
#include <cstdlib>
#include <iostream>

namespace game_level {
using namespace chase::assets;
namespace runtime = chase::runtime;

namespace {
runtime::Vec3 viewDirection(float yaw, float pitch) {
    float forward[3], right[3];
    chase::render::cameraAxes(yaw, pitch, forward, right);
    return {forward[0], forward[1], forward[2]};
}

int roomOfModel(const std::string& path) {
    const std::size_t slash = path.find_last_of('/');
    const std::string file = path.substr(slash == std::string::npos ? 0 : slash + 1);
    if (file.compare(0, 4, "room") != 0 || file.size() <= 11 || file.compare(file.size() - 7, 7, ".bfnmdl") != 0) return -1;
    return runtime::roomNumber(file.substr(0, file.size() - 7));
}
}

void LevelSession::startRoomStreaming() {
    if (!player || !follow || currentWorldLevel.instance.empty()) return;
    const std::string folder = "models/" + levelName;
    roomStreamer = runtime::RoomStreamer::forWorldLevel(shared.master, currentWorldLevel.instance, shared.archive, folder);
    if (!roomStreamer) return;
    int startRoom = shared.options.startRoom.empty() ? -1 : runtime::roomNumber(shared.options.startRoom);
    const runtime::Vec3 position = player->position();
    if (startRoom < 0 && shared.options.start) startRoom = roomStreamer->roomAt(position);
    if (startRoom < 0)
        if (const std::optional<Placement> start = playerStart(level)) startRoom = runtime::roomNumber(start->room);
    if (!roomStreamer->rooms().count(startRoom)) startRoom = roomStreamer->roomAt(position);
    if (!roomStreamer->rooms().count(startRoom)) {
        roomStreamer.reset();
        return;
    }
    roomStreamer->settle(startRoom, position, viewDirection(follow->yaw(), follow->pitch()));
    std::size_t index = 0;
    for (const std::string& model : chase::render::levelModels(shared.archive, shared.options.level, "")) {
        const int room = roomOfModel(model);
        if (room > 0) roomInstances[room].push_back(index);
        ++index;
    }
    levelScripts.clearAllRoomsLoaded();
    std::set<std::string> sceneRooms;
    for (const auto& room : roomStreamer->rooms()) sceneRooms.insert("Room" + std::to_string(room.first));
    levelScripts.setSceneRooms(sceneRooms);
    showLoadedRooms();
    std::cout << "room streaming: " << roomStreamer->rooms().size() << " rooms, start Room" << startRoom << ", "
              << roomStreamer->loadedRooms().size() << " loaded\n";
}

void LevelSession::showLoadedRooms() {
    for (const auto& [room, instances] : roomInstances) {
        if (!roomStreamer->rooms().count(room)) continue;
        const bool loaded = roomStreamer->loaded(room) && room != shared.options.hideRoom;
        for (const std::size_t instance : instances) scene.setInstanceVisible(instance, loaded);
        levelScripts.setRoomLoaded("Room" + std::to_string(room), loaded);
    }
}

bool LevelSession::roomShown(int room) const {
    return !roomStreamer || !roomStreamer->rooms().count(room) || (roomStreamer->loaded(room) && room != shared.options.hideRoom);
}

void LevelSession::applyFog(const runtime::ClipPlanes& planes) const {
    const runtime::FogSetting fog = qol::withFogDistance(runtime::levelFog(fogObjects, roomStreamer ? roomStreamer->currentRoom() : 0));
    if (!fog.enabled) return;
    chase::render::setFog({runtime::buildFogLut(fog.start, fog.end, planes), fog.colour, planes.nearZ, planes.farZ});
}

// WSTREAM-S33: only vehicles change rooms, and their node box centre is the centre of the model bounds.
void LevelSession::moveVehicleRooms() {
    std::vector<runtime::MovingObject> objects;
    for (std::size_t i = 0; i < vehicleDrives.size(); ++i) {
        const VehicleDrive& drive = vehicleDrives[i];
        if (drive.placedIndex >= placedRooms.size()) continue;
        const chase::render::Mat4& matrix = boardingVehicles[i].matrix;
        objects.push_back({drive.placedIndex, {matrix[12] + drive.nodeBounds.centre[0], matrix[13] + drive.nodeBounds.centre[1], matrix[14] + drive.nodeBounds.centre[2]}});
    }
    const int current = roomStreamer->currentRoom();
    for (const runtime::ObjectRoomMove& move : vehicleRoomMover.update(roomStreamer->rooms(), roomStreamer->boxes(), current, current, objects, placedRooms))
        std::cout << "vehicle " << placedObjects[move.index].instance << ": room " << move.from << " -> " << move.to << " at update " << updatesForLog << '\n';
}

void LevelSession::updateRoomStreaming() {
    if (!roomStreamer || !player || !follow) return;
    const int before = roomStreamer->currentRoom();
    const std::set<int> loadedBefore = roomStreamer->loadedRooms();
    moveVehicleRooms();
    movePedestrianRooms();
    roomStreamer->update(player->position(), viewDirection(follow->yaw(), follow->pitch()), scriptedRooms);
    if (roomStreamer->currentRoom() != before) std::cout << "room " << before << " -> " << roomStreamer->currentRoom() << " at update " << updatesForLog << '\n';
    if (roomStreamer->loadedRooms() != loadedBefore) {
        showLoadedRooms();
        std::cout << "rooms loaded: " << roomStreamer->loadedRooms().size() << " at update " << updatesForLog << '\n';
        const runtime::Vec3 view = viewDirection(follow->yaw(), follow->pitch());
        const runtime::Vec3 eye = {follow->eye()[0], follow->eye()[1], follow->eye()[2]};
        for (const int room : roomStreamer->loadedRooms()) {
            if (loadedBefore.count(room) || !roomStreamer->boxes().count(room)) continue;
            const runtime::BoxApproach approach = runtime::approachToBox(roomStreamer->boxes().at(room), eye, view);
            std::cout << "room " << room << " loaded at update " << updatesForLog << ": nearest box point " << approach.distance
                      << " from the camera, depth " << approach.depth << '\n';
        }
    }
}

}
