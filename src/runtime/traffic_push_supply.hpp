#pragma once
// What chase-game gives the traffic cars' push of characters (docs/re/traffic-character-gather.md, docs/re/traffic-car-push.md): the shape boxes
// that bound the contact grid, and the world of 0x2baef0 (TrafficPushWorld, TrafficPushShapes, TrafficPushShapeWorld) over the characters of the
// grid and the traffic cars, with message 45 delivered through deliverCarPush. No SDL dependency.
#include "runtime/collision.hpp"
#include "runtime/script_placement.hpp"
#include "runtime/traffic_car_push.hpp"
#include "runtime/traffic_character_feed.hpp"
#include "runtime/traffic_push_shapes.hpp"
#include "runtime/traffic_push_world.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <vector>

namespace chase::runtime {

// Owner flags and kind of a traffic car's entity: hword +0x10 is 0x1300 for a traffic vehicle (0x1301 while disabled) and byte +0x12 is 0xb7 for
// the traffic van (docs/re/vehicles-system.md, docs/re/vehicles-boarding.md). STAND-IN TRAFFIC-S47: the other traffic classes' kind is not read.
constexpr std::uint16_t kTrafficCarEntityFlags = 0x1300;
constexpr std::uint8_t kTrafficCarEntityKind = 183;
constexpr std::uint8_t kPedestrianEntityKind = 197;   // PedestrianWalk's class

// The first bound set of the grid build 0x271280: the world boxes of the level's static shapes. STAND-IN TRAFFIC-S46: the scene's list +0x1308
// and the rooms' item lists +0x24 are the entries of the level's static collision (0x30e0d8 registers them as mask 1 entries); the port's
// static items (no owner, mask 1) stand for them with the box of their root node.
std::vector<TrafficWorldBox> trafficStaticShapeBoxes(const CollisionWorld& world);

// One traffic car as 0x2baef0 and message 45 read it.
struct TrafficPushVehicle {
    AiMatrix node{};          // 0x250810 of the car's node
    AiVec3 centre{};          // entity +0x60
    AiVec3 half{};            // entity +0x6c
    float forwardSpeed = 0.0f;  // 0x229948: signed, negative when reversing
};

// A message 45 that reached a character.
struct TrafficCharacterPush {
    std::size_t car = 0;
    std::uint32_t character = 0;        // the owner identity of the feed
    bool player = false;
    CarPushDelivery delivery;
    std::optional<CharacterHitResult> hit;   // applyCarPushToCharacter on the pedestrian's hit target; none for the player
};

// The push hook of each traffic car over the characters of a feed. `world` and `feed` must outlive it; the feed's entries are read when a push
// runs. STAND-IN TRAFFIC-S46: the pair test 0x1fc508 ignores no pair, the pushed character's word +0xc bit 3 is clear, and a character state
// is the idle state (the pedestrians' states are not read), so events 17 and 18 have no listener. STAND-IN TRAFFIC-S47: what a pedestrian or
// the player does with the hit is not ported (no pedestrian hit reaction, no player hit): the hit goes to a hit target kept per pedestrian
// and its outcome is reported.
class TrafficPushSupply {
public:
    TrafficPushSupply(const CollisionWorld& world, const TrafficCharacterFeed& feed);
    TrafficPushSupply(const TrafficPushSupply&) = delete;
    TrafficPushSupply& operator=(const TrafficPushSupply&) = delete;

    void setCar(std::size_t car, const TrafficPushVehicle& vehicle);
    TrafficPushHook pushOf(std::size_t car);
    std::vector<TrafficCharacterPush> takePushes();

private:
    bool isCar(std::uint32_t entity) const { return entity >= kTrafficFeedCarBase; }
    const TrafficGridEntry& characterEntry(std::uint32_t entity) const;
    const TrafficPushVehicle& vehicle(std::uint32_t entity) const;
    void messageToCharacter(std::uint32_t character, std::uint32_t pusher);

    const TrafficCharacterFeed& feed_;
    std::map<std::size_t, TrafficPushVehicle> vehicles_;
    std::map<std::uint32_t, CharacterHitTarget> targets_;
    std::vector<TrafficCharacterPush> pushes_;
    std::uint32_t lastHit_ = 0;
    TrafficPushShapes shapes_;
    TrafficPushShapeWorld shapeWorld_;
    TrafficPushWorld world_;
};

}
