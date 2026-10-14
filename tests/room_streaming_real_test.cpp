#include "assets/assets.hpp"
#include "runtime/room_locator.hpp"
#include "runtime/room_streamer.hpp"
#include "runtime/room_streaming.hpp"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using namespace chase;
using namespace chase::runtime;

void require(bool ok, const std::string& what) {
    if (!ok) throw std::runtime_error(what);
}

std::string show(const std::set<int>& rooms) {
    std::string text;
    for (int room : rooms) text += (text.empty() ? "" : ",") + std::to_string(room);
    return text;
}

// Room node positions ([room+0x14]+0xa0) of the 43 rooms of WorldLevel_01_CherryTreeHills, GDB reads of the oracle
// (state wstream-room8-settled, agent O, no RPC); the same values in every state of the street walk.
const std::map<int, Vec3> kRoomPositions = {
        {1, {1111.51648f, 67.2134628f, 1250.27954f}},
        {2, {849.762451f, 69.2693176f, 1250.27954f}},
        {3, {749.762451f, 70.0978546f, 1250.27954f}},
        {4, {649.762329f, 70.5980606f, 1250.27954f}},
        {5, {506.835083f, 71.7534637f, 1250.27954f}},
        {6, {1109.06726f, 39.0744095f, 1100.2373f}},
        {7, {949.762451f, 45.3379631f, 1100.2373f}},
        {8, {849.762451f, 27.4065304f, 1100.2373f}},
        {9, {749.762451f, 38.8720474f, 1100.2373f}},
        {10, {649.762329f, 37.5572052f, 1100.2373f}},
        {11, {549.762329f, 59.0745277f, 1100.2373f}},
        {12, {456.835083f, 85.2054749f, 1081.86951f}},
        {13, {87.2604675f, 68.5950241f, 1145.32898f}},
        {14, {1054.11829f, 78.8463593f, 1000.23743f}},
        {15, {849.762451f, 35.4777794f, 1000.23743f}},
        {16, {749.76239f, 36.608532f, 1000.23743f}},
        {17, {649.762329f, 35.6101151f, 1000.23743f}},
        {18, {549.762329f, 30.1917591f, 1000.23743f}},
        {19, {456.835144f, 51.1090813f, 981.869507f}},
        {20, {1111.57117f, 95.6759415f, 900.237366f}},
        {21, {849.762451f, 84.1589508f, 900.237427f}},
        {22, {749.762451f, 84.1087723f, 900.237305f}},
        {23, {649.762329f, 40.6532364f, 900.237427f}},
        {24, {549.762329f, 34.507309f, 900.237366f}},
        {25, {456.835083f, 49.983326f, 900.237305f}},
        {26, {1132.26392f, 98.2464447f, 800.237366f}},
        {27, {849.762451f, 90.3772736f, 800.237366f}},
        {28, {749.76239f, 85.3323059f, 813.655396f}},
        {29, {649.762329f, 44.9121017f, 813.655396f}},
        {30, {549.762329f, 36.487114f, 813.655334f}},
        {31, {449.762329f, 55.0691719f, 813.655334f}},
        {32, {1131.12134f, 89.0286789f, 700.237366f}},
        {33, {849.762451f, 51.3361359f, 700.237427f}},
        {34, {754.742798f, 48.2583771f, 713.655457f}},
        {35, {654.742615f, 90.0561829f, 713.655334f}},
        {36, {549.762329f, 52.4004211f, 713.655334f}},
        {37, {449.762329f, 59.7859039f, 713.655334f}},
        {38, {1127.02527f, 73.5914307f, 600.237305f}},
        {39, {849.762451f, 46.774334f, 600.237305f}},
        {40, {754.742798f, 27.8800888f, 600.237305f}},
        {41, {654.742615f, 69.5910339f, 600.237305f}},
        {42, {539.984497f, 37.9584389f, 613.018005f}},
        {43, {982.401489f, 66.3164291f, 260.368927f}},
};

struct Reading {
    const char* provenance;
    int currentRoom;
    Vec3 player, view;
    std::set<int> loaded;
};

