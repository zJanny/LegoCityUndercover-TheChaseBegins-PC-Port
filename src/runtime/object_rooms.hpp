#pragma once
// The rooms of moving level objects (0x2e6ce4, docs/re/streaming-rooms.md): every update the objects of two rooms are
// tested, the player's room and one entry of the current room's list in round-robin order; an object outside its room box
// moves to the room objectRoomAfterMove picks, and the moves are applied after all tests. No SDL.
#include "runtime/room_locator.hpp"

#include <cstddef>
#include <vector>

namespace chase::runtime {

struct MovingObject {
    std::size_t index = 0;
    Vec3 testPoint = {0, 0, 0};
};

struct ObjectRoomMove {
    std::size_t index = 0;
    int from = 0;
    int to = 0;
};

class ObjectRoomMover {
public:
    // WSTREAM-S32: the round-robin counter starts at entry 0 and steps by one entry after every update.
    std::vector<ObjectRoomMove> update(const StreamRooms& rooms, const std::map<int, RoomBox>& boxes, int currentRoom, int playerRoom,
                                       const std::vector<MovingObject>& objects, std::vector<int>& objectRooms);

private:
    std::size_t roundRobin_ = 0;
};

}
