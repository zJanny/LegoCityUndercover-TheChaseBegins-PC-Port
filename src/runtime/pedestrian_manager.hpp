#pragma once
// The pedestrian manager 0x51b33c (vtable 0x362850, docs/re/pedestrians.md): init 0x2ee4b4, level load 0x2ee4c8 (+0x1c, once per loaded
// scene from 0x27ecb8), scene enter 0x2edbf8 (+0x40, from 0x198014), scene leave 0x2edc10 (+0x44), enable 0x2b476c
// (EnableTrafficAndPedestrians), update 0x2ee5e8 (+0x48: system list 0x5899f4 at priority 128, run by 0x169374 at the start of the
// gameplay step 0x2e5158) and spawn 0x2ee068. The caller supplies the state of the rest of the game: the spawn points of the pregen
// file's leaf, the view, each pedestrian's node and room, the room layer table and the blocked test. No SDL dependency.
#include "assets/assets.hpp"
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
constexpr std::int32_t kPedestriansMaxDefault = 5;
constexpr std::size_t kPedestrianLayerClasses = 7;
constexpr std::size_t kPedestrianRoomTypes = 17;
constexpr std::uint8_t kPedestrianRoomLoadedState = 2;

// "Pedestrian01" .. "Pedestrian20" (format "Pedestrian%02u" at 0x2ee558).
std::string pedestrianSlotName(std::size_t number);

// 0x2451d0(0x23c188(scene), "Pedestrians_Max", 5): the parameter of the scene's level root (owner 0, whose data is the scene's
// .blvl), found by 0x217080 in the level's tables +0x10 then +0x14 (a match without flag bit 0 or 1 is skipped). Types 5 and 7
// give the stored word, type 0 the float truncated (vcvt.s32.f32), any other type and a missing parameter 5.
// STAND-IN PED-S5: a slot parameter (flag bit 1) gives its table value; the root's slot values (+0x2c / +0x30) are not read.
std::int32_t pedestriansMaxOf(const assets::Level& level);

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

// A scene node (entity +0x3c) as the manager's tests read it. `parent` and `PedestrianRoom::node` are identities (0 = null) that
// the removal compares.
struct PedestrianNode {
    bool present = true;
    std::uint32_t word = 0;                   // node +0: type in bits 0..4, bit 9 distance fade, bit 18
    std::uint32_t parent = 0;                 // node +4
    std::uint32_t parentWord = 0;             // word +0 of the parent node
    std::uint8_t fadeFlags = 0;               // node +0xde: bit 4 distance fade on, bit 5 DistanceFadeMid
    float radius = 0;                         // node +0x9c
};

// 0x28781c: -1 without bit 9 of the word or bit 4 of +0xde, 1 with bit 5 of +0xde, else the radius against the words 0x52a1d0 ..
// 0x52a1d8 (table +0x3a0 .. +0x3a8 of 0x529e30): below the first 0, below the second 1, below the third 2, else -1.
int pedestrianFadeTier(const PedestrianNode& node, const DistanceFadeTable& table);

// 0x23de70: true without a node, when the parent has bits 17 and 18 set, or when the node has bit 18 set.
bool pedestrianNodeHidden(const PedestrianNode& node);

// A room object (entity +0x38, spawn record +0x24) as the room test reads it.
struct PedestrianRoom {
    std::uint8_t state = 0;                   // byte +8 of the record at room +4
    bool resource = false;                    // word +0x14 of that record is set
    std::uint8_t type = 0;                    // byte +0x3a
    std::uint32_t node = 0;                   // the node at room +0x7c
    std::uint32_t attachNode = 0;             // the node at room +0x14, the parent of a pedestrian spawned in the room
};

// The hwords of the layer table *0x376ad4 (+4, 7 classes x 17 room types), set by 0x2405a0; supplied by the caller.
using PedestrianLayerTable = std::array<std::uint16_t, kPedestrianLayerClasses * kPedestrianRoomTypes>;

