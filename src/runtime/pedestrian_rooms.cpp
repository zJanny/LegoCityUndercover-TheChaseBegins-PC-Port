#include "runtime/pedestrian_rooms.hpp"

#include <algorithm>

namespace chase::runtime {

Vec3 pedestrianRoomTestPoint(const Vec3& nodePosition, const Vec3& boundsCentre) {
    return {nodePosition[0] + boundsCentre[0], nodePosition[1] + boundsCentre[1], nodePosition[2] + boundsCentre[2]};
}

std::vector<PedestrianRoomMove> movePedestrianRooms(ObjectRoomMover& mover, const StreamRooms& rooms, const std::map<int, RoomBox>& boxes,
                                                    int currentRoom, int playerRoom, const std::vector<PedestrianRoomCandidate>& candidates) {
    std::size_t slots = 0;
    for (const PedestrianRoomCandidate& candidate : candidates) slots = std::max(slots, candidate.slot + 1);
    std::vector<int> objectRooms(slots, 0);
    std::vector<MovingObject> objects;
    for (const PedestrianRoomCandidate& candidate : candidates) {
        objectRooms[candidate.slot] = candidate.room;
        objects.push_back({candidate.slot, pedestrianRoomTestPoint(candidate.nodePosition, candidate.boundsCentre)});
    }
    std::vector<PedestrianRoomMove> moves;
    for (const ObjectRoomMove& move : mover.update(rooms, boxes, currentRoom, playerRoom, objects, objectRooms))
        moves.push_back({move.index, move.from, move.to});
    return moves;
}

}
