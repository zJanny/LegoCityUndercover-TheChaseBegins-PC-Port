// Synthetic cases of the object room rule (src/runtime/room_locator.cpp, src/runtime/object_rooms.cpp): the rule comes from
// the disassembly of 0x2e6ce4, 0x237b04 and 0x22d204 (docs/re/streaming-rooms.md); the rooms here are made up.
#include "runtime/object_rooms.hpp"
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

RoomBox box(float lowX, float highX) { return {{lowX, 0.0f, 0.0f}, {highX, 10.0f, 10.0f}}; }

// Rooms 2 and 3 share one box; room 4 lists room 3 as its neighbour, room 1 lists nothing.
const std::map<int, RoomBox> kBoxes = {{1, box(0, 10)}, {2, box(10, 20)}, {3, box(10, 20)}, {4, box(20, 30)}};
const StreamRooms kRooms = {{1, {}}, {2, {{0, 0, 0}, {1, 3}}}, {3, {{0, 0, 0}, {2, 4}}}, {4, {{0, 0, 0}, {3}}}};

void ownRoomIsKeptWhileItContainsThePoint() {
    require(objectRoomAfterMove(kRooms, kBoxes, 4, 1, {25, 5, 5}) == 4, "inside its own box the object stays");
    require(objectRoomAfterMove(kRooms, kBoxes, 3, 2, {15, 5, 5}) == 3, "a point inside two boxes keeps the own room");
}

void currentRoomListComesBeforeNumberOrder() {
    require(objectRoomAfterMove(kRooms, kBoxes, 4, 4, {15, 5, 5}) == 3, "room 4's list holds room 3, picked before room 2");
    require(objectRoomAfterMove(kRooms, kBoxes, 4, 1, {15, 5, 5}) == 2, "room 1's list is empty: the first room in number order");
}

void pointIsLiftedBeforeTheBoxTest() {
    require(objectRoomAfterMove(kRooms, kBoxes, 4, 1, {5, -0.05f, 5}) == 1, "0.05 below the floor is inside after the lift of 0.1");
}

void belowEveryBoxTheSmallestVerticalGapWins() {
    const std::map<int, RoomBox> stacked = {{1, {{0, 20, 0}, {10, 30, 10}}}, {2, {{0, 2, 0}, {10, 12, 10}}}, {4, box(20, 30)}};
    require(objectRoomAfterMove(kRooms, stacked, 4, 1, {5, 15, 5}) == 2, "between two boxes the nearer one in height is taken");
    require(objectRoomAfterMove(kRooms, stacked, 4, 1, {50, 5, 5}) == 4, "outside every x/z range the own room is kept");
}

void onlyThePlayerRoomAndTheRoundRobinEntryAreTested() {
    ObjectRoomMover mover;
    std::vector<int> rooms = {3, 2};
    const std::vector<MovingObject> objects = {{0, {5, 5, 5}}, {1, {25, 5, 5}}};
    std::vector<ObjectRoomMove> moves = mover.update(kRooms, kBoxes, 2, 2, objects, rooms);
    require(moves.size() == 1 && moves[0].index == 1 && moves[0].from == 2 && moves[0].to == 4, "update 1 visits room 2: only the object of room 2 moves");
    require(rooms[0] == 3 && rooms[1] == 4, "the move is written to the object's room");
    moves = mover.update(kRooms, kBoxes, 2, 2, objects, rooms);
    require(moves.empty() && rooms[0] == 3, "update 2 visits room 1, the object of room 3 waits");
    moves = mover.update(kRooms, kBoxes, 2, 2, objects, rooms);
    require(moves.size() == 1 && moves[0].index == 0 && rooms[0] == 1, "update 3 visits room 3: its object moves to room 1 of the current list");
}
}

int main() {
    try {
        ownRoomIsKeptWhileItContainsThePoint();
        currentRoomListComesBeforeNumberOrder();
        pointIsLiftedBeforeTheBoxTest();
        belowEveryBoxTheSmallestVerticalGapWins();
        onlyThePlayerRoomAndTheRoundRobinEntryAreTested();
    } catch (const std::exception& error) {
        std::cerr << "object-rooms-test: " << error.what() << '\n';
        return 1;
    }
    std::cout << "object-rooms-test: ok\n";
    return 0;
}
