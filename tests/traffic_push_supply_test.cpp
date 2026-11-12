// Tests of src/runtime/traffic_push_supply.*: the grid's shape boxes and a traffic car's push of the characters of a feed (0x2baef0 with message
// 45, docs/re/traffic-car-push.md). The data is made up; every expected value is evaluated by hand from the code read by 20261112-enemy-C on
// 2026-11-12 (llvm-objdump --triple=armv6k-none-eabi --mattr=+vfp2 of artifacts/game/game.arm.elf: 0x2baef0, 0x1f49d8, 0x283d0c, 0x30e0d8). No
// emulator was used.
#include "runtime/traffic_push_supply.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

using namespace chase::runtime;

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

template <typename Function>
bool throws(Function function) {
    try {
        function();
    } catch (const std::logic_error&) {
        return true;
    }
    return false;
}

AiMatrix at(float x, float y, float z) {
    AiMatrix matrix{};
    matrix[0] = {1.0f, 0.0f, 0.0f};
    matrix[1] = {0.0f, 1.0f, 0.0f};
    matrix[2] = {0.0f, 0.0f, 1.0f};
    matrix[3] = {x, y, z};
    return matrix;
}

TrafficGridLayout tenMetreLayout() {
    TrafficGridLayout layout;
    layout.origin = {0.0f, 0.0f, 0.0f};
    layout.inverseCell = {0.1f, 0.1f, 0.1f};
    layout.counts = {4, 2, 4};
    return layout;
}

TrafficFedCharacter character(std::uint32_t id, float x, float y, float z) {
    TrafficFedCharacter fed;
    fed.id = id;
    fed.node = at(x, y, z);
    fed.shapeCentre = {0.0f, 0.0f, 0.0f};
    fed.shapeHalf = {0.5f, 1.0f, 0.5f};
    return fed;
}

// A car at (5, 1, 3) facing +z with the box half (1, 1, 2): its moved box reaches z 5.
TrafficPushVehicle car(float forwardSpeed) {
    TrafficPushVehicle vehicle;
    vehicle.node = at(5.0f, 1.0f, 3.0f);
    vehicle.centre = {0.0f, 0.0f, 0.0f};
    vehicle.half = {1.0f, 1.0f, 2.0f};
    vehicle.forwardSpeed = forwardSpeed;
    return vehicle;
}

void testStaticShapeBoxes() {
    CollisionWorld world;
    chase::assets::CollisionItem level;
    chase::assets::CollisionNode root;
    root.centre[0] = 10.0f;
    root.centre[1] = 2.0f;
    root.centre[2] = -4.0f;
    root.half[0] = 5.0f;
    root.half[1] = 1.0f;
    root.half[2] = 3.0f;
    level.nodes.push_back(root);
    world.add(level);
    world.add(level, kStaticMask << 4);
    CollisionWorld::Entry placed;
    placed.item = level;
    placed.owner = "prop_a";
    world.mutableEntries().push_back(placed);
    chase::assets::CollisionItem empty;
    world.add(empty);

    const std::vector<TrafficWorldBox> boxes = trafficStaticShapeBoxes(world);
    require(boxes.size() == 1, "only the static item without an owner and with a root node is a shape");
    require(boxes[0].centre == AiVec3({10.0f, 2.0f, -4.0f}) && boxes[0].half == AiVec3({5.0f, 1.0f, 3.0f}), "its box is the root node's");
}

struct Scene {
    CollisionWorld world;
    TrafficCharacterFeed feed{tenMetreLayout()};
    std::vector<TrafficContactEntry> gathered(const AiVec3& centre, const AiVec3& half) {
        const TrafficContactGather gather = makeTrafficContactGather(feed.grid(), [](std::size_t c) { return kTrafficFeedCarBase + std::uint32_t(c); });
        return gather(0, centre, half, TrafficContactDescriptor{}, 128);
    }
};

