#pragma once
#include "assets/assets.hpp"

#include <array>
#include <map>
#include <optional>
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

// Vertex bounding box of every mesh of the room model except mesh 0 (WSTREAM-S3, fit to 43 oracle readings); its centre is
// the room node position the ranking uses.
struct RoomBox {
    Vec3 low = {0, 0, 0}, high = {0, 0, 0};
    Vec3 centre() const { return {(low[0] + high[0]) * 0.5f, (low[1] + high[1]) * 0.5f, (low[2] + high[2]) * 0.5f}; }
};

std::optional<RoomBox> roomBoxOf(const assets::Model& model);
std::map<int, RoomBox> roomBoxesOf(assets::FuseArchive& archive, const std::string& levelFolder);

StreamRooms streamRoomsOf(const assets::Level& master, const std::string& worldLevelInstance, const std::map<int, RoomBox>& boxes);

RoomStreamPlan planRoomStreaming(const StreamRooms& rooms, const RoomStreamInput& input);

}
