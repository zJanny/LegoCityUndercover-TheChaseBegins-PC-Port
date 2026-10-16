#include "runtime/room_locator.hpp"

#include <algorithm>
#include <cmath>

namespace chase::runtime {
namespace {
bool inside(const std::map<int, RoomBox>& boxes, int room, const Vec3& point) {
    const auto found = boxes.find(room);
    return found != boxes.end() && roomContains(found->second, point);
}
}

bool roomContains(const RoomBox& box, const Vec3& point) {
    const Vec3 centre = box.centre();
    for (int axis = 0; axis < 3; ++axis) {
        const float half = (box.high[axis] - box.low[axis]) * 0.5f;
        const float offset = point[axis] - centre[axis];
        if (offset > half || offset < -half) return false;
    }
    return true;
}

BoxApproach approachToBox(const RoomBox& box, const Vec3& eye, const Vec3& view) {
    Vec3 nearest;
    for (int axis = 0; axis < 3; ++axis) nearest[axis] = std::min(std::max(eye[axis], box.low[axis]), box.high[axis]);
    const Vec3 offset = {nearest[0] - eye[0], nearest[1] - eye[1], nearest[2] - eye[2]};
    BoxApproach approach;
    approach.distance = std::sqrt(offset[0] * offset[0] + offset[1] * offset[1] + offset[2] * offset[2]);
    approach.depth = offset[0] * view[0] + offset[1] * view[1] + offset[2] * view[2];
    return approach;
}

int objectRoomAfterMove(const StreamRooms& rooms, const std::map<int, RoomBox>& boxes, int ownRoom, int currentRoom, const Vec3& testPoint) {
    Vec3 point = testPoint;
    point[1] += kRoomTestLift;
    if (inside(boxes, ownRoom, point)) return ownRoom;
    if (inside(boxes, currentRoom, point)) return currentRoom;
    const auto current = rooms.find(currentRoom);
    if (current != rooms.end())
        for (int neighbour : current->second.neighbours)
            if (inside(boxes, neighbour, point)) return neighbour;
    for (const auto& [number, box] : boxes)
        if (roomContains(box, point)) return number;
    int best = ownRoom;
    float bestGap = 0.0f;
    bool found = false;
    for (const auto& [number, box] : boxes) {
        const Vec3 centre = box.centre();
        const Vec3 half = {(box.high[0] - box.low[0]) * 0.5f, (box.high[1] - box.low[1]) * 0.5f, (box.high[2] - box.low[2]) * 0.5f};
        if (!(std::fabs(testPoint[0] - centre[0]) < half[0]) || !(std::fabs(testPoint[2] - centre[2]) < half[2])) continue;
        const float gap = std::fabs(testPoint[1] - centre[1]) - half[1];
        if (!found || gap < bestGap) {
            best = number;
            bestGap = gap;
            found = true;
        }
    }
    return best;
}

int roomAfterMove(const StreamRooms& rooms, const std::map<int, RoomBox>& boxes, int currentRoom, const Vec3& playerPosition) {
    const Vec3 testPoint = {playerPosition[0] + kPlayerBoxCentre[0], playerPosition[1] + kPlayerBoxCentre[1],
                            playerPosition[2] + kPlayerBoxCentre[2]};
    return objectRoomAfterMove(rooms, boxes, currentRoom, currentRoom, testPoint);
}

}
