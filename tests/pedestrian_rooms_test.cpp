// Synthetic cases of the pedestrians' room move (src/runtime/pedestrian_rooms.cpp, rule of 0x2e6ce4 in docs/re/streaming-rooms.md,
// pedestrian list 0 from the create 0x1913f8 and 0x242b08 in docs/re/pedestrians-inputs.md); the rooms here are made up.
#include "runtime/pedestrian_rooms.hpp"
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

RoomBox box(float lowX, float highX) { return {{lowX, 0.0f, 0.0f}, {highX, 10.0f, 10.0f}}; }

const std::map<int, RoomBox> kBoxes = {{1, box(0, 10)}, {2, box(10, 20)}};
const StreamRooms kRooms = {{1, {{0, 0, 0}, {2}}}, {2, {{0, 0, 0}, {1}}}};

void testPointAddsTheBoundsCentre() {
    const Vec3 point = pedestrianRoomTestPoint({10.0f, 1.0f, 2.0f}, {0.5f, 0.25f, -1.0f});
    require(point[0] == 10.5f && point[1] == 1.25f && point[2] == 1.0f, "node translation plus box centre");
}

void aPedestrianWalkingPastTheBoxEdgeChangesRoom() {
    ObjectRoomMover mover;
    std::vector<PedestrianRoomCandidate> candidates = {{3, 1, {9.0f, 0.0f, 5.0f}, {0, 0, 0}}};
    require(movePedestrianRooms(mover, kRooms, kBoxes, 1, 1, candidates).empty(), "inside room 1 nothing moves");
    candidates[0].nodePosition = {10.5f, 0.0f, 5.0f};
    const std::vector<PedestrianRoomMove> moves = movePedestrianRooms(mover, kRooms, kBoxes, 1, 1, candidates);
    require(moves.size() == 1 && moves[0].slot == 3 && moves[0].from == 1 && moves[0].to == 2, "past x 10 the pedestrian is in room 2");
}

void theBoundsCentreShiftsTheTestPoint() {
    ObjectRoomMover mover;
    const std::vector<PedestrianRoomCandidate> candidates = {{0, 1, {9.5f, 0.0f, 5.0f}, {1.0f, 0.5f, 0}}};
    const std::vector<PedestrianRoomMove> moves = movePedestrianRooms(mover, kRooms, kBoxes, 1, 1, candidates);
    require(moves.size() == 1 && moves[0].to == 2, "the box centre pushes the test point over the edge");
}

void onlyTheVisitedRoomsAreTested() {
    ObjectRoomMover mover;
    const std::vector<PedestrianRoomCandidate> candidates = {{0, 2, {5.0f, 0.0f, 5.0f}, {0, 0, 0}}};
    require(movePedestrianRooms(mover, kRooms, kBoxes, 1, 1, candidates).empty(), "update 1 visits room 1 and the player's room 1");
    const std::vector<PedestrianRoomMove> moves = movePedestrianRooms(mover, kRooms, kBoxes, 1, 1, candidates);
    require(moves.size() == 1 && moves[0].from == 2 && moves[0].to == 1, "update 2 visits room 2 in the round-robin");
}

}

int main() {
    try {
        testPointAddsTheBoundsCentre();
        aPedestrianWalkingPastTheBoxEdgeChangesRoom();
        theBoundsCentreShiftsTheTestPoint();
        onlyTheVisitedRoomsAreTested();
    } catch (const std::exception& error) {
        std::cerr << "pedestrian-rooms-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "pedestrian-rooms-test: ok\n";
    return 0;
}
