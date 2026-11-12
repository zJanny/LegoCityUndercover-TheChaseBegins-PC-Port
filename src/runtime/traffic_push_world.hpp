#pragma once
// The world of a traffic car's push (docs/re/traffic-car-push.md): the push vector and shape test 0x1f49d8 behind TrafficPushWorld::pushVector,
// the squash 0x233594, and what the character states do with the events 17 and 18 that message 45 (0x283d0c) sends: only the listeners of the
// states 0x99 (super knockback, 0x3305fc) and 0x199 (throw finisher, 0x334888 -> 0x1e3040) take them. No SDL dependency.
#include "runtime/ai_driver_input.hpp"
#include "runtime/character_hit.hpp"
#include "runtime/traffic_car_push.hpp"
#include "runtime/traffic_driver_calls.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace chase::runtime {

constexpr std::uint16_t kEntityFlagOrientedPush = 0x900;   // hword +0x10 of the pushed entity: 0x1f49d8 takes the oriented test 0x27f174
constexpr std::uint16_t kEntityFlagPushParts = 0x800;      // hword +0x10 of the pushed entity: the test runs per part of its node record
constexpr float kPushUprightLimit = 0.99f;                 // 0x1f4d2c: |row 1 y| of the entry matrix at or above it keeps the box as it is
constexpr float kPushShare = 0.5f;                         // 0x1f5054: each side of the push takes half of the move's length
constexpr std::uint32_t kPushSlidePushedFlags = 88;        // 0x23d434(pushed, vector, 88, 17, 0.0, 0.0)
constexpr std::uint32_t kPushSlideMoverFlags = 92;         // 0x23d434(mover, back, 92, 17, *0x376910, 0.0)
constexpr std::uint32_t kPushSlideMode = 17;

struct TrafficPushBox {
    AiVec3 centre{};   // entity +0x60, node space
    AiVec3 half{};     // entity +0x6c
};

// What 0x1f49d8(mover, move, entry, vector, slide) asks of the world. Entities are identities; the move's fourth float (the turn) is 0 on every
// call of 0x2baef0 (the contact slide passes (move, 0.0), the second push the vector of the constant 0x56d3a0).
struct TrafficPushShapes {
    std::function<std::uint16_t(std::uint32_t entity)> flags;               // hword +0x10
    std::function<std::uint8_t(std::uint32_t entity)> kind;                 // byte +0x12
    std::function<AiMatrix(std::uint32_t entity)> nodeMatrix;               // 0x250810(entity +0x3c)
    std::function<TrafficPushBox(std::uint32_t entity)> box;                // +0x60 / +0x6c; a box entry's shape +0x18 points at its owner's
    std::function<AiMatrix(const TrafficContactEntry& entry)> entryMatrix;  // 0x21bb78: the owner's node matrix times entry +0x14
    std::function<bool(const TrafficContactEntry& entry)> meshEntry;        // entry +0x1c set (kind 1, a collision item)
    // STAND-IN TRAFFIC-S40: 0x287ac4(mover, move, entry, 93, 3, *0x376910, 0.0) (mesh entries), 0x27f174 (the oriented test of a pushed entity
    // with 0x900 or a mover with 0x2000, per part for 0x800) and the slide 0x23d434 (0x208d84 with the flags 88 / 92 and 17) are not read.
    std::function<bool(std::uint32_t mover, const AiVec3& move, const TrafficContactEntry& entry)> meshPush;
    std::function<bool(std::uint32_t mover, const AiVec3& move, const TrafficContactEntry& entry, const AiMatrix& matrix, const TrafficPushBox& box,
                       const AiVec3& moverCentre, const AiVec3& movedCentre, bool slide)> orientedPush;
    std::function<void(std::uint32_t entity, AiVec3& move, std::uint32_t flags, float floorNormalMin)> slideMove;
    std::function<void(std::uint32_t entity, const AiVec3& delta)> moveNode;   // node translation += delta, then 0x24c118
    std::function<void(std::uint32_t owner)> lastHit;                          // *0x377134 = owner
    // STAND-IN TRAFFIC-S40: the kinds of the list 0x58d9b0 (count *0x377138, 0 in the image's data); its writers are not read.
    std::vector<std::uint8_t> excludedKinds;
    float floorNormalMin = 0x1.37af86p-1f;   // *0x376910 at the level start (kFloorNormalMin of collision.hpp)
};

// 0x1f49d8. `vector` is cleared (0x25b390) and, for a box entry with slide, gets the push of the pushed entity; the mover then moves back by
// its share. Returns whether `entry` is hit.
bool trafficPushVector(const TrafficPushShapes& shapes, std::uint32_t mover, const AiVec3& move, const TrafficContactEntry& entry, AiVec3* vector,
                       bool slide);

// trafficPushVector as the hook TrafficPushWorld::pushVector. The shapes must outlive the hook.
std::function<bool(std::uint32_t, const AiVec3&, const TrafficContactEntry&, AiVec3*, bool)> makeTrafficPushVector(const TrafficPushShapes& shapes);

constexpr std::int32_t kSquashDamage = 32000;   // 0x2335b8
constexpr std::uint8_t kSquashByte15 = 255;     // record byte +0x15; CharacterHitMessage does not carry it (STAND-IN TRAFFIC-S41)

// 0x233594(target, sender, weapon): message 0 to `target` (in 0x2baef0 the pushing character) with {no move, sender, damage 32000, force 0,
// kind 0, weapon}.
CharacterHitMessage trafficSquashHit(const CharacterHitSender& sender, std::uint8_t weapon);

