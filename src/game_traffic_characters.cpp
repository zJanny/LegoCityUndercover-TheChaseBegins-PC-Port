#include "game_level.hpp"

#include "runtime/collision.hpp"
#include "runtime/pedestrian_manager.hpp"
#include "runtime/pedestrian_walk.hpp"

#include <cmath>
#include <iostream>

namespace game_pedestrians {

std::vector<LevelPedestrians::LiveCharacter> LevelPedestrians::allCharacters(const assets::PlacedObjectState& state) const {
    std::vector<LiveCharacter> all;
    for (std::size_t i = 0; i < slots_.size(); ++i) {
        const runtime::PedestrianEntityBox box = runtime::pedestrianEntityBox(slots_[i].boundsCentre, slots_[i].boundsHalf);
        all.push_back({i, state.isDisabled(system_.slots()[i].instance), system_.body(i).node, box.centre, box.half});
    }
    return all;
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

// STAND-IN TRAFFIC-S46: the layout's first bound set is the root boxes of the level's static collision items and the second the vertex boxes of
// all rooms (0x271280 takes the scene's shape list, the rooms' items and the node bounds of the rooms of the current room set), the grid is built
// once at the first drive step (the original builds it at 0x30e0d8, called at the scene's start), the player is flagged disabled while seated in a
// vehicle, the pair test 0x1fc508 ignores no pair, and a pedestrian's state is idle. STAND-IN TRAFFIC-S47: the hit of a message 45 is only
// reported; no pedestrian reaction and no player hit exist.
void LevelSession::feedTrafficCharacters(runtime::TrafficDriveStep& step) {
    if (!trafficCharacters) {
        if (!roomStreamer || roomStreamer->boxes().empty()) return;
        std::vector<runtime::TrafficWorldBox> rooms;
        for (const auto& [room, box] : roomStreamer->boxes()) rooms.push_back(boxOf(box));
        std::vector<runtime::TrafficWorldBox> shapes = runtime::trafficStaticShapeBoxes(collision);
        if (shapes.empty()) shapes = rooms;
        trafficCharacters.emplace(runtime::buildTrafficGridLayout(shapes, rooms));
        trafficPush.emplace(collision, *trafficCharacters);
        const runtime::TrafficGridLayout& layout = trafficCharacters->grid().layout;
        std::cout << "traffic character grid: " << int(layout.counts[0]) << " x " << int(layout.counts[1]) << " x " << int(layout.counts[2]) << " cells at update " << updatesForLog << '\n';
    }
    std::vector<runtime::TrafficFedCharacter> characters;
    {
        const chase::runtime::PlayerBox box;
        const float heading = runtime::headingRadians(player->motion.facing);
        runtime::TrafficFedCharacter fed;
        fed.id = runtime::kTrafficFeedPlayerId;
        fed.node = rowsOf(runtime::pedestrianPlacement({player->position()[0], player->position()[1], player->position()[2]}, {std::sin(heading), 0.0f, std::cos(heading)}));
        fed.shapeCentre = {box.centre[0], box.centre[1], box.centre[2]};
        fed.shapeHalf = {box.half[0], box.half[1], box.half[2]};
        fed.disabled = playerSeated();
        characters.push_back(fed);
    }
    for (const game_pedestrians::LevelPedestrians::LiveCharacter& live : pedestrians.allCharacters(objectState)) {
        runtime::TrafficFedCharacter fed;
        fed.id = runtime::kTrafficFeedPedestrianBase + std::uint32_t(live.slot);
        fed.node = rowsOf(live.node);
        fed.shapeCentre = {live.shapeCentre[0], live.shapeCentre[1], live.shapeCentre[2]};
        fed.shapeHalf = {live.shapeHalf[0], live.shapeHalf[1], live.shapeHalf[2]};
        fed.disabled = live.disabled;
        characters.push_back(fed);
    }
    trafficCharacters->update(characters);

    const auto bodyOf = step.body;
    step.body = [this, bodyOf](std::size_t car) {
        std::optional<runtime::TrafficCarBody> body = bodyOf(car);
        if (body) trafficPush->setCar(car, {body->node, body->centre, body->halfExtents, body->forwardSpeed});
        return body;
    };
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
    step.carFlags = runtime::kTrafficCarEntityFlags;
    step.slide.ignorePair = [](std::uint32_t) { return false; };
    step.pushOf = [this](std::size_t car) { return trafficPush->pushOf(car); };
}

void LevelSession::reportTrafficPushes() {
    if (!trafficPush) return;
    for (const runtime::TrafficCharacterPush& push : trafficPush->takePushes()) {
        std::cout << "traffic car " << traffic.instance(push.car) << ": message 45 to " << (push.player ? std::string("the player") : "pedestrian " + std::to_string(push.character - runtime::kTrafficFeedPedestrianBase));
        const std::optional<runtime::CharacterHitMessage>& hit = push.delivery.push.hit;
        if (!hit) std::cout << ", no hit";
        else {
            std::cout << ", hit (kind " << unsigned(hit->kind) << ", damage " << hit->damage << ", force " << hit->force << ")";
            if (push.hit) std::cout << ", outcome " << int(push.hit->outcome);
        }
        std::cout << " at update " << updatesForLog << '\n';
    }
}

}
