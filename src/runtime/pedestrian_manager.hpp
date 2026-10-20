#pragma once
// The pedestrian manager 0x51b33c (vtable 0x362850, docs/re/pedestrians.md): init 0x2ee4b4, level load 0x2ee4c8, enable
// 0x2b476c (EnableTrafficAndPedestrians), spawn data flag from 0x2edc94, update 0x2ee5e8 (vtable +0x48) and spawn 0x2ee068.
// The caller supplies what the update reads from the rest of the game: the spawn points of the pregen file's leaf, the
// view, the room and node tests of each pedestrian. No SDL dependency.
#include "runtime/game_random.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

using PedestrianVec3 = std::array<float, 3>;

constexpr std::size_t kPedestrianSlotCount = 20;
constexpr unsigned kPedestrianSpawnTries = 11;
constexpr std::uint32_t kPedestrianCountdownPerLive = 7;
constexpr std::uint32_t kPedestrianDirectionRange = 20;
constexpr std::uint32_t kPedestrianDirectionForwardFrom = 10;
constexpr float kPedestrianViewAheadPerSpeed = 0.5f;
constexpr float kPedestrianSpawnFacing = 0.8f;
constexpr float kPedestrianMinDistanceScale = 0.5f;
constexpr float kPedestrianMaxDistanceScale = 1.5f;
constexpr float kPedestrianSpacingSquared = 64.0f;
constexpr float kPedestrianSpeedBase = 2.0f;
constexpr float kPedestrianSpeedRange = 1.5f;
constexpr float kPedestrianLateralScale = 0.5f;

// "Pedestrian01" .. "Pedestrian20" (format "Pedestrian%02u" at 0x2ee558).
std::string pedestrianSlotName(std::size_t number);

// Words 0x52a1d0 .. 0x52a1f0 written by 0x1077dc: the three radius tiers, the fade-in and the fade-out distances of the
// distance fade (docs/re/script-natives-world.md). Defaults are the values read in the oracle.
struct DistanceFadeTable {
    std::array<float, 3> radius{3.0f, 6.0f, 12.0f};
    std::array<float, 3> fadeIn{36.0f, 72.0f, 108.0f};
    std::array<float, 3> fadeOut{38.0f, 76.0f, 114.0f};

    float word(int index) const;
};

// Spawn distance window of 0x2ee068 for the fade tier of 0x28781c (-1 .. 2): the fade-in word at 0x52a1dc + 4 tier times
// 0.5 and the fade-out word at 0x52a1e8 + 4 tier times 1.5 (tier -1 reads the word before each table).
struct PedestrianSpawnWindow {
    float minimum = 0;
    float maximum = 0;
};
PedestrianSpawnWindow pedestrianSpawnWindow(const DistanceFadeTable& table, int tier);

// One 40-byte record of `Pregen/Pedestrians/<level>_Pedestrians.bin` after 0x2ee868 resolved it: position +0, direction
// +0xc, path object +0x18 (0 = not found), path parameter +0x20, room +0x24 (`roomActive` = 0x2284bc of the room and
// 0x1ffbd8 of the pedestrian's class with the room type).
struct PedestrianSpawnPoint {
    PedestrianVec3 position{};
    PedestrianVec3 direction{};
    std::uint32_t path = 0;
    float pathParameter = 0;
    bool roomActive = false;
};

// The fields of a pedestrian entity and its component (+0x7c) that the manager reads and writes.
struct Pedestrian {
    bool live = false;
    std::uint16_t state = 0;
    std::uint16_t requested = 0;
    std::uint32_t hold = 0;
    std::uint32_t path = 0;
    bool forward = false;
    float lateral = 0;
    float speed = 0;
    float pathParameter = 0;
    bool fadeStarted = false;
    std::array<PedestrianVec3, 4> placement{};
};

// What 0x2ee068 reads of the view: the camera matrix (0x2429d0 of camera 0x22e1f4(0)) rows 2 and 3, and the speed of the
// player's vehicle body (0x34163c; 0 on foot or without a valid player).
struct PedestrianView {
    PedestrianVec3 position{};
    PedestrianVec3 row2{};
    float playerVehicleSpeed = 0;
};

struct PedestrianWorld {
    PedestrianView view;
    // The leaf of the spawn tree at +0x24 that holds the player's position (0x1eb490); null when there is none.
    const std::vector<PedestrianSpawnPoint>* spawnPoints = nullptr;
    DistanceFadeTable fade;
    std::function<int(std::size_t slot)> fadeTier;
    std::function<bool(std::size_t slot, const PedestrianVec3& position)> spawnBlocked;
    std::function<bool(std::size_t slot)> roomActive;
    std::function<bool(std::size_t slot)> onRoomRoot;
    std::function<bool(std::size_t slot)> hidden;
    std::function<bool(std::size_t slot)> fadeNode;
};

struct PedestrianRemoval {
    std::size_t slot = 0;
    bool stopAnimations = false;
};

struct PedestrianUpdate {
    std::optional<std::size_t> spawned;
    std::vector<PedestrianRemoval> removed;
};

class PedestrianManager {
public:
    // 0x2ee4b4: clears bit 0 and sets bit 1 of +0xa4.
    void init();
    // 0x2ee4c8: the slots are the level objects "Pedestrian01" .. "Pedestrian20" that exist, in that order; each is
    // disabled. The limit +0xa0 is the level parameter "Pedestrians_Max". Returns the numbers of the slots found.
    std::vector<std::size_t> loadLevel(const std::function<bool(const std::string& name)>& exists, std::uint32_t limit);
    // 0x2edc94: bit 0 of +0xa4 is the result of the pregen file load 0x2ee868.
    void setSpawnDataLoaded(bool loaded);
    // 0x2b476c: bit 1 of +0xa4.
    void setEnabled(bool enabled);

    PedestrianUpdate update(const PedestrianWorld& world, GameRandom& random);

    bool enabled() const { return (flags_ & 2u) != 0; }
    bool spawnDataLoaded() const { return (flags_ & 1u) != 0; }
    std::uint32_t limit() const { return limit_; }
    std::uint32_t countdown() const { return countdown_; }
    void setCountdown(std::uint32_t value) { countdown_ = value; }
    std::size_t liveCount() const;
    std::vector<Pedestrian>& pedestrians() { return pedestrians_; }
    const std::vector<Pedestrian>& pedestrians() const { return pedestrians_; }

private:
    bool spawn(std::size_t slot, const PedestrianWorld& world, GameRandom& random);
    void lowerCountdown(std::uint32_t value);

    std::vector<Pedestrian> pedestrians_;
    std::uint8_t flags_ = 0;
    std::uint32_t limit_ = 0;
    std::uint32_t countdown_ = 0;
};

// 0x237a0c on a matrix whose row 2 is `facing` and row 3 `position`: row 0 from row 2 (|y| <= 0.98: (z, 0, -x), else
// (0, -z, y)), normalized by 0x257ea0, row 1 = row 2 x row 0 (0x23b960).
std::array<PedestrianVec3, 4> pedestrianPlacement(const PedestrianVec3& position, const PedestrianVec3& facing);

}
