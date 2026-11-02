#pragma once
// The AI spawner class, kind 14 (`props/prop_aispawner`; docs/re/scripts-missions-1-4-spawners.md): create 0x1a58c4, post-link 0x19862c, handler 0x1b4fb0
// and the update 0x1a5d2c, which runs 0x2424d0 with the pre-update 0x288f18 and the update 0x288fbc, then the state change 0x268978. The record at
// entity +0x7c holds the state (+2) and the requested state (+4), the character slots (+0x1c, count +0x24) and the counters. No SDL dependency.
#include "assets/assets.hpp"
#include "runtime/ai_start_state.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr std::uint8_t kAiSpawnerKind = 14;
constexpr unsigned kAiSpawnerUpdatesPerSecond = 30;    // 0x23ca4c
constexpr std::size_t kAiSpawnerTargetParams = 10;     // target1 .. target10 (0x19862c)
constexpr std::uint8_t kAiSpawnerMessageReset = 8;
constexpr std::uint8_t kAiSpawnerMessageCountHeld = 33;
constexpr std::uint8_t kAiSpawnerMessageOff = 0xfe;
constexpr std::uint8_t kAiSpawnerMessageOn = 0xff;

enum class AiSpawnerState : std::uint16_t { Idle = 0, Active = 1, Spawn = 2, PlayerOutside = 3 };
const char* aiSpawnerStateName(AiSpawnerState state);

// The parameters create 0x1a58c4 and post-link 0x19862c read (defaults are the fallbacks the code passes).
struct AiSpawnerParams {
    bool startActive = true;          // StartActive (default 1): +4 and +2
    bool startSpawned = true;         // StartSpawned (default 1, post-link)
    std::int16_t maxSpawn = 0;        // MaxSpawn +0x3c (default 0); below 0 the post-link sets the slot count
    std::uint16_t maxOnScreen = 0;    // MaxOnScreen +0x42 (default 0)
    std::uint8_t respawnFrames = 0;   // respawntime x 30, at most 255 (+0x2a)
    std::uint8_t staggerFrames = 0;   // staggertime x 30, low byte (+0x29)
    bool waitForDead = false;         // WaitForDead, +0x60 bit 0 (default 0)
    bool spawnFromSpawner = true;     // SpawnFromSpawner, bit 1 (default 1): the character spawns at the spawner's node
    bool spawnOnScreen = true;        // SpawnOnScreen, bit 2 (default 1): 0 spawns only out of view (0x265190)
    std::uint16_t startState = 0;     // StartState +0x26: entry of the state table 0x58db48 the spawned character gets
    std::uint16_t sfx = 0;            // ATTR_SFX +0x44
    std::int32_t sfxPlayFrames = 0;   // SFXPlayTime x 30 (+0x38)
    std::string startEffect;          // StartEffect (+0x18), lower case, empty when unset
    std::string revealObject;         // RevealObject (+0x54)
    std::string playerInsideBound;    // PlayerInsideBound (+0x58): a bound of the class file
    std::string mesh;                 // MeshName; without one the spawner has no "close", "spawn" and "waiting" clips
};

struct AiSpawnerSlot {
    std::string character;            // lower case, slot word +0
    std::uint8_t countdown = 0;       // slot byte +4: updates until the slot may spawn again
    assets::Matrix placed{};          // the character's node matrix at the post-link (+0x20, kept only without SpawnFromSpawner)
};

struct AiSpawner {
    std::string instance;             // lower case
    assets::Matrix matrix{};          // the spawner's node matrix
    AiSpawnerParams params;
    AiStartState start;               // the StartState entry's record (+0x5c)
    std::vector<AiSpawnerSlot> slots;
    AiSpawnerState state = AiSpawnerState::Idle;      // +2
    AiSpawnerState requested = AiSpawnerState::Idle;  // +4
    std::int16_t maxSpawn = 0;        // +0x3c
    std::uint16_t spawned = 0;        // +0x3e
    std::int16_t onScreen = 0;        // +0x40
    std::uint16_t heldMask = 0;       // +0x2e: bit per slot whose character is held
    std::uint8_t stagger = 0;         // +0x28
    std::uint8_t spawnSlot = 0;       // +0x2b
    std::uint8_t lastHeld = 0;        // +0x30
    bool revealPending = false;       // +0x54 not cleared yet
};

