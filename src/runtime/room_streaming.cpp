#include "runtime/room_streaming.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace chase::runtime {
namespace {
constexpr int kMaxRoomNumber = 200;
constexpr std::uint32_t kDegenerateLengthBits = 872415232;

float viewDot(const Vec3& roomPosition, const Vec3& player, const Vec3& view) {
    Vec3 toRoom = {roomPosition[0] - player[0], roomPosition[1] - player[1], roomPosition[2] - player[2]};
    float squared = toRoom[0] * toRoom[0];
    squared += toRoom[1] * toRoom[1];
    squared += toRoom[2] * toRoom[2];
    const float length = std::sqrt(squared);
    std::uint32_t bits;
    static_assert(sizeof(bits) == sizeof(length));
    std::memcpy(&bits, &length, sizeof(bits));
    if (static_cast<std::int32_t>(bits) <= static_cast<std::int32_t>(kDegenerateLengthBits)) toRoom = {0, 0, 1};
    else {
        const float inverse = 1.0f / length;
        toRoom = {toRoom[0] * inverse, toRoom[1] * inverse, toRoom[2] * inverse};
    }
    float dot = toRoom[0] * view[0];
    dot += toRoom[1] * view[1];
    dot += toRoom[2] * view[2];
    return dot;
}

const StreamRoom* roomOf(const StreamRooms& rooms, int number) {
    const auto found = rooms.find(number);
    return found == rooms.end() ? nullptr : &found->second;
}

std::vector<int> listOf(const StreamRoom& room, int number) {
    std::vector<int> list{number};
    list.insert(list.end(), room.neighbours.begin(), room.neighbours.end());
    return list;
}
}

StreamRooms streamRoomsOf(const assets::Level& master, const std::string& worldLevelInstance,
                          const std::map<int, Vec3>& positions) {
    std::map<std::uint32_t, int> numberOfHash;
    for (int n = 1; n <= kMaxRoomNumber; ++n) numberOfHash[assets::nameHash("Room" + std::to_string(n))] = n;
    const std::uint32_t worldHash = assets::nameHash(worldLevelInstance);
    StreamRooms rooms;
    for (const assets::RoomLinks& record : master.roomLinks) {
        if (record.worldLevelHash != worldHash) continue;
        const auto number = numberOfHash.find(record.roomHash);
        if (number == numberOfHash.end()) continue;
        StreamRoom& room = rooms[number->second];
        for (const assets::RoomLinkEntry& entry : record.linked) {
            const auto linked = numberOfHash.find(entry.roomHash);
            if (linked != numberOfHash.end()) room.neighbours.push_back(linked->second);
        }
        const auto position = positions.find(number->second);
        if (position != positions.end()) room.position = position->second;
    }
    return rooms;
}

RoomStreamPlan planRoomStreaming(const StreamRooms& rooms, const RoomStreamInput& input) {
    RoomStreamPlan plan;
    const StreamRoom* current = roomOf(rooms, input.currentRoom);
    if (current) {
        const std::vector<int> list = listOf(*current, input.currentRoom);
        std::vector<float> dots;
        for (int number : list) {
            const StreamRoom* room = roomOf(rooms, number);
            dots.push_back(room ? viewDot(room->position, input.playerPosition, input.viewDirection) : 0.0f);
            plan.wanted.insert(number);
        }
        for (std::size_t i = 0; i < list.size(); ++i) {
            if (input.loaded.count(list[i])) continue;
            int priority = kCurrentRoomPriority;
            if (list[i] != input.currentRoom) {
                priority = kNeighbourBasePriority;
                for (float other : dots)
                    if (dots[i] < other) --priority;
            }
            plan.loadOrder.push_back({list[i], priority, false});
        }
        float bestDot = 0.0f;
        for (std::size_t i = 0; i < list.size(); ++i) {
            if (list[i] == input.currentRoom || !input.loaded.count(list[i])) continue;
            if (dots[i] > bestDot) {
                bestDot = dots[i];
                plan.preloadAnchor = list[i];
            }
        }
        if (const StreamRoom* anchor = roomOf(rooms, plan.preloadAnchor)) {
            std::vector<int> candidates;
            std::vector<float> candidateDots;
            for (int number : listOf(*anchor, plan.preloadAnchor)) {
                const StreamRoom* room = roomOf(rooms, number);
                if (!room || plan.wanted.count(number)) continue;
                const float dot = viewDot(room->position, input.playerPosition, input.viewDirection);
                std::size_t at = 0;
                while (at < candidates.size() && !(candidateDots[at] < dot)) ++at;
                candidates.insert(candidates.begin() + std::ptrdiff_t(at), number);
                candidateDots.insert(candidateDots.begin() + std::ptrdiff_t(at), dot);
            }
            for (std::size_t i = 0; i < candidates.size() && i < kRoomPreloadLimit; ++i) {
                plan.loadOrder.push_back({candidates[i], kPreloadBasePriority - int(i), true});
                plan.wanted.insert(candidates[i]);
            }
        }
    }
    for (int number : input.scripted) plan.wanted.insert(number);
    return plan;
}

}
