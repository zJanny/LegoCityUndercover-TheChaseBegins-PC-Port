#pragma once
// The traffic in chase-game (runtime/traffic_system.hpp): the Traffic%02u cars' entities, enabled at a spawn and disabled at a removal.
#include "assets/assets.hpp"
#include "assets/object_collision.hpp"
#include "game_characters.hpp"
#include "runtime/floor_walk.hpp"
#include "runtime/game_random.hpp"
#include "runtime/natives_progress.hpp"
#include "runtime/room_streaming.hpp"
#include "runtime/traffic_system.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace game_traffic {

namespace assets = chase::assets;
namespace runtime = chase::runtime;

struct SystemInputs {
    std::optional<bool> enabled;          // the last EnableTrafficAndPedestrians
    std::vector<float> maxVehicles;       // SetTrafficMaxVehicles calls since the last update
    std::vector<runtime::TrafficPathRequest> pathRequests;  // EnableTrafficPath calls since the last update
    runtime::TrafficVec3 player{};
    runtime::TrafficVec3 camera{};
    std::function<bool(int room)> roomLoaded;
    std::uint32_t update = 0;             // the update number of the log lines
    // The spawn 0x2def8c places the car's node (and wheels) at the placement; false when the car has no vehicle to place.
    std::function<bool(const std::string& instance, const runtime::TrafficCarPlacement& placement)> place;
};

class LevelTraffic {
public:
    // The level load 0x2dfa78; every car's entity is disabled. `heightOffset` is 0x20af24 of an object's vehicle.
    void load(const assets::Level& level, const std::function<float(const assets::LevelObject& object)>& heightOffset, assets::PlacedObjectState& state);
    // The room set-up 0x2dedbc with the level's own pregen file; the prefill spawns cars through `inputs.place`.
    void enterScene(assets::FuseArchive& archive, const assets::Level& level, const std::string& levelName, const runtime::StreamRooms* rooms,
                    const std::map<int, runtime::RoomBox>* boxes, int currentRoom, const runtime::FloorMesh* floor, const SystemInputs& inputs,
                    assets::PlacedObjectState& state);
    // Slot +0x48 of the system list: spawns enable a car's entity, removals disable it.
    void updateSystem(const SystemInputs& inputs, assets::PlacedObjectState& state);
    // The junction class update of the entity pass: one line per light change of a junction (not for red waiting over red).
    std::vector<runtime::TrafficJunctionLightEvent> updateJunctions(float dt, std::uint32_t update);
    // The vehicle update of the live cars: one line per stop at and start from the lights and per junction lane taken or left; a leave
    // call disables the car's entity. Returns the node matrices to set.
    std::vector<runtime::TrafficCarMove> driveCars(const runtime::TrafficDriveStep& step, std::uint32_t update, assets::PlacedObjectState& state);

    // The contact message 0x80000002 for a car's driver and the vehicle record fields the driver writes.
    void carContact(std::size_t car, bool rigidKinematic, const runtime::TrafficCarContactMessage& message, runtime::TrafficCarContactHooks hooks) {
        system_.carContact(car, rigidKinematic, message, std::move(hooks));
    }
    runtime::AiDriverVehicle& driverVehicle(std::size_t car) { return system_.driverVehicle(car); }
    const runtime::AiDriverRecord& driverRecord(std::size_t car) const { return system_.drivers().car(car).driver; }

    runtime::TrafficWhistleStop& whistle() { return system_.drivers().whistle(); }
    runtime::GameRandom& random() { return random_; }
    std::size_t carCount() const { return system_.slots().size(); }
    const std::string& instance(std::size_t car) const { return system_.slots()[car].instance; }
    // The fade byte of a car (message 0x80000007).
    std::uint8_t fade(std::size_t car) const { return system_.manager().fades()[car]; }
    std::optional<std::size_t> carOf(const std::string& lowerInstance) const;

private:
    void applyPathRequests(const SystemInputs& inputs);
    runtime::TrafficSystemStep makeStep(const SystemInputs& inputs, assets::PlacedObjectState& state);
    void report(const std::vector<runtime::TrafficEvent>& events, std::uint32_t update, assets::PlacedObjectState& state);

    runtime::TrafficSystem system_;
    std::vector<std::uint8_t> pregenFile_;
    // STAND-IN PED-S6 (shared with the pedestrians): one generator with the fixed seed of the level characters (LEVOBJ-S4), not the game's shared one.
    runtime::GameRandom random_{game_characters::kStandInSeed};
};

}
