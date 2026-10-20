#pragma once
// The pedestrians of a level as the game runs them (docs/re/pedestrians.md): the manager 0x51b33c (PedestrianManager) with the
// spawn data its scene enter 0x2edbf8 loads (step 0x2edc94, pregen loader 0x2ee868), and the update of each pedestrian
// character (kind 197, PedestrianWalk). The manager update is slot +0x48 of the system list (0x169374, before the runner); the
// characters update in the entity pass 0x1a3f80. The caller owns the entities: it disables a slot's entity at the level load and
// at a removal (0x2428c0), enables it at a spawn (0x242cf0) and plays the clips the characters request. No SDL dependency.
#include "assets/assets.hpp"
#include "assets/pregen_traffic.hpp"
#include "runtime/game_random.hpp"
#include "runtime/pedestrian_manager.hpp"
#include "runtime/pedestrian_walk.hpp"
#include "runtime/pregen_derive.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr std::size_t kPedestrianPathNames = 128;
constexpr const char* kPedestrianSystemClass = "props/pedestriansystem";
constexpr const char* kPedestrianCharacterClass = "chars/pedestrian";

// "PedestrianPath001" .. "PedestrianPath128" (format "PedestrianPath%03u" at 0x358a3c).
std::string pedestrianPathName(std::size_t number);
// 0x2edc94: how many of the names 1 .. 128 are paths (0x22a3b0: the path section of a level by name hash) of a loaded level; a
// name counts once, for the first level in list order that has it.
std::size_t pedestrianPathCount(const std::vector<const assets::Level*>& loadedLevels);
// Kind 198: the create 0x260d9c of a `props/pedestriansystem` object stores it at manager +0x94.
bool levelHasPedestrianSystem(const assets::Level& level);

// What 0x2edbf8 reads on a scene enter. `loadedLevels` is the list 0x589a18 as 0x1ed57c rebuilt it just before (0x2e44e0 before
// the level enter 0x2e6264, 0x30360c in the transition 0x303480); `rooms.currentRoom` is the room of `Player1Start` (0x2e6354) or
// of the transition's destination (0x30362c). `pregenFile` is null when `Pregen/Pedestrians/<level>_Pedestrians.bin` is missing.
struct PedestrianSceneEnter {
    std::vector<const assets::Level*> loadedLevels;
    bool pedestrianSystem = false;
    const std::vector<std::uint8_t>* pregenFile = nullptr;
    PregenRoomContext rooms;
};

// One pedestrian slot as the system creates it at the level load (create 0x1913f8).
struct PedestrianSlotInfo {
    std::string instance;
    bool isMale = true;
    // Entity +0x5c: pedestrianEntityBox(...).radius of the body model bounds, supplied by the caller.
    float arrivalRadius = 0;
};

// The manager update's view of the game. Rooms are the port's room numbers (0 = none).
struct PedestrianSystemStep {
    PedestrianView view;
    PedestrianVec3 playerPosition{};  // the player node position of the leaf lookup 0x1eb490
    std::function<bool(std::size_t slot)> entityEnabled;
    std::function<std::optional<PedestrianRoom>(int room)> room;
    DistanceFadeTable fade;
    PedestrianLayerTable layers{};
    std::uint32_t meshNodeType = 0;
    std::function<bool(std::size_t slot, const PedestrianVec3& position)> spawnBlocked;
};

// The entity pass's view of the game for the pedestrian characters (PedestrianWalkWorld per slot).
struct PedestrianCharacterStep {
    float dt = 0;
    PedestrianVec3 playerPosition{};
    bool playerInVehicle = false;
    std::uint16_t playerState = 0;
    std::function<bool(std::size_t slot)> visible;
    std::function<bool(std::size_t slot)> hidden;
    std::function<int(std::size_t slot, const PedestrianVec3& position, const PedestrianVec3& velocity, float dt)> blocked;
    std::function<void(std::size_t slot, PedestrianVec3& position)> groundProbe;
    std::function<std::optional<float>(const PedestrianVec3& from, const PedestrianVec3& to)> groundRay;
    std::function<bool(std::size_t slot, PedestrianClip clip)> clipFinished;
};

struct PedestrianCharacterUpdate {
    std::size_t slot = 0;
    PedestrianWalkUpdate walk;
};

class PedestrianSystem {
public:
    // 0x2ee4b4.
    void init() { manager_.init(); }
    // 0x2ee4c8 for one loaded scene, with the create 0x1913f8 of each slot's character. The caller disables every slot's entity.
    // Returns the slots in manager order.
    const std::vector<PedestrianSlotInfo>& loadLevel(const assets::Level& level, const assets::ClassLookup& classOf,
                                                     const std::function<float(const assets::LevelObject& object)>& arrivalRadius);
    // 0x2edbf8: 0x2edc94 (paths, pregen load, spawn tree), then the countdown 0. Throws for a malformed pregen file.
    void enterScene(const PedestrianSceneEnter& enter);
    // 0x2edc10.
    void leaveScene();
    // 0x2b476c (EnableTrafficAndPedestrians).
    void setEnabled(bool enabled) { manager_.setEnabled(enabled); }

    // Slot +0x48, the manager update 0x2ee5e8.
    PedestrianUpdate update(const PedestrianSystemStep& step, GameRandom& random);
    // The slot 0xc update 0x19188c of every live pedestrian, in slot order.
    std::vector<PedestrianCharacterUpdate> updateCharacters(const PedestrianCharacterStep& step, GameRandom& random);

    const std::vector<PedestrianSlotInfo>& slots() const { return slots_; }
    const PedestrianManager& manager() const { return manager_; }
    const PedestrianBody& body(std::size_t slot) const { return bodies_[slot]; }
    int room(std::size_t slot) const { return rooms_[slot]; }
    void setRoom(std::size_t slot, int room) { rooms_[slot] = room; }
    std::size_t spawnPointCount() const { return points_.size(); }
    // The leaf of the spawn tree that holds (x, z) (0x1eb490), as spawn points with their rooms; empty without spawn data.
    std::vector<PedestrianSpawnPoint> leafAt(float x, float z, const std::function<std::optional<PedestrianRoom>(int room)>& room) const;

private:
    struct SpawnPoint {
        PedestrianVec3 position{};
        PedestrianVec3 direction{};
        std::uint32_t path = 0;
        float pathParameter = 0;
        int room = 0;
    };

    PedestrianManager manager_;
    std::vector<PedestrianSlotInfo> slots_;
    std::vector<PedestrianBody> bodies_;
    std::vector<int> rooms_;
    assets::PregenSpawns spawns_;
    std::vector<SpawnPoint> points_;
    std::vector<PedestrianPath> paths_;
};

// The room objects of the port's rooms as the room test reads them: a loaded room is state 2 with its resource. The pedestrian
// node hangs on room +0x14 after the spawn (0x245008); the removal compares its parent with room +0x7c.
// STAND-IN PED-S2: the room type byte +0x3a is 0, and +0x14 and +0x7c are taken to be different nodes.
PedestrianRoom pedestrianRoomObject(int room, bool loaded);
std::uint32_t pedestrianRoomAttachNode(int room);

}
