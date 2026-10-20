#pragma once
// Message 0 (a hit) of the props of ObjectType 67 (props/prop_prop, leprops/leprop_prop and 55 other classes): the kind's handler 0x15e408 and the
// hit machine 0x2398d8 it passes the message to, with the record the create 0x15a4b4 -> 0x16a268 fills (docs/re/vehicles-prop-smash.md, entry
// "The hit machine of ObjectType 67"). No SDL dependency.
#include "assets/assets.hpp"

#include <cstdint>

namespace chase::runtime {

// The record fields of 0x16a268 that message 0 reads.
struct HitMachineProp {
    std::uint8_t mode = 0;               // +0xcc: the class parameter `Destructable` (0x15a4b4)
    std::int8_t health = 0;              // +0xd4: `Health` (default 1) for the modes 1, 3, 4, 7 and 11
    std::int8_t projectileType = -1;     // +0xdb: `ProjectileType`
    bool ignorePlayerDamage = false;     // `IgnorePlayerDamage`
    std::int32_t abilityDestructable = 0;  // `AbilityDestructable`
};

HitMachineProp hitMachineProp(const assets::LevelObject& object, const assets::Level& cls);

// The message words the two handlers read: msg+0 word, msg+0xc damage, msg+0x14 / +0x15 / +0x16 bytes, and of the sender (msg+4) its kind byte
// +0x12, whether it is a character (0x245b00) and whether it is the player's entity (*0x3772c0).
struct HitMessage {
    std::uint32_t word0 = 0;
    std::int32_t damage = 0;
    std::uint8_t hitKind = 0;
    std::uint8_t projectile = 0;
    std::uint8_t sequence = 0;
    bool hasSender = false;
    std::uint8_t senderKind = 0;
    bool senderIsCharacter = false;
    bool senderIsPlayer = false;
};

// The vehicle's message 0 of the smash pass 0x275e74: damage 100, byte +0x14 = 11, the vehicle (kind 183) as sender. 0x245b00 is false for it:
// kind 183 is neither 27 nor 149 nor in the character-kind table, and its entity word +0xc has no bit 14 (oracle, veh-smash-prop).
HitMessage vehicleSmashMessage();

enum class HitOutcome {
    Ignored,    // no change (also the modes 0, 2, 6, 8..10 and above 11 and the gates of 0x15e408)
    Damaged,    // the health went down and stays above 0
    Destroyed,  // the health reached 0: 0x23cd80(entity, 0)
    NotPorted   // the modes 3 (lights and area damage) and 11 (the hit sequence 0x239c40 / 0x2b7894), 11 with byte +0x16 = 255 (0x15e780),
                // and a character sender of a prop with `AbilityDestructable` (the ability test 0x23a70c -> 0x219a8c is not read)
};

// 0x15e408 message 0 and 0x2398d8 case 0. STAND-IN VEH-S107: the culled test 0x23de70 (with `DamageWhenCulled`) counts as not culled and the
// record byte +0xde bit 0 as clear; the event 0x23cee8 of 0x15e5d4 is not raised. A prop without record (entity byte +0x13 = 2) is never
// built here: every object whose mode is not 0 has one, and mode 0 ignores the hit.
HitOutcome hitMachineMessage0(HitMachineProp& prop, const HitMessage& message);

}
