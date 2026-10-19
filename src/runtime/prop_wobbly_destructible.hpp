#pragma once
// The class leprops/leprop_wobblydestructible (object type 119, message handler 0x2a3390, create 0x298120) for any sender of message 0: the punch of the
// player, the car (runtime/vehicle_prop_smash.hpp) or a character of the player's type. Message 0 subtracts the damage word from the health halfword
// (record +0x10, WobbleHealth); a hit that leaves health raises ObjectHit and wobbles the prop (bricks 0x2bc370, spring 0x16b958, sound ATTR_SFX_HIT), a
// hit that takes it to 0 or below destroys the entity (0x23cd80, message 1: ObjectDead event, the effects of runtime/prop_destroy.hpp, the sound
// ATTR_SFX_DESTROYED and TriggerObject through 0x244f40). Evidence: docs/re/props-wobbly-bouncer.md. No SDL dependency.
#include "assets/assets.hpp"
#include "runtime/debris.hpp"
#include "runtime/prop_destroy.hpp"
#include "runtime/stud_callers.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr const char* kWobblyClass = "leprops/leprop_wobblydestructible";
constexpr std::uint8_t kMessageHit = 0;
constexpr std::uint8_t kCharacterTypeByte = 27;  // entity byte +0x12 of the player's type; another sender of this type destroys the prop outright

// Message 0 as the handler reads it: msg+4 (the sender; null or *0x3772b8 skip the wobbling test and the type test) and msg+0xc (the damage).
struct WobbleHitMessage {
    bool senderPresent = true;
    bool senderIsPlayer = true;
    std::uint8_t senderType = kCharacterTypeByte;
    std::int32_t damage = 0;
};

struct WobbleHitResult {
    bool dropped = false;      // not handled: a sender other than the player while the prop wobbles (0x1c54ac), or damage 0
    bool hitEvent = false;     // 0x23cee8 with *0x3775e0 (ObjectHit) is raised
    bool wobble = false;       // health above 0 after the hit: spring, bricks and sound of the hit
    bool destroyed = false;    // health at or below 0 (or a sender of type 27): 0x23cd80
    std::int16_t health = 0;
};

// The body of message 0 of 0x2a3390 up to the choice between wobble and destroy. `alreadyWobbling` is the answer of 0x1c54ac.
WobbleHitResult wobblyHit(std::int16_t health, const WobbleHitMessage& message, bool alreadyWobbling);

// The parameters of the hit's brick burst 0x2bc370.
struct WobbleBurstParams {
    unsigned count = 2;                                    // WobbleDebrisAmount when above 0 (low byte of its integer), else 2
    std::uint32_t colour = kDebrisDefaultColour;           // DebrisColour
    std::uint32_t secondColour = kDebrisNoSecondColour;    // DebrisColour2
    float ratio = 0;                                       // DebrisRatio
};

struct WobblyProp {
    std::string instance;                 // lower case
    std::int16_t health = 0;
    bool destroyed = false;
    PropDestroyEffects destroyEffects;
    std::string triggerObject;            // TriggerObject, lower case, empty when unset
    assets::Matrix node{};                // node world matrix at the level start
    std::array<float, 3> boundsCentre{};  // node bounds (node+0xa0), the union of the model's mesh boxes; also the entity box of a prop without MeshCollision
    std::array<float, 3> boundsHalf{};
    WobbleBurstParams burst;
    unsigned studValue = 2;               // WobbleStudValue: the studs of a wobbling hit (0x2bc370 -> 0x23a6b0)
    StudSpawnSource studSpawn;            // SpawnCone, SpawnLocal, SpawnDir, SpawnLoc
    std::uint16_t soundHit = 0;           // ATTR_SFX_HIT
    std::uint16_t soundDestroyed = 0;     // ATTR_SFX_DESTROYED
};

struct WobblyReaction {
    std::string instance;
    WobbleHitResult hit;
    std::optional<DebrisBurst> wobbleBricks;   // the burst of 0x2bc370 at the node bounds centre
    unsigned wobbleStudValue = 0;              // studs of the hit
    std::optional<StudDropRequest> wobbleStuds;  // 0x2bc370 after the bricks: none for a value of 0
    std::uint16_t sound = 0;                   // ATTR_SFX_HIT for a wobble, ATTR_SFX_DESTROYED for a destroy
    std::array<float, 3> soundPosition{};      // the node position
    std::string triggerObject;                 // on a destroy: the object 0x244f40 enables, messages and raises ObjectTriggered for
    WobblyProp prop;                           // the state after the hit, for the destroy effects
};

class WobblyProps {
public:
    void load(assets::FuseArchive& archive, const assets::Level& level, const assets::ClassLookup& classOf);
    // Delivers message 0 to the instance (lower case). Nothing when it is not a wobbly prop or was destroyed already.
    std::optional<WobblyReaction> deliver(const std::string& lowerInstance, const WobbleHitMessage& message);
    const WobblyProp* find(const std::string& lowerInstance) const;
    std::size_t size() const { return props_.size(); }

private:
    std::vector<WobblyProp> props_;
};

}
