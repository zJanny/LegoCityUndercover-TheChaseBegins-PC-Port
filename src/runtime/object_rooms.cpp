#include "runtime/object_rooms.hpp"

namespace chase::runtime {

std::vector<ObjectRoomMove> ObjectRoomMover::update(const StreamRooms& rooms, const std::map<int, RoomBox>& boxes, int currentRoom, int playerRoom,
                                                    const std::vector<MovingObject>& objects, std::vector<int>& objectRooms) {
    std::vector<int> list{currentRoom};
    const auto current = rooms.find(currentRoom);
    if (current != rooms.end()) list.insert(list.end(), current->second.neighbours.begin(), current->second.neighbours.end());
    const int visited = list[roundRobin_ % list.size()];
    ++roundRobin_;
    std::vector<ObjectRoomMove> moves;
    for (const MovingObject& object : objects) {
        const int own = objectRooms[object.index];
        if (own != visited && own != playerRoom) continue;
        const int next = objectRoomAfterMove(rooms, boxes, own, currentRoom, object.testPoint);
        if (next != own) moves.push_back({object.index, own, next});
    }
    for (const ObjectRoomMove& move : moves) objectRooms[move.index] = move.to;
    return moves;
}

}
