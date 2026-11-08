#pragma once
// The traffic of a level as the game runs it (docs/re/traffic.md): the manager 0x527410 (TrafficManager) fed with the spawn data its room
// set-up 0x2dedbc loads (pregen loader 0x2e02d4) and the level's Traffic%02u vehicles. The manager update is slot +0x48 of the system list
// (0x169374, before the runner). The caller owns the entities: it disables a car's entity at the level load and at a removal (0x2428c0),
// enables it at a spawn (0x242cf0) and places it. No SDL dependency.
#include "assets/assets.hpp"
#include "assets/pregen_traffic.hpp"
#include "runtime/game_random.hpp"
#include "runtime/pedestrian_manager.hpp"
#include "runtime/pregen_derive.hpp"
#include "runtime/room_layers.hpp"
#include "runtime/traffic_driver_connect.hpp"
#include "runtime/traffic_junction_update.hpp"
#include "runtime/traffic_manager.hpp"
#include "runtime/traffic_route_setup.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr std::size_t kTrafficCarNames = 32;
constexpr std::size_t kTrafficFadeBoundNames = 4;

// "Traffic01" .. "Traffic32" (format "Traffic%02u" at 0x2dfc4c) and "Traffic_FadeBound01" .. "Traffic_FadeBound04" (0x2dfc6c).
std::string trafficCarName(std::size_t number);
std::string trafficFadeBoundName(std::size_t number);

// Measured on the ten Traffic cars of lvl_01 (20261025-traffic-C-1, recorded from a save state on the street after mission 1-1, 2026-10-25): the layer class of the room tests
// (entity byte +0x18) and the node of the 0x28781c class (word +0 with bit 9, byte +0xde 0x31: bit 5, "mid", so class 1).
constexpr std::uint8_t kTrafficCarLayerClass = 0;
constexpr std::uint32_t kTrafficCarNodeWord = 0x4202;
constexpr std::uint8_t kTrafficCarNodeFadeFlags = 0x31;
PedestrianNode trafficCarNode();

// STAND-IN TRAFFIC-S1: the removal radius 0x26e130 of every car (byte +0xb of the row record +0x20 of the table *0x375f40; 50 for all ten
// cars of lvl_01, 20261025-traffic-C-1) and the room-type byte of the cars' rooms (0, as PED-S2).
constexpr float kTrafficStandInDespawnRadius = 50.0f;

// One Traffic%02u car as the level load 0x2dfa78 collects it.
struct TrafficCarSlot {
    std::string instance;
    // 0x20af24: driver +0x74 (class value TrafficHeightOffRoad) - body +0x54 (CentreOfMass y), added to the spawn point's y.
    float heightOffset = 0;
};

// What the spawn 0x2def8c does to the car's node (steps 1 to 5): the matrix is built from the point's direction (0x237a0c) with the
// position raised by `heightOffset`.
struct TrafficCarPlacement {
    std::size_t car = 0;
    TrafficVec3 position{};
    TrafficVec3 direction{};
    float pathParameter = 0;
    int room = 0;
    int floorCell = -1;
    std::size_t point = 0;
    std::uint32_t route = 0;
};

// What 0x2edbf8's counterpart, the room set-up 0x2dedbc, reads on a scene enter. `pregenFile` is null when
// `Pregen/Traffic/<level>_Traffic.bin` is missing.
struct TrafficSceneEnter {
    std::vector<const assets::Level*> loadedLevels;
    const std::vector<std::uint8_t>* pregenFile = nullptr;
    PregenRoomContext rooms;
    const FloorMesh* floor = nullptr;
};

// The manager update's view of the game. Rooms are the port's room numbers (0 = none).
struct TrafficSystemStep {
    TrafficVec3 player{};
    TrafficVec3 camera{};
    std::function<bool(std::size_t car)> entityEnabled;
    std::function<std::optional<PedestrianRoom>(int room)> room;
    DistanceFadeTable lod;
    PedestrianLayerTable layers{};
    float despawnRadius = kTrafficStandInDespawnRadius;
    std::function<bool(const TrafficCarPlacement& placement)> place;
};

class TrafficSystem {
public:
    // 0x2dfa78 for one level: the cars, the limit, the shuffled spawn order and the fade bounds. The caller disables every car's entity.
    const std::vector<TrafficCarSlot>& loadLevel(const assets::Level& level, const std::function<float(const assets::LevelObject& object)>& heightOffset,
                                                 GameRandom& random);
    // 0x2dedbc: the pregen load and route set-up, then the 64 prefill updates. Throws for a malformed pregen file.
    std::vector<TrafficEvent> enterScene(const TrafficSceneEnter& enter, const TrafficSystemStep& step, GameRandom& random);
    // 0x2dfe78.
    void leaveScene();
    // 0x29f09c (EnableTrafficAndPedestrians).
    void setEnabled(bool enabled) { manager_.setEnabled(enabled); }
    // 0x2a6d70 (SetTrafficMaxVehicles); throws unless exactly one number is given.
    void setMaxVehicles(const std::vector<float>& values) { runSetTrafficMaxVehicles(manager_, values); }

    // Slot +0x48, the manager update 0x2dfe88.
    std::vector<TrafficEvent> update(const TrafficSystemStep& step, GameRandom& random);
    // The vehicle update of the live cars (TrafficDrivers::drive) over the junctions with the manager's route bits.
    // STAND-IN TRAFFIC-S27: the caller runs it after the junction update of the entity pass.
    TrafficDriveResult driveCars(const TrafficDriveStep& step, GameRandom& random);
    // The junction class update 0x1d8924 of the entity pass, after the runner (runTrafficJunctionUpdates), with the manager's route bits.
    std::vector<TrafficJunctionLightEvent> updateJunctions(float dt);

    const std::vector<TrafficCarSlot>& slots() const { return slots_; }
    const TrafficManager& manager() const { return manager_; }
    std::size_t spawnPointCount() const { return points_.size(); }
    std::size_t routeCount() const { return manager_.routeTable().routes.size(); }
    // The junctions and the route -> junction pairs of the last scene enter (TRAFFIC-2 reads them).
    const TrafficRoutes& routes() const { return routes_; }
    // The junction records kept from the last scene enter and their route paths.
    const TrafficJunctions& junctions() const { return junctions_; }
    // EnableTrafficPath (0x27bc30 -> 0x2dece4): before the scene enter a disable is queued and replayed by it.
    void enablePath(const std::string& name, bool on) { runEnableTrafficPath(manager_.routeTable(), name, on); }
    std::size_t floorCellsFound() const;
    // The point of the loaded file that a spawn event's cell index names.
    std::size_t pointOfCell(std::size_t cellIndex) const { return cellPoints_.at(cellIndex); }
    TrafficVec3 carPosition(std::size_t car) const { return manager_.cars().at(car).position; }
    const TrafficDrivers& drivers() const { return drivers_; }

private:
    struct SpawnPoint {
        TrafficVec3 position{};
        TrafficVec3 direction{};
        std::optional<std::uint32_t> route;
        float pathParameter = 0;
        int room = 0;
        int floorCell = -1;
    };

    TrafficFrame makeFrame(const TrafficSystemStep& step);
    void syncCars(const TrafficSystemStep& step);

    TrafficManager manager_;
    std::vector<TrafficCarSlot> slots_;
    std::vector<TrafficFadeBound> fadeBounds_;
    std::vector<int> carRooms_;
    TrafficDrivers drivers_;
    assets::PregenSpawns spawns_;
    std::vector<SpawnPoint> points_;
    std::vector<std::size_t> cellPoints_;
    TrafficRoutes routes_;
    TrafficJunctions junctions_;
};

}