// What the class does to other entities. The port's state of a character: held is entity word +0xc bit 4 (the destroyed bit of
// assets::PlacedObjectState), disabled is halfword +0x10 bit 0.
class AiSpawnerWorld {
public:
    virtual ~AiSpawnerWorld() = default;
    virtual bool held(const std::string& character) const = 0;
    virtual bool disabled(const std::string& character) const = 0;
    // 0x1b5250: bit 4 set, message 53 (0x236724 hides the character's ability items), 0x2428c0 (disable).
    virtual void hold(const std::string& character) = 0;
    // 0x2428c0 alone (the post-link on the reveal object).
    virtual void disable(const std::string& object) = 0;
    // Message 8: bit 4 cleared and 0x242cf0, without message 52.
    virtual void restore(const std::string& character) = 0;
    // The tests of 0x288fbc on a held character whose slot may spawn: its controller's +0x16c is null or carries the character's node, and it is
    // not waiting for DEAD (requested state +0x6e 11) while enabled.
    virtual bool controllerAllowsSpawn(const std::string& character) const = 0;
    // 0x265190: the spawn point is in the camera's view (only asked with SpawnOnScreen 0).
    virtual bool inView(const assets::Matrix& at) const = 0;
    // 0x268978 state 2: 0x24c118 puts the character's node at `matrix`, then message 52 (0x21e810: 0x218b88, 0x242cf0 enable, bit 4 cleared,
    // message 2). Returns whether the character came out (bit 4 clear and the controller's requested state +0x6e is 1).
    virtual bool release(const std::string& character, const assets::Matrix& matrix) = 0;
    // The spawn word +0xc of the StartState entry (ai_start_state.hpp), called with the character of slot +0x2b once it came out.
    virtual void startState(const AiSpawner& spawner, std::size_t slot) = 0;
    // 0x23cee8 of *0x3775ec ("AISpawned") with the object as owner and instigator.
    virtual void aiSpawned(const std::string& object) = 0;
    // 0x23211c(spawner, record +0xc, mode): the spawner's TriggerObject reference. referenceIdle is its kind bits (+0xc & 31) == 0.
    virtual void activateReference(const std::string& spawner) = 0;
    virtual void deactivateReference(const std::string& spawner) = 0;
    virtual bool referenceIdle(const std::string& spawner) const = 0;
    // 0x273290: the reveal object is enabled when it is disabled and moved to the last held character; message 51.
    virtual void reveal(const std::string& object, const std::string& lastHeldCharacter) = 0;
    // 0x22f650 of the player's node translation against the spawner's PlayerInsideBound.
    virtual bool playerInside(const AiSpawner& spawner) const = 0;
};

enum class AiSpawnerEventKind : std::uint8_t { State, Spawned, Fired, Revealed };
struct AiSpawnerEvent {
    AiSpawnerEventKind kind = AiSpawnerEventKind::State;
    std::string spawner;
    AiSpawnerState state = AiSpawnerState::Idle;  // State: the state the change entered
    std::string character;                         // Spawned
    std::uint8_t slot = 0;
    std::uint16_t spawned = 0, maxSpawn = 0;       // Spawned: the counters after the spawn (the debug line 0x359878 prints +0x3e before it)
};

// Reads one kind-14 object (create and the parameter part of the post-link). `names` maps a reference hash to the lower-case instance.
AiSpawner aiSpawnerOf(const assets::LevelObject& object, const assets::Level& cls, const std::map<std::uint32_t, std::string>& names,
                      const assets::Matrix& matrix);

class AiSpawnerSet {
public:
    // Every created kind-14 object of the level (create flag bit 0 clear), in object order. `characterMatrix` gives a slot character's node
    // matrix at the post-link; a target that names no object of the level is no slot.
    void load(const assets::Level& level, const assets::ClassLookup& classOf,
              const std::function<std::optional<assets::Matrix>(const std::string&)>& characterMatrix);
    void add(AiSpawner spawner) { spawners_.push_back(std::move(spawner)); }
    // The slot loop of the post-link 0x19862c: holds the characters (StartSpawned 0, or beyond MaxOnScreen) and counts the others.
    void postLink(AiSpawnerWorld& world);
    // Handler 0x1b4fb0: 8 reset, 33 count the held slots into MaxSpawn, 254 off, 255 on when the requested state is 0 and off otherwise (0x1b4f28
    // with 0). Messages 28 (the entity id of the slot +0x30) and 252 (a callback the message carries) answer nothing here. False when the instance is
    // no spawner.
    bool message(const std::string& lowerInstance, std::uint8_t message, AiSpawnerWorld& world);
    // One update of every spawner (0x1a5d2c).
    void update(AiSpawnerWorld& world);
    std::vector<AiSpawnerEvent> takeEvents();
    const std::vector<AiSpawner>& spawners() const { return spawners_; }
    const AiSpawner* find(const std::string& lowerInstance) const;
    std::vector<AiSpawner>& spawners() { return spawners_; }

private:
    void hold(AiSpawner& spawner, std::size_t slot, AiSpawnerWorld& world);
    void reset(AiSpawner& spawner, AiSpawnerWorld& world);
    void countHeld(AiSpawner& spawner, AiSpawnerWorld& world);
    void preUpdate(AiSpawner& spawner, AiSpawnerWorld& world);
    void stateUpdate(AiSpawner& spawner, AiSpawnerWorld& world);
    void heldCheck(AiSpawner& spawner, AiSpawnerWorld& world);
    void stateChange(AiSpawner& spawner, AiSpawnerWorld& world);
    std::vector<AiSpawner> spawners_;
    std::vector<AiSpawnerEvent> events_;
};

}
