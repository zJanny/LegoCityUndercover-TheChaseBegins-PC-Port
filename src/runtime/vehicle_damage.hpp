#pragma once
// Contact damage of a vehicle (docs/re/vehicles-physics-damage.md): the contact message 0x80000002 of the vehicle handler 0x1797c4 (0x179ca4) turns
// the closing speed of a contact into a brick burst and a damage message, and the damage message 0 takes the part of the current heart and the
// hearts of the vehicle record (+0x30c, +0x30d). Only the vehicle the player drives takes damage (0x24255c of the player is the entity). No SDL
// dependency.
#include "runtime/vehicle.hpp"
#include "runtime/vehicle_box_contact.hpp"
#include "runtime/vehicle_collision.hpp"

#include <cstdint>
#include <optional>

namespace chase::runtime {

// The static initialiser 0x34498c fills the floats at 0x3745a4 with 0.5, 0.15, 30, 6, 2, 0.8, 132, 3.5, 1.0 (the mass scale 2 at 0x3745b4 and 6.0 at
// 0x3745b0 are among them). 0x3745a4 is the time after a lost heart in which no further damage counts, 0x3745c4 the time between two brick bursts.
constexpr float kVehicleHitCooldown = 0.5f;
constexpr float kVehicleDebrisCooldown = 1.0f;
// 0x179ca4: a contact above this closing speed spawns bricks (0x229fb8), above 20 it damages the vehicle; the damage grows up to the speed 60.
constexpr float kVehicleBrickBurstSpeed = 10.0f;
constexpr float kVehicleBrickSpeedCap = 40.0f;
constexpr float kVehicleDamageSpeed = 20.0f;
constexpr float kVehicleDamageSpeedCap = 60.0f;
constexpr float kVehicleDamageScale = 0.025f;
constexpr float kVehicleDamageFactor = 5.0f;
constexpr std::uint8_t kVehicleFullHealthPoints = 100;
constexpr std::uint8_t kVehicleDamageKindContact = 11;
constexpr std::uint8_t kVehicleDamageTypeScripted = 6;
constexpr std::uint8_t kVehicleFlagRevivable = 0x80;
constexpr int kVehicleSmokeLastHeart = 0x11f;
constexpr int kVehicleSmokeDamaged = 0x11e;

// 0x19b5c4 / 0x19b648 for a point of a Bullet manifold (0x271764). `selfIsFirst` is the message side byte 0 (the vehicle is the manifold's body A and
// the normal is negated). The arms are the point's local coordinates as the original uses them (+0x00 for A, +0x10 for B). `other` is the other body of
// a vehicle pair, null for a static box.
float manifoldImpactSpeed(const VehicleRigidBody& self, bool selfIsFirst, const ManifoldPoint& point, const VehicleRigidBody* other);
// The same for a contact of the floor pass (0x181e68 from 0x1c54f8, no other body): the arm is the contact point minus the body position.
float floorImpactSpeed(const VehicleRigidBody& self, const FloorContact& contact);

struct DamageMessage {
    std::uint32_t hearts = 0;  // +0xc
    float fraction = 0;        // +0x10: the part of a heart, 0 to 1
    std::uint8_t kind = 0;     // +0x14
    std::uint8_t blocked = 0;  // +0x15: a non-zero byte refuses the damage unless the type is 6
    std::uint8_t type = 0;     // +0x17
};

struct ContactOther {
    bool entity = false;        // the other body belongs to an entity (the static boxes do not)
    bool destroyed = false;     // its entity word +0xc bit 0x10
    bool hasBody = false;
    bool noDamageBody = false;  // its body flag +0xc4 bit 0x40000
};

struct ContactReply {
    bool brickBurst = false;
    int brickCount = 0;  // the byte of the particle descriptor, 1 to 3
    std::optional<DamageMessage> damage;
    bool impactSound = false;  // speed above 3: the vehicle's sound at record +0x28 (0x229ef0, 0x229e80)
};

// The tail of 0x179ca4 for the player's vehicle; `speed` is manifoldImpactSpeed or floorImpactSpeed. Sets the brick cool-down.
ContactReply vehicleContactReply(VehicleRecord& record, float speed, const ContactOther& other);

struct DamageOutcome {
    bool applied = false;
    bool heartLost = false;
    bool destroyed = false;
    int smokeEffect = 0;
};

// Message 0 of 0x1797c4. `playerVehicle` is whether the player drives the vehicle. STAND-IN VEH-S90: the check 0x25207c (a timer of the game state
// that refuses damage) is taken as false.
DamageOutcome vehicleDamage(VehicleRecord& record, const DamageMessage& message, bool playerVehicle);

// End of the vehicle update 0x1571dc (0x157430): both cool-downs count down by the update's time and stop at 0.
void tickVehicleDamage(VehicleRecord& record, float dt);

}
