#include "game_level.hpp"

#include "runtime/collision.hpp"
#include "runtime/pedestrian_manager.hpp"
#include "runtime/pedestrian_walk.hpp"

#include <cmath>
#include <iostream>

namespace game_pedestrians {

std::vector<LevelPedestrians::LiveCharacter> LevelPedestrians::liveCharacters(const assets::PlacedObjectState& state) const {
    std::vector<LiveCharacter> live;
    for (std::size_t i = 0; i < slots_.size(); ++i) {
        if (state.isDisabled(system_.slots()[i].instance)) continue;
        const runtime::PedestrianEntityBox box = runtime::pedestrianEntityBox(slots_[i].boundsCentre, slots_[i].boundsHalf);
        live.push_back({i, system_.body(i).node, box.centre, box.half});
    }
    return live;
}

}

namespace game_level {
namespace runtime = chase::runtime;

namespace {

runtime::AiMatrix rowsOf(const std::array<runtime::PedestrianVec3, 4>& node) {
    runtime::AiMatrix rows{};
    for (std::size_t row = 0; row < 4; ++row) rows[row] = {node[row][0], node[row][1], node[row][2]};
    return rows;
}

runtime::TrafficWorldBox boxOf(const chase::runtime::RoomBox& room) {
    runtime::TrafficWorldBox box;
    for (int axis = 0; axis < 3; ++axis) {
        box.centre[axis] = (room.low[axis] + room.high[axis]) * 0.5f;
        box.half[axis] = (room.high[axis] - room.low[axis]) * 0.5f;
    }
    return box;
}

}

// STAND-IN TRAFFIC-S44: the layout comes from the vertex boxes of all rooms of the level (both bounds sets of 0x271280), the player is in the
// grid unless seated in a vehicle, every enabled pedestrian is in it, the pair test 0x1fc508 ignores no pair and the push of a gathered
// character is not applied (TRAFFIC-56).
void LevelSession::feedTrafficCharacters(runtime::TrafficDriveStep& step) {
    if (!trafficCharacters) {
        if (!roomStreamer || roomStreamer->boxes().empty()) return;
        std::vector<runtime::TrafficWorldBox> boxes;
        for (const auto& [room, box] : roomStreamer->boxes()) boxes.push_back(boxOf(box));
        trafficCharacters.emplace(runtime::buildTrafficGridLayout(boxes, boxes));
        const runtime::TrafficGridLayout& layout = trafficCharacters->grid().layout;
        std::cout << "traffic character grid: " << int(layout.counts[0]) << " x " << int(layout.counts[1]) << " x " << int(layout.counts[2]) << " cells at update " << updatesForLog << '\n';
    }
    std::vector<runtime::TrafficFedCharacter> characters;
    if (!playerSeated()) {
        const chase::runtime::PlayerBox box;
        const float heading = runtime::headingRadians(player->motion.facing);
        runtime::TrafficFedCharacter fed;
        fed.id = runtime::kTrafficFeedPlayerId;
        fed.node = rowsOf(runtime::pedestrianPlacement({player->position()[0], player->position()[1], player->position()[2]}, {std::sin(heading), 0.0f, std::cos(heading)}));
        fed.shapeCentre = {box.centre[0], box.centre[1], box.centre[2]};
        fed.shapeHalf = {box.half[0], box.half[1], box.half[2]};
        characters.push_back(fed);
    }
    for (const game_pedestrians::LevelPedestrians::LiveCharacter& live : pedestrians.liveCharacters(objectState)) {
        runtime::TrafficFedCharacter fed;
        fed.id = runtime::kTrafficFeedPedestrianBase + std::uint32_t(live.slot);
        fed.node = rowsOf(live.node);
        fed.shapeCentre = {live.shapeCentre[0], live.shapeCentre[1], live.shapeCentre[2]};
        fed.shapeHalf = {live.shapeHalf[0], live.shapeHalf[1], live.shapeHalf[2]};
        characters.push_back(fed);
    }
    trafficCharacters->update(characters);

    trafficContactsListed.resize(traffic.carCount(), 0);
    const runtime::TrafficContactGather gather = runtime::makeTrafficContactGather(
        trafficCharacters->grid(), [](std::size_t car) { return runtime::kTrafficFeedCarBase + std::uint32_t(car); });
    step.gatherContacts = [this, gather](std::size_t car, const runtime::AiVec3& centre, const runtime::AiVec3& half,
                                         const runtime::TrafficContactDescriptor& descriptor, unsigned maxEntries) {
        std::vector<runtime::TrafficContactEntry> listed = gather(car, centre, half, descriptor, maxEntries);
        if (listed.size() != trafficContactsListed[car])
            std::cout << "traffic car " << traffic.instance(car) << ": contact gather lists " << listed.size() << " characters at update " << updatesForLog << '\n';
        trafficContactsListed[car] = listed.size();
        return listed;
    };
    step.slide.ignorePair = [](std::uint32_t) { return false; };
    step.slide.push = [](runtime::AiVec3&, std::size_t, std::vector<runtime::TrafficContactEntry>&, bool) { return false; };
}

}