constexpr std::uint32_t kCharacterEventCollider = 17;   // "..._ColliderEvent": 0x283d0c to the pushed character, args {pusher, vector}
constexpr std::uint32_t kCharacterEventCollidee = 18;   // "..._CollideeEvent": 0x283d0c to a pushing character, arg the pushed one
constexpr std::uint16_t kCharacterStateSuperKnockback = 0x99;   // state object 0x58d920, listeners 0x58d8f0..0x58d914 (0x268e24)
constexpr std::uint16_t kCharacterStateThrowFinisher = 0x199;   // state object 0x5158f8, listeners 0x5158b8..0x5158e8 (0x28b96c)
// STAND-IN TRAFFIC-S41: the damage word *0x377130 that 0x3305fc sends is 1 in the image's data; its writers are not read.
constexpr std::int32_t kSuperKnockbackDamageStandIn = 1;
constexpr std::uint8_t kThrowFinisherHitKind = 15;      // listener +0xc of 0x5158d8 / 0x5158e8, record byte +0x14
constexpr std::int32_t kThrowFinisherDamage = 1;        // 0x1e306c
constexpr float kHeadingUnits = 10430.378f;             // 0x1e31d8
constexpr std::uint8_t kThrowFinisherKindA = 85;        // 0x1e3188: the kinds that take the record as message 0
constexpr std::uint8_t kThrowFinisherKindB = 119;
constexpr std::uint8_t kThrowFinisherKindC = 214;

// What the throw finisher's 0x1e3040(listener, self, other) asks of the other entity.
struct ThrowFinisherOther {
    std::uint32_t entity = 0;
    std::uint8_t kind = 0;                 // +0x12
    bool character = false;                // 0x245b00
    // STAND-IN TRAFFIC-S41: 0x22716c, 0x23a70c(ctrl, 52), 0x21a2d4(other, self, 8) and 0x22103c are not read; their results are inputs.
    bool takesCombat = false;              // 0x22716c(other)
    std::uint32_t target140 = 0;           // ctrl +0x140
    bool ability52 = false;                // 0x23a70c(ctrl, 52)
    bool reachable = false;                // 0x21a2d4(other, self, 8)
    bool controlled = false;               // 0x22103c(other) and ctrl (+0x7c) set
    bool ctrlBitDE0 = false;               // ctrl +0xde bit 0
    std::uint8_t ctrlCC = 0;               // ctrl +0xcc
    AiVec3 position{};                     // node translation
};

struct CollisionEventOutcome {
    std::optional<std::uint32_t> messageTarget;     // message 0 (0x244fc8) goes to it with `hit`
    std::optional<CharacterHitMessage> hit;
    bool destroysSelf = false;                      // 0x23cd80(self, 0)
    std::optional<std::uint32_t> combatTarget;      // 0x22e884(other, ctrl, record) with `hit` after 0x23deb4(other) and record byte +0xe = 1
    std::optional<std::uint16_t> combatHeading;     // ctrl +8 and +0xa of the combat target: it faces the finisher
};

// 0x3305fc: event 17 with a sender (args +0) or event 18 with an entity hits it with {no sender, damage *0x377130}; every event destroys self.
CollisionEventOutcome superKnockbackEvent(std::uint32_t type, std::optional<std::uint32_t> other);

// 0x334888 for the events 17 and 18 -> 0x1e3040(listener, self, other).
CollisionEventOutcome throwFinisherEvent(const CharacterHitSender& selfSender, std::uint32_t self, const AiVec3& selfPosition,
                                         const std::optional<ThrowFinisherOther>& other);

// The listener of `state` for the event 17 or 18 (0x21e1d4 -> 0x1eb8c4 on the state on top of ctrl +0x50); nothing for the other states.
CollisionEventOutcome characterCollisionEvent(std::uint16_t state, std::uint32_t type, const CharacterHitSender& selfSender, std::uint32_t self,
                                              const AiVec3& selfPosition, const std::optional<ThrowFinisherOther>& other);

struct CarPushCharacter {
    std::uint32_t entity = 0;
    std::uint16_t state = 0;     // the state on top of ctrl +0x50
    AiVec3 position{};
    bool player = false;         // *0x3772c0
    CharacterHitSender asSender; // the character as the sender of its listener's record
};

struct CarPushDelivery {
    CollisionEventOutcome event17;                  // the character's listener, first
    std::optional<CollisionEventOutcome> event18;   // the sending character's listener
    CharacterPushResult push;                       // receiveCarPush: the hit of a moving vehicle in front
};

// Message 45 to a character in the order of 0x283d0c: event 17 to its state with the sender, event 18 to a character sender's state, the hit.
// `senderState`, `senderAsSender` and `senderPosition` are those of a character sender, `asOther` the character as the sender's listener sees
// it, `senderAsOther` the sender as the character's listener sees it.
CarPushDelivery deliverCarPush(const CarPushCharacter& character, const std::optional<CharacterPushSender>& sender,
                               const std::optional<ThrowFinisherOther>& senderAsOther, std::uint16_t senderState, const CharacterHitSender& senderAsSender,
                               const AiVec3& senderPosition, const std::optional<ThrowFinisherOther>& asOther);

// The character's own part of a delivery: its destroy (entity +0xc bit 4, 0x23cd80) by event 17, then the hit of receiveCarPush through
// receiveHit (0x1b8570), which ignores a destroyed character.
std::optional<CharacterHitResult> applyCarPushToCharacter(CharacterHitTarget& target, const CarPushDelivery& delivery);

}
