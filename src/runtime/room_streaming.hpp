#pragma once
#include "assets/assets.hpp"

#include <array>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr unsigned kRoomPreloadLimit = 3;
constexpr int kPreloadBasePriority = 200;
constexpr int kCurrentRoomPriority = 255;
constexpr int kNeighbourBasePriority = 254;

using Vec3 = std::array<float, 3>;

struct StreamRoom {
    Vec3 position = {0, 0, 0};
    std::vector<int> neighbours;
};

using StreamRooms = std::map<int, StreamRoom>;

struct RoomRequest {
    int room = 0;
    int priority = 0;
    bool preload = false;
};

struct RoomStreamInput {
    int currentRoom = 0;
    Vec3 playerPosition = {0, 0, 0};
    Vec3 viewDirection = {0, 0, 1};
    std::set<int> loaded;
    std::set<int> scripted;
};

struct RoomStreamPlan {
    std::set<int> wanted;
    std::vector<RoomRequest> loadOrder;
    int preloadAnchor = 0;
};

StreamRooms streamRoomsOf(const assets::Level& master, const std::string& worldLevelInstance,
                          const std::map<int, Vec3>& positions);

RoomStreamPlan planRoomStreaming(const StreamRooms& rooms, const RoomStreamInput& input);

}
