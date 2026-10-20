#include "game_level.hpp"

#include <iostream>

namespace game_pedestrians {

std::vector<runtime::PedestrianRoomMove> LevelPedestrians::moveRooms(const runtime::StreamRooms& rooms, const std::map<int, runtime::RoomBox>& boxes,
                                                                     int currentRoom, int playerRoom, const assets::PlacedObjectState& state) {
    std::vector<runtime::PedestrianRoomCandidate> candidates;
    for (std::size_t i = 0; i < slots_.size(); ++i) {
        if (system_.room(i) == 0 || state.isDisabled(system_.slots()[i].instance)) continue;
        candidates.push_back({i, system_.room(i), system_.body(i).node[3], slots_[i].boundsCentre});
    }
    const std::vector<runtime::PedestrianRoomMove> moves = runtime::movePedestrianRooms(roomMover_, rooms, boxes, currentRoom, playerRoom, candidates);
    for (const runtime::PedestrianRoomMove& move : moves) system_.setRoom(move.slot, move.to);
    return moves;
}

}

namespace game_level {

void LevelSession::movePedestrianRooms() {
    const int current = roomStreamer->currentRoom();
    for (const chase::runtime::PedestrianRoomMove& move : pedestrians.moveRooms(roomStreamer->rooms(), roomStreamer->boxes(), current, current, objectState))
        std::cout << "pedestrian slot " << move.slot << ": room " << move.from << " -> " << move.to << " at update " << updatesForLog << '\n';
}

}