// The room test of the spawn and the removal: a room that is set, 0x2284bc (state 2 with a resource) and 0x1ffbd8(class, type):
// 0 for class 7 and up, else bit 1 of the hword [class x 17 + min(type, 16)].
bool pedestrianRoomActive(const std::optional<PedestrianRoom>& room, std::uint8_t layerClass, const PedestrianLayerTable& layers);

// One 40-byte record of `Pregen/Pedestrians/<level>_Pedestrians.bin` after 0x2ee868 resolved it: position +0, direction
// +0xc, path object +0x18 (0 = not found), path parameter +0x20, room +0x24 (unset = no room).
struct PedestrianSpawnPoint {
    PedestrianVec3 position{};
    PedestrianVec3 direction{};
    std::uint32_t path = 0;
    float pathParameter = 0;
    std::optional<PedestrianRoom> room;
};

// The fields of a pedestrian entity and its component (+0x7c) that the manager reads and writes. The game keeps `layerClass`
// (entity byte +0x18), `node` and `room` (entity +0x38) up to date.
struct Pedestrian {
    std::uint8_t layerClass = 0;
    PedestrianNode node;
    std::optional<PedestrianRoom> room;
    bool animationPlayer = false;             // entity +0x44 is set
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
    PedestrianLayerTable layers{};
    // *0x37674c, the type of a mesh node: a spawned pedestrian whose node word has it in bits 0..4 starts the fade-in.
    std::uint32_t meshNodeType = 0;
    // 0x26cd38(pedestrian, point, zero vector, 0.0): 0x1ffcd4 against each entity of the list 0x5863b4 + 0x305c (count +0x3058)
    // whose vehicle record (+0x84 -> +0x7c) has no bit 1 at +0xcc. STAND-IN PED-S2: the test is the caller's.
    std::function<bool(std::size_t slot, const PedestrianVec3& position)> spawnBlocked;
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
    // 0x2ee4c8, run for each scene the loader loads: the slots are the scene's objects "Pedestrian01" .. "Pedestrian20" that exist
    // (0x23b350), in that order; each is disabled. The limit +0xa0 is `pedestriansMaxOf` of the scene's level, stored as a word.
    // Returns the numbers of the slots found.
    std::vector<std::size_t> loadLevel(const std::function<bool(const std::string& name)>& exists, const assets::Level& level);
    // 0x2edc94: bit 0 of +0xa4 is the result of the pregen file load 0x2ee868.
    void setSpawnDataLoaded(bool loaded);
    // 0x2edbf8 on every scene enter: 0x2edc94 (which sets bit 0 only when the level has a pedestriansystem, `loaded` empty
    // otherwise), then the countdown +0xa8 = 0.
    void enterScene(std::optional<bool> loaded);
    // 0x2edc10: the spawn data is freed and bit 0 cleared.
    void leaveScene();
    // 0x2b476c: bit 1 of +0xa4.
    void setEnabled(bool enabled);

    PedestrianUpdate update(const PedestrianWorld& world, GameRandom& random);

    // Byte 0x375d5c: the update clears it first (0x2ee5f8); the walk takes it (PedestrianWalkWorld::groundProbeTaken points here).
    bool* walkToken() { return &walkTokenTaken_; }
    // What the avoidance 0x2edf08 (PedestrianWalk::avoidance) compares with after the player: `placement` row 3 of every other live
    // slot, in slot order (PedestrianWalkWorld::otherPedestrians).
    std::vector<PedestrianVec3> otherLivePositions(std::size_t slot) const;

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
    bool walkTokenTaken_ = false;
};

// 0x237a0c on a matrix whose row 2 is `facing` and row 3 `position`: row 0 from row 2 (|y| <= 0.98: (z, 0, -x), else
// (0, -z, y)), normalized by 0x257ea0, row 1 = row 2 x row 0 (0x23b960).
std::array<PedestrianVec3, 4> pedestrianPlacement(const PedestrianVec3& position, const PedestrianVec3& facing);

}