// A car with flags 0x1300 is a pusher: the contact slide has no slide, 0x2baef0 sends message 45 and returns 1. A car moving forward at 5 or
// more with the character in front hits it (kind 3, weapon 1, force the speed, damage 0 for a pedestrian, 1 for the player).
void testPedestrianInFrontOfTheCar() {
    Scene scene;
    scene.feed.update({character(kTrafficFeedPedestrianBase + 3, 5.0f, 1.0f, 6.0f), character(kTrafficFeedPlayerId, 25.0f, 1.0f, 25.0f)});
    TrafficPushSupply supply(scene.world, scene.feed);
    supply.setCar(0, car(8.0f));

    std::vector<TrafficContactEntry> entries = scene.gathered({5.0f, 1.0f, 3.0f}, {2.0f, 2.0f, 3.0f});
    require(entries.size() == 1 && entries[0].owner == kTrafficFeedPedestrianBase + 3, "the car's box gathers the pedestrian");
    AiVec3 move = {0.0f, 0.0f, 1.0f};
    require(supply.pushOf(0)(move, 0, entries, false), "message 45 without slide gives 1");
    require(move == AiVec3({0.0f, 0.0f, 1.0f}), "the box test leaves the move");

    const std::vector<TrafficCharacterPush> pushes = supply.takePushes();
    require(pushes.size() == 1 && pushes[0].car == 0 && pushes[0].character == kTrafficFeedPedestrianBase + 3 && !pushes[0].player, "one push of the pedestrian");
    require(pushes[0].delivery.push.hit.has_value(), "a car at 8 with the pedestrian 2 ahead of its front point hits it");
    const CharacterHitMessage& hit = *pushes[0].delivery.push.hit;
    require(hit.kind == kCarPushHitKind && hit.weapon == kCarPushHitWeapon && hit.force == 8.0f && hit.damage == 0, "kind 3, weapon 1, force 8, no damage");
    require(hit.sender.kind == kTrafficCarEntityKind && !hit.sender.character, "the sender is the vehicle kind 0xb7");
    require(pushes[0].hit.has_value(), "the pedestrian's hit target takes the hit");
    require(supply.takePushes().empty(), "the pushes are taken once");

    scene.feed.update({character(kTrafficFeedPedestrianBase + 3, 5.0f, 1.0f, 6.0f), character(kTrafficFeedPlayerId, 5.0f, 1.0f, 6.0f)});
    std::vector<TrafficContactEntry> both = scene.gathered({5.0f, 1.0f, 3.0f}, {2.0f, 2.0f, 3.0f});
    require(both.size() == 2, "the player beside the pedestrian is gathered too");
    for (std::size_t i = 0; i < both.size(); ++i) {
        AiVec3 again = {0.0f, 0.0f, 1.0f};
        require(supply.pushOf(0)(again, i, both, false), "both are pushed");
    }
    const std::vector<TrafficCharacterPush> twice = supply.takePushes();
    require(twice.size() == 2, "two pushes");
    const TrafficCharacterPush& playerPush = twice[0].player ? twice[0] : twice[1];
    require(playerPush.player && !playerPush.hit.has_value() && playerPush.delivery.push.hit && playerPush.delivery.push.hit->damage == 1,
            "the player gets the hit with damage 1 and no hit target of the supply");
}

void testReversingCarDoesNotHit() {
    Scene scene;
    scene.feed.update({character(kTrafficFeedPedestrianBase, 5.0f, 1.0f, 6.0f)});
    TrafficPushSupply supply(scene.world, scene.feed);
    supply.setCar(0, car(-2.0f));
    std::vector<TrafficContactEntry> entries = scene.gathered({5.0f, 1.0f, 3.0f}, {2.0f, 2.0f, 3.0f});
    AiVec3 move = {0.0f, 0.0f, 1.0f};
    require(supply.pushOf(0)(move, 0, entries, false), "the push still sends message 45");
    const std::vector<TrafficCharacterPush> pushes = supply.takePushes();
    require(pushes.size() == 1 && !pushes[0].delivery.push.hit && !pushes[0].hit, "a reversing car (speed below 5) does not hit");
}

void testNoPushWithoutOverlap() {
    Scene scene;
    scene.feed.update({character(kTrafficFeedPedestrianBase, 5.0f, 1.0f, 9.0f)});
    TrafficPushSupply supply(scene.world, scene.feed);
    supply.setCar(0, car(8.0f));
    std::vector<TrafficContactEntry> entries = scene.gathered({5.0f, 1.0f, 3.0f}, {2.0f, 2.0f, 8.0f});
    require(entries.size() == 1, "a long box gathers the far pedestrian");
    AiVec3 move = {0.0f, 0.0f, 1.0f};
    require(!supply.pushOf(0)(move, 0, entries, false), "the moved car box z 2..6 does not reach z 8.5");
    require(supply.takePushes().empty(), "no message 45");
}

void testMissingData() {
    Scene scene;
    scene.feed.update({character(kTrafficFeedPedestrianBase, 5.0f, 1.0f, 6.0f)});
    TrafficPushSupply supply(scene.world, scene.feed);
    std::vector<TrafficContactEntry> entries = scene.gathered({5.0f, 1.0f, 3.0f}, {2.0f, 2.0f, 3.0f});
    AiVec3 move = {0.0f, 0.0f, 1.0f};
    require(throws([&] { supply.pushOf(0)(move, 0, entries, false); }), "a car that was never set is an error, not a silent skip");
}

}

int main() {
    try {
        testStaticShapeBoxes();
        testPedestrianInFrontOfTheCar();
        testReversingCarDoesNotHit();
        testNoPushWithoutOverlap();
        testMissingData();
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "traffic push supply: ok\n";
    return 0;
}