// Player position: node (handle of *0x3772b8 +0x3c) +0x5c +0x30; view: object *0x377544 +0x5c +0x20 (third matrix row).
// Loaded: rooms of scene 2 with state byte 2, settled (no room in state 1). Readings are GDB memory reads of the oracle.
const Reading kReadings[] = {
    {"street-at-entrance +1 update (oracle agentO 2026-10-14, pad idle)", 7,
     {903.71875f, 16.573266983f, 1113.3282470703125f}, {0.19336770474910736f, -0.16759523749351501f, 0.9667061567306519f},
     {1, 2, 3, 6, 7, 8, 9, 14, 15}},
    {"wstream-room8-settled (street-at-entrance, stick 20 150 for 60 updates, 90 updates settle)", 8,
     {887.78595f, 16.6094093f, 1134.23364f}, {-0.57240957f, -0.167595565f, 0.802657485f},
     {1, 2, 3, 4, 7, 8, 9, 10, 14, 15, 16}},
    {"wstream-walk-b (wstream-room8-settled, stick -150 0 for 100 updates, 90 settle)", 8,
     {887.7995f, 16.6133823f, 1123.86658f}, {-0.970507741f, -0.167594641f, 0.173282176f},
     {1, 2, 3, 4, 7, 8, 9, 10, 14, 15, 16, 17}},
    {"wstream-walk-c (wstream-walk-b, stick 0 150 for 120 updates, 90 settle)", 8,
     {879.095154f, 20.1091995f, 1130.59094f}, {-0.853561103f, -0.167595208f, 0.493300498f},
     {1, 2, 3, 4, 7, 8, 9, 10, 14, 15, 16}},
    {"wstream-walk-d (wstream-walk-c, stick -150 -100 for 150 updates, 90 settle)", 7,
     {912.068909f, 16.6570244f, 1087.27771f}, {-0.711438954f, -0.595208645f, -0.373605996f},
     {1, 2, 6, 7, 8, 9, 14, 15, 16, 22}},
};
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: room-streaming-real-test FIB NAMES\n";
        return 2;
    }
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) {
        std::cout << "room-streaming-real-test: data not available, skipped\n";
        return 77;
    }
    try {
        assets::FuseArchive archive(argv[1]);
        archive.loadNames(argv[2]);
        const assets::FuseEntry* entry = archive.find("levels/levels/lvl_the_city.blvl");
        require(entry != nullptr, "master level not in archive");
        const assets::Level city = assets::parseLevel(archive.read(*entry));
        const StreamRooms rooms = streamRoomsOf(city, "WorldLevel_01_CherryTreeHills", kRoomPositions);
        require(rooms.size() == 43, "43 rooms of WorldLevel_01_CherryTreeHills");
        // Room node positions derived from models/lvl_01_cherrytreehills/roomN.bfnmdl against the 43 oracle readings above.
        const StreamRooms derived = streamRoomsOf(city, "WorldLevel_01_CherryTreeHills", roomBoxesOf(archive, "models/lvl_01_cherrytreehills"));
        require(derived.size() == 43, "43 derived rooms");
        for (const auto& [number, position] : kRoomPositions)
            for (int axis = 0; axis < 3; ++axis)
                require(std::fabs(derived.at(number).position[axis] - position[axis]) < 0.01f,
                        "Room" + std::to_string(number) + " node position axis " + std::to_string(axis) + " from the model: " +
                            std::to_string(derived.at(number).position[axis]) + " vs oracle " + std::to_string(position[axis]));
        for (const Reading& reading : kReadings) {
            RoomStreamInput input;
            input.currentRoom = reading.currentRoom;
            input.playerPosition = reading.player;
            input.viewDirection = reading.view;
            input.loaded = reading.loaded;
            const RoomStreamPlan plan = planRoomStreaming(rooms, input);
            require(plan.wanted == reading.loaded, std::string(reading.provenance) + ": wanted " + show(plan.wanted) +
                                                       " but loaded " + show(reading.loaded));
            require(plan.loadOrder.empty() || plan.loadOrder.front().preload,
                    std::string(reading.provenance) + ": a settled set requests only preloads");
        }
        // walk1 trace (agent O 2026-10-14, memtrace of the room request bytes): update 28 enters Room8 with Room6 loaded and
        // Room16 not: Room6 is unwanted at once and Room16 is requested as a neighbour.
        RoomStreamInput entering;
        entering.currentRoom = 8;
        entering.playerPosition = kReadings[1].player;
        entering.viewDirection = kReadings[1].view;
        entering.loaded = {1, 2, 3, 6, 7, 8, 9, 14, 15};
        const RoomStreamPlan entered = planRoomStreaming(rooms, entering);
        require(!entered.wanted.count(6) && entered.wanted.count(16), "entering Room8 unwants Room6 and wants Room16");
        bool requested16 = false;
        for (const RoomRequest& request : entered.loadOrder) requested16 = requested16 || (request.room == 16 && !request.preload);
        require(requested16, "Room16 is requested as a neighbour");
        const std::optional<RoomStreamer> made =
            RoomStreamer::forWorldLevel(city, "WorldLevel_01_CherryTreeHills", archive, "models/lvl_01_cherrytreehills");
        require(made.has_value(), "the street level is streamable");
        RoomStreamer streamer = *made;
        streamer.settle(7, kReadings[0].player, kReadings[0].view);
        require(streamer.loadedRooms() == kReadings[0].loaded, "settled start set " + show(streamer.loadedRooms()));
        // The oracle walks in order: each reading's set is reached by updates with that reading's player and view
        // (walk1 enters Room8 at update 28 and loads Room16 in 9 updates, agent O memtrace).
        for (int update = 1; update <= 40; ++update) {
            streamer.update(kReadings[1].player, kReadings[1].view);
            if (update == 1)
                require(!streamer.loaded(6) && streamer.currentRoom() == 8 && streamer.loading().count(16) && !streamer.loaded(16),
                        "update 1 in Room8: Room6 unloaded, Room16 loading");
            if (update == int(kRoomLoadUpdates)) require(!streamer.loaded(16), "Room16 still loading 9 updates after the request");
            if (update == int(kRoomLoadUpdates) + 1) require(streamer.loaded(16), "Room16 loaded at update request + 9 (28 to 37 in the trace)");
        }
        require(streamer.loadedRooms() == kReadings[1].loaded, "wstream-room8-settled set " + show(streamer.loadedRooms()));
        {
            // Room change rule (0x2e6ce4 -> 0x237b04 -> 0x22d204), oracle agentRS 2026-10-14: street-at-entrance, 3 updates idle,
            // stick 20 150, memtrace of the current room 0x376c78 and the player node world position each update. The current
            // room is Room7 (0x880cf40) up to the record with x 899.7787 and Room8 (0x880d580) from the record with
            // x 899.3618 (Room7's box starts at x 899.53, its centre 949.76 minus half extent 50.24).
            RoomStreamer walk = *made;
            walk.settle(7, kReadings[0].player, kReadings[0].view);
            const Vec3 lastInRoom7 = {899.7787475585938f, 16.550920486450195f, 1121.957763671875f};
            const Vec3 firstInRoom8 = {899.3617553710938f, 16.550920486450195f, 1122.3746337890625f};
            const Vec3 blocked = {888.0829467773438f, 16.601083755493164f, 1134.0035400390625f};
            walk.update(lastInRoom7, kReadings[0].view);
            require(walk.currentRoom() == 7, "x 899.7787 is still Room7");
            walk.update(firstInRoom8, kReadings[0].view);
            require(walk.currentRoom() == 8, "x 899.3618 leaves Room7 for Room8");
            walk.update(blocked, kReadings[0].view);
            require(walk.currentRoom() == 8, "the blocked position stays in Room8");
            const RoomBox& box7 = walk.boxes().at(7);
            const RoomBox& box8 = walk.boxes().at(8);
            require(roomContains(box7, {lastInRoom7[0] + kPlayerBoxCentre[0], lastInRoom7[1] + kPlayerBoxCentre[1] + kRoomTestLift,
                                        lastInRoom7[2] + kPlayerBoxCentre[2]}) &&
                        roomContains(box8, {lastInRoom7[0] + kPlayerBoxCentre[0], lastInRoom7[1] + kPlayerBoxCentre[1] + kRoomTestLift,
                                            lastInRoom7[2] + kPlayerBoxCentre[2]}),
                    "the last Room7 position is inside both boxes: the room is kept while it still contains the player");
            require(roomAfterMove(walk.rooms(), walk.boxes(), 8, lastInRoom7) == 8, "Room8 is kept while it contains the player");
            require(roomAfterMove(walk.rooms(), walk.boxes(), 7, lastInRoom7) == 7, "Room7 is kept while it contains the player");
            // Same scenario continued (stick -150 0): the player is back at x 899.629 inside both boxes with the current room
            // Room8 (0x880d580, records 63 and 64 of the walk) and Room7 (0x880cf40) from the record with x 900.273, past
            // Room8's high x of 900.0. All 160 records of the two traces (walk1: 80, walk2: 35 + 45) agree with the rule.
            require(roomAfterMove(walk.rooms(), walk.boxes(), 8, {899.6290893554688f, 16.586313247680664f, 1113.384521484375f}) == 8,
                    "Room8 is kept at x 899.629 although Room7 contains it too");
            require(roomAfterMove(walk.rooms(), walk.boxes(), 8, {900.2732543945312f, 16.586313247680664f, 1113.7161865234375f}) == 7,
                    "x 900.273 leaves Room8 for Room7");
        }
        {
            // RENDER-31, oracle agentRS 2026-10-14 (memtrace with the camera object [0x377544]+0x5c: eye at +0x30, look
            // direction at +0x20): camera position and view at the update a room's state byte becomes 2, with the nearest
            // box point distance and its depth along the view printed by the analysis (docs/re/rendering-fog.md).
            struct Loaded {
                int room;
                Vec3 eye, view;
                float distance, depth;
            };
            const Loaded loads[] = {
                {16, {900.60888671875f, 20.445377349853516f, 1116.3876953125f}, {-0.42286717891693115f, -0.16819892823696136f, 0.8904451131820679f}, 120.3f, -16.1f},
                {4, {901.0902709960938f, 20.464563369750977f, 1119.4110107421875f}, {-0.887476921081543f, -0.16730576753616333f, 0.4294106066226959f}, 203.4f, 191.6f},
                {10, {900.5964965820312f, 20.50408172607422f, 1120.866943359375f}, {-0.9791252017021179f, -0.16837888956069946f, -0.11385247111320496f}, 200.6f, 196.4f},
                {6, {893.1802978515625f, 20.473302841186523f, 1117.841064453125f}, {0.9851309657096863f, -0.16984662413597107f, 0.02586953155696392f}, 106.3f, 104.8f},
            };
            for (const Loaded& load : loads) {
                const BoxApproach approach = approachToBox(made->boxes().at(load.room), load.eye, load.view);
                require(std::fabs(approach.distance - load.distance) < 0.1f && std::fabs(approach.depth - load.depth) < 0.1f,
                        "Room" + std::to_string(load.room) + " approach " + std::to_string(approach.distance) + " / " + std::to_string(approach.depth));
            }
        }
        for (std::size_t index = 2; index < sizeof(kReadings) / sizeof(kReadings[0]); ++index) {
            for (int update = 0; update < 40; ++update) streamer.update(kReadings[index].player, kReadings[index].view);
            require(streamer.currentRoom() == kReadings[index].currentRoom, std::string(kReadings[index].provenance) + ": current room");
            require(streamer.loadedRooms() == kReadings[index].loaded,
                    std::string(kReadings[index].provenance) + ": streamer set " + show(streamer.loadedRooms()));
        }
    } catch (const std::exception& error) {
        std::cerr << "room-streaming-real-test: FAIL: " << error.what() << "\n";
        return 1;
    }
    std::cout << "room-streaming-real-test: OK\n";
    return 0;
}
