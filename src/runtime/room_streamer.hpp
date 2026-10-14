#pragma once
// The loaded-room state of one world level: the request rule of room_streaming.hpp applied every update. No SDL.
#include "runtime/room_streaming.hpp"

#include <map>
#include <optional>
#include <set>
#include <string>

namespace chase::runtime {

// Updates a requested room stays in the loading state (state byte 1) before it counts as loaded: Room16 in the oracle walk
// (docs/re/streaming-rooms.md). The real duration is asynchronous and host dependent (WSTREAM-S4).
constexpr unsigned kRoomLoadUpdates = 9;

class RoomStreamer {
public:
    RoomStreamer(StreamRooms rooms, std::map<int, RoomBox> boxes);

    // Rooms whose model exists for every room of the world level; nothing otherwise (the level is then not streamed).
    static std::optional<RoomStreamer> forWorldLevel(const assets::Level& master, const std::string& worldLevelInstance,
                                                    assets::FuseArchive& archive, const std::string& levelFolder);

    // The level load: the rooms wanted around the start are loaded before the first update.
    void settle(int startRoom, const Vec3& player, const Vec3& view);
    void update(const Vec3& player, const Vec3& view);

    int currentRoom() const { return currentRoom_; }
    bool loaded(int room) const { return loaded_.count(room) > 0; }
    const std::set<int>& loadedRooms() const { return loaded_; }
    const std::map<int, unsigned>& loading() const { return loading_; }
    const StreamRooms& rooms() const { return rooms_; }

    // WSTREAM-S4: the room whose box (x and z) contains the player, the smallest one when several do, else the room kept.
    int roomAt(const Vec3& player) const;

private:
    void apply(const RoomStreamPlan& plan);

    StreamRooms rooms_;
    std::map<int, RoomBox> boxes_;
    std::set<int> loaded_;
    std::map<int, unsigned> loading_;
    int currentRoom_ = 0;
};

}
