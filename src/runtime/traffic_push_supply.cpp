#include "runtime/traffic_push_supply.hpp"

#include <stdexcept>
#include <string>

namespace chase::runtime {

std::vector<TrafficWorldBox> trafficStaticShapeBoxes(const CollisionWorld& world) {
    std::vector<TrafficWorldBox> boxes;
    for (const CollisionWorld::Entry& entry : world.entries()) {
        if (!entry.owner.empty() || entry.mask != kStaticMask || entry.box || entry.item.nodes.empty()) continue;
        const assets::CollisionNode& root = entry.item.nodes.front();
        boxes.push_back({{root.centre[0], root.centre[1], root.centre[2]}, {root.half[0], root.half[1], root.half[2]}});
    }
    return boxes;
}

TrafficPushSupply::TrafficPushSupply(const CollisionWorld& world, const TrafficCharacterFeed& feed) : feed_(feed) {
    shapes_.flags = [this](std::uint32_t entity) { return isCar(entity) ? kTrafficCarEntityFlags : characterEntry(entity).ownerFlags; };
    shapes_.kind = [this](std::uint32_t entity) {
        if (isCar(entity)) return kTrafficCarEntityKind;
        return entity == kTrafficFeedPlayerId ? kPlayerEntityKind : kPedestrianEntityKind;
    };
    shapes_.nodeMatrix = [this](std::uint32_t entity) { return isCar(entity) ? vehicle(entity).node : characterEntry(entity).nodeMatrix; };
    shapes_.box = [this](std::uint32_t entity) {
        if (isCar(entity)) return TrafficPushBox{vehicle(entity).centre, vehicle(entity).half};
        const TrafficGridEntry& entry = characterEntry(entity);
        return TrafficPushBox{entry.shapeCentre, entry.shapeHalf};
    };
    shapes_.entryMatrix = [this](const TrafficContactEntry& entry) { return characterEntry(entry.owner).nodeMatrix; };
    shapes_.meshEntry = [](const TrafficContactEntry&) { return false; };
    shapes_.moveNode = [](std::uint32_t, const AiVec3&) {
        throw std::logic_error("traffic push supply: no node move; the half push of 0x1f49d8 needs a mover and an owner without flag 0x100");
    };
    shapes_.lastHit = [this](std::uint32_t owner) { lastHit_ = owner; };

    shapeWorld_.world = &world;
    shapeWorld_.meshOf = [](const TrafficContactEntry&) -> const CollisionWorld::Entry* { return nullptr; };
    shapeWorld_.parts = [](std::uint32_t) -> std::optional<std::vector<TrafficPushPart>> { return std::nullopt; };
    connectTrafficPushShapes(shapes_, shapeWorld_);

    world_.flags = shapes_.flags;
    world_.bit3 = [](std::uint32_t) { return false; };
    world_.ignorePair = [](std::uint32_t, std::uint32_t) { return false; };
    world_.pushVector = makeTrafficPushVector(shapes_);
    world_.message45 = [this](std::uint32_t character, std::uint32_t pusher, const AiVec3*) { messageToCharacter(character, pusher); };
    world_.isCharacter = [this](std::uint32_t entity) { return !isCar(entity); };
    world_.squash = [](std::uint32_t, std::uint32_t) {
        throw std::logic_error("traffic push supply: the squash needs a pushing character; a traffic car is none");
    };
    world_.moveNode = shapes_.moveNode;
}

void TrafficPushSupply::setCar(std::size_t car, const TrafficPushVehicle& vehicle) { vehicles_[car] = vehicle; }

TrafficPushHook TrafficPushSupply::pushOf(std::size_t car) {
    return [this, car](AiVec3& move, std::size_t entry, std::vector<TrafficContactEntry>& entries, bool slide) {
        const TrafficPushCar self{kTrafficFeedCarBase + std::uint32_t(car), kTrafficCarEntityFlags};
        return makeTrafficCarPush(self, world_)(move, entry, entries, slide);
    };
}

std::vector<TrafficCharacterPush> TrafficPushSupply::takePushes() {
    std::vector<TrafficCharacterPush> taken;
    taken.swap(pushes_);
    return taken;
}

const TrafficGridEntry& TrafficPushSupply::characterEntry(std::uint32_t entity) const {
    for (const TrafficGridEntry& entry : feed_.grid().entries)
        if (entry.owner == entity) return entry;
    throw std::logic_error("traffic push supply: no grid entry for the owner " + std::to_string(entity));
}

const TrafficPushVehicle& TrafficPushSupply::vehicle(std::uint32_t entity) const {
    const auto found = vehicles_.find(entity - kTrafficFeedCarBase);
    if (found == vehicles_.end()) throw std::logic_error("traffic push supply: no car for the owner " + std::to_string(entity));
    return found->second;
}

void TrafficPushSupply::messageToCharacter(std::uint32_t character, std::uint32_t pusher) {
    if (isCar(character)) throw std::logic_error("traffic push supply: message 45 to a car is not read (a car without flag 0x2000)");
    if (!isCar(pusher)) throw std::logic_error("traffic push supply: message 45 from a character is not supplied");
    const TrafficGridEntry& entry = characterEntry(character);
    const TrafficPushVehicle& car = vehicle(pusher);

    const bool player = character == kTrafficFeedPlayerId;
    CarPushCharacter pushed;
    pushed.entity = character;
    pushed.state = kCharacterStateIdle;
    pushed.position = entry.nodeMatrix[3];
    pushed.player = player;
    pushed.asSender.present = true;
    pushed.asSender.player = player;
    pushed.asSender.character = true;
    pushed.asSender.kind = player ? kPlayerEntityKind : kPedestrianEntityKind;

    CharacterPushSender sender;
    sender.entity = pusher;
    sender.kind = kTrafficCarEntityKind;
    sender.character = false;
    sender.node = car.node;
    sender.localPoint = car.centre;
    sender.halfLength = car.half[2];
    sender.forwardSpeed = car.forwardSpeed;

    TrafficCharacterPush push;
    push.car = pusher - kTrafficFeedCarBase;
    push.character = character;
    push.player = player;
    push.delivery = deliverCarPush(pushed, sender, std::nullopt, kCharacterStateIdle, CharacterHitSender{}, car.node[3], std::nullopt);
    if (!player) {
        auto target = targets_.find(character);
        if (target == targets_.end()) {
            CharacterHitTarget fresh;
            fresh.kind = kPedestrianEntityKind;
            target = targets_.emplace(character, fresh).first;
        }
        target->second.disabled = (entry.ownerFlags & kTrafficOwnerDisabled) != 0;
        push.hit = applyCarPushToCharacter(target->second, push.delivery);
    }
    pushes_.push_back(push);
}

}
