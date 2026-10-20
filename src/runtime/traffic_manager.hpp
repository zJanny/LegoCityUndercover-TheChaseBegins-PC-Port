#pragma once
// The traffic manager, object 0x527410 of the original (vtable 0x361e34): level load 0x2dfa78, room set-up 0x2dedbc, update 0x2dfe88, spawn
// 0x2def8c, fade 0x2df8b8, SetTrafficMaxVehicles 0x2a6d70 / 0x286ca4 (docs/re/traffic.md). No SDL dependency. The level's Traffic%02u vehicles,
// spawn points, fade bounds and the per-car facts the update reads are supplied by the caller.
#include "runtime/game_random.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace chase::runtime {

using TrafficVec3 = std::array<float, 3>;

constexpr std::size_t kTrafficMaxCars = 32;
constexpr std::size_t kTrafficMaxFadeBounds = 4;
constexpr std::uint32_t kTrafficDefaultMaxVehicles = 5;
constexpr unsigned kTrafficPrefillUpdates = 64;
constexpr unsigned kTrafficSpawnTries = 10;
constexpr float kTrafficRouteSpacingSquared = 256.0f;
constexpr float kTrafficSpawnNearScale = 0.7f;
constexpr float kTrafficSpawnFarScale = 1.75f;
constexpr std::uint8_t kTrafficOpaque = 255;
constexpr std::uint8_t kTrafficFadeKeepLimit = 32;

// One entry of a spawn cell (pregen node: +0 position, +0xc direction, +0x18 route, +0x20 speed, +0x24 room).
struct TrafficSpawnPoint {
    TrafficVec3 position{};
    TrafficVec3 direction{};
    std::optional<std::uint32_t> route;
    float speed = 0.0f;
    // The room test of 0x2def8c: room object +0x24 set, 0x2284bc and the layer table 0x1ffbd8 for the car's byte +0x18.
    bool roomActive = true;
};

// The LOD distances of the table at 0x529e30 (+0x3ac near, +0x3b8 far) for one class of 0x28781c.
struct TrafficSpawnBand {
    float nearDistance = 0.0f;
    float farDistance = 0.0f;
};

// A Traffic_FadeBound%02u object: centre +0x10, half extents +0x1c..+0x24, rows of the room matrix when byte +8 is not 255.
struct TrafficFadeBound {
    TrafficVec3 centre{};
    TrafficVec3 halfExtents{};
    std::optional<std::array<TrafficVec3, 3>> rows;
};

// What the manager reads of one Traffic%02u vehicle; the manager writes the fields its own code writes.
struct TrafficCar {
    bool disabled = true;            // owner flags +0x10 bit 0
    bool pinned = false;             // owner flags +0x10 bit 0x20
    bool roomActive = true;          // the room tests of the update (0x2e00c8..0x2e0124)
    bool aiDriven = false;           // driver +0xaf bit 0
    std::uint8_t driveState = 0;     // driver +0x78
    std::uint8_t driveStage = 0;     // driver +0x79
    bool keepWhenFar = false;        // driver +0xad
    float driveValue = 0.0f;         // driver +0x68
    float cruiseSpeed = 0.0f;        // driver +0x64, the point's speed at the spawn
    std::optional<std::uint32_t> route;  // driver +4
    float despawnRadius = 0.0f;      // 0x26e130
    unsigned lodClass = 0;           // 0x28781c of the car's node
    TrafficVec3 position{};
};

enum class TrafficEventKind {
    Despawn,   // message 0x80000006, then 0x2428c0
    Spawn,     // placed at the point, message 0x80000007 with the fade, vehicle state request 1, 0x242cf0, message 0x80000005
    Fade,      // message 0x80000007 with the new fade byte
};

struct TrafficEvent {
    TrafficEventKind kind;
    std::size_t car;
    std::uint8_t fade = kTrafficOpaque;
    std::size_t point = 0;
};

struct TrafficFrame {
    TrafficVec3 player{};
    TrafficVec3 camera{};
    std::vector<TrafficSpawnPoint> cell;
    std::array<TrafficSpawnBand, 3> bands{};
    std::vector<TrafficFadeBound> fadeBounds;
    // 0x23091c, the body placement at the point; it can refuse after the car was moved.
    std::function<bool(std::size_t car, const TrafficSpawnPoint& point)> place;
};

class TrafficManager {
public:
    // 0x2dfa48: bit 1 of +0x390 set.
    TrafficManager() = default;

    // 0x2dfa78: the level's Traffic01..Traffic32 that exist, Traffic_MaxVehicles (default 5) limited to their count, the order shuffled by
    // count pairs of draws of 0x235498, every car faded in, disabled.
    void loadLevel(std::size_t carCount, std::optional<std::uint32_t> maxVehiclesAttribute, GameRandom& random);

    // 0x2dedbc after 0x2df574 found the routes and loaded the pregen data (bit 0 of +0x390): countdown 0, 64 updates with the player as focus.
    void startRoom(bool routesLoaded, std::uint32_t routeCount, const TrafficFrame& frame, GameRandom& random, std::vector<TrafficEvent>* events);

    // 0x2dfe88.
    void update(const TrafficFrame& frame, GameRandom& random, std::vector<TrafficEvent>* events);

    // 0x29f09c (EnableTrafficAndPedestrians): bit 1 of +0x390.
    void setEnabled(bool on) { enabled_ = on; }
    bool enabled() const { return enabled_; }
    // 0x2dfe78 clears bit 0.
    void unloadRoutes() { routesLoaded_ = false; }
    bool routesLoaded() const { return routesLoaded_; }

    // 0x2a6d70: the argument converted by vcvt.s32, then 0x286ca4 limits it (unsigned) to the car count.
    void setMaxVehicles(float argument);
    std::uint32_t maxVehicles() const { return maxVehicles_; }

    void setRouteEnabled(std::uint32_t route, bool on);
    bool routeEnabled(std::uint32_t route) const;

    std::vector<TrafficCar>& cars() { return cars_; }
    const std::vector<TrafficCar>& cars() const { return cars_; }
    const std::vector<std::uint32_t>& order() const { return order_; }
    const std::vector<std::uint8_t>& fades() const { return fades_; }
    std::uint32_t countdown() const { return countdown_; }
    bool prefilling() const { return prefill_; }

    // 0x2def8c.
    bool trySpawn(std::size_t car, const TrafficFrame& frame, GameRandom& random, std::vector<TrafficEvent>* events);
    // 0x2df8b8.
    void refreshFade(std::size_t car, const TrafficFrame& frame, std::vector<TrafficEvent>* events);

private:
    void despawnCar(std::size_t car, std::vector<TrafficEvent>* events);
    void lowerCountdown(std::uint32_t live);

    std::vector<TrafficCar> cars_;
    std::vector<std::uint32_t> order_;
    std::vector<std::uint8_t> fades_;
    std::vector<bool> routes_;
    std::uint32_t maxVehicles_ = 0;
    std::uint32_t countdown_ = 0;
    bool enabled_ = true;
    bool routesLoaded_ = false;
    bool prefill_ = false;
};

// SetTrafficMaxVehicles with the argument numbers of a script call; a call without exactly one number is refused with an error naming the
// native.
void runSetTrafficMaxVehicles(TrafficManager& manager, const std::vector<float>& values);

// The distance band of 0x2def8c: 0 to the far distance while prefilling, else 0.7 x near to 1.75 x far.
TrafficSpawnBand trafficSpawnBand(const TrafficSpawnBand& lod, bool prefill);

}
