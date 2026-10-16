#pragma once
#include "runtime/room_streaming.hpp"

#include <map>

namespace chase::runtime {

constexpr float kRoomTestLift = 0.1f;

// WSTREAM-S31: the player node's local box centre (node +0xa0, read once in the oracle); the room test uses the world
// position plus this offset.
constexpr Vec3 kPlayerBoxCentre = {0.0f, 0.68752300739f, 0.139908999f};

bool roomContains(const RoomBox& box, const Vec3& point);

// Distance from a camera position to the nearest point of the room's vertex box and that point's depth along the view
// direction (RENDER-31 measurement).
struct BoxApproach {
    float distance = 0.0f;
    float depth = 0.0f;
};

BoxApproach approachToBox(const RoomBox& box, const Vec3& eye, const Vec3& view);

// The room of a level object after one update's move (0x2e6ce4 -> 0x237b04 -> 0x22d204): the object's own room is kept
// while it contains the lifted test point (node world translation plus node box centre); otherwise the current room and
// then its list, then the first room in number order that contains it, then the room whose x/z range contains the point
// with the smallest vertical gap; with none of these the own room is kept.
int objectRoomAfterMove(const StreamRooms& rooms, const std::map<int, RoomBox>& boxes, int ownRoom, int currentRoom, const Vec3& testPoint);

// The room the player is in after one update's move (0x2e6ce4 -> 0x237b04 -> 0x22d204): the room is kept while it still
// contains the lifted point; otherwise the first room of the current room's list, then the first room in number order
// that contains it, then the room whose x/z range contains the point with the smallest vertical gap.
int roomAfterMove(const StreamRooms& rooms, const std::map<int, RoomBox>& boxes, int currentRoom, const Vec3& playerPosition);

}
