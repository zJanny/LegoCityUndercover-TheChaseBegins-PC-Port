#include "runtime/room_streamer.hpp"

#include <utility>
#include <vector>

namespace chase::runtime {
namespace {
constexpr unsigned kSettleLimit = 16;
}

RoomStreamer::RoomStreamer(StreamRooms rooms, std::map<int, RoomBox> boxes) : rooms_(std::move(rooms)), boxes_(std::move(boxes)) {}

std::optional<RoomStreamer> RoomStreamer::forWorldLevel(const assets::Level& master, const std::string& worldLevelInstance,
                                                       assets::FuseArchive& archive, const std::string& levelFolder) {
    std::map<int, RoomBox> boxes = roomBoxesOf(archive, levelFolder);
    StreamRooms rooms = streamRoomsOf(master, worldLevelInstance, boxes);
    if (rooms.empty()) return std::nullopt;
    for (const auto& [number, room] : rooms) {
        if (!boxes.count(number)) return std::nullopt;
        for (int neighbour : room.neighbours)
            if (!rooms.count(neighbour)) return std::nullopt;
    }
    return RoomStreamer(std::move(rooms), std::move(boxes));
}

int RoomStreamer::roomAt(const Vec3& player) const {
    int best = currentRoom_;
    float bestArea = 0;
    bool found = false;
    for (const auto& [number, box] : boxes_) {
        if (player[0] < box.low[0] || player[0] > box.high[0] || player[2] < box.low[2] || player[2] > box.high[2]) continue;
        const float area = (box.high[0] - box.low[0]) * (box.high[2] - box.low[2]);
        if (!found || area < bestArea) {
            best = number;
            bestArea = area;
            found = true;
        }
    }
    return best;
}

void RoomStreamer::apply(const RoomStreamPlan& plan) {
    for (auto it = loaded_.begin(); it != loaded_.end();) it = plan.wanted.count(*it) ? std::next(it) : loaded_.erase(it);
    for (auto it = loading_.begin(); it != loading_.end();) it = plan.wanted.count(it->first) ? std::next(it) : loading_.erase(it);
    for (const RoomRequest& request : plan.loadOrder)
        if (!loaded_.count(request.room) && !loading_.count(request.room)) loading_[request.room] = kRoomLoadUpdates;
}

void RoomStreamer::settle(int startRoom, const Vec3& player, const Vec3& view) {
    currentRoom_ = startRoom;
    loaded_.clear();
    loading_.clear();
    for (unsigned pass = 0; pass < kSettleLimit; ++pass) {
        RoomStreamInput input;
        input.currentRoom = currentRoom_;
        input.playerPosition = player;
        input.viewDirection = view;
        input.loaded = loaded_;
        const RoomStreamPlan plan = planRoomStreaming(rooms_, input);
        if (plan.wanted == loaded_) return;
        loaded_ = plan.wanted;
    }
}

void RoomStreamer::update(const Vec3& player, const Vec3& view) {
    for (auto it = loading_.begin(); it != loading_.end();) {
        if (--it->second == 0) {
            loaded_.insert(it->first);
            it = loading_.erase(it);
        } else {
            ++it;
        }
    }
    currentRoom_ = roomAt(player);
    RoomStreamInput input;
    input.currentRoom = currentRoom_;
    input.playerPosition = player;
    input.viewDirection = view;
    input.loaded = loaded_;
    apply(planRoomStreaming(rooms_, input));
}

}
