#pragma once
// The room move of walking pedestrians (pass 0x2e6ce4, docs/re/streaming-rooms.md): an enabled pedestrian sits in list 0 of its
// room (create 0x1913f8 stores 0 at +0x13, 0x242b08 buckets by it), so the pass tests it like every other object of the player's
// room and of the round-robin entry. No SDL.
#include "runtime/object_rooms.hpp"

#include <cstddef>
#include <vector>

namespace chase::runtime {

struct PedestrianRoomCandidate {
    std::size_t slot = 0;
    int room = 0;
    Vec3 nodePosition = {0, 0, 0};
    Vec3 boundsCentre = {0, 0, 0};
};

struct PedestrianRoomMove {
    std::size_t slot = 0;
    int from = 0;
    int to = 0;
};

// The test point of 0x2e6ce4: the node's world translation plus its local box centre (node +0xa0, added by 0x23f50c).
Vec3 pedestrianRoomTestPoint(const Vec3& nodePosition, const Vec3& boundsCentre);

// One update of the pass for the pedestrians that are enabled (candidates); the caller applies the returned moves.
// STAND-IN PED-S13: the entity bits 2 and 10 of +0x10 are taken as clear and the node flag bit 19 as set, so every enabled
// pedestrian is tested.
std::vector<PedestrianRoomMove> movePedestrianRooms(ObjectRoomMover& mover, const StreamRooms& rooms, const std::map<int, RoomBox>& boxes,
                                                    int currentRoom, int playerRoom, const std::vector<PedestrianRoomCandidate>& candidates);

}
