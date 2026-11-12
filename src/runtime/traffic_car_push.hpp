#pragma once
// A traffic car pushing characters and the cop whistle (docs/re/traffic-car-push.md): the push of one gathered character 0x2baef0 with its
// message 45 to the character, the character's handler of message 45 (0x18e4c8 case 45 -> 0x283d0c: the hit a moving vehicle gives), and the
// player state 0x1b2 COPWHISTLE (request in the player input 0x200354, enter 0x276338, update 0x29a2e0) that starts the traffic stop of
// traffic_driver_calls.hpp. No SDL dependency.
#include "runtime/ai_driver_input.hpp"
#include "runtime/character_hit.hpp"
#include "runtime/game_random.hpp"
#include "runtime/traffic_driver_calls.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace chase::runtime {

constexpr std::uint32_t kCharacterMessageCarPush = 45;   // 0x18e4c8 case 45
constexpr std::uint16_t kEntityFlagTakesPush = 0x200;    // hword +0x10: 0x2baef0 does nothing unless both entities of the pair have it
constexpr std::uint16_t kEntityFlagBlocker = 0x100;      // hword +0x10: an entity the pushed character stops at
constexpr std::uint16_t kEntityFlagReturnsPush = 0x2000; // hword +0x10 of the car: the push is answered to the car

struct TrafficPushCar {
    std::uint32_t entity = 0;     // identity, not 0
    std::uint16_t flags = 0;      // hword +0x10
};

// What 0x2baef0 asks of the world. `owner` is entry +0x10 of a gathered entry.
struct TrafficPushWorld {
    std::function<std::uint16_t(std::uint32_t entity)> flags;                       // hword +0x10
    std::function<bool(std::uint32_t entity)> bit3;                                 // word +0xc bit 3 of the pushed character
    std::function<bool(std::uint32_t a, std::uint32_t b)> ignorePair;               // 0x1fc508(a, b, 0): the pair does not collide
    // STAND-IN TRAFFIC-45: 0x1f49d8(mover, move, entry, vector, slide) is not ported (the shape test and slide that decide whether and how far
    // `entry` is pushed). `vector` is null for a test only; a hit writes it. Without this hook nothing is pushed.
    std::function<bool(std::uint32_t mover, const AiVec3& move, const TrafficContactEntry& entry, AiVec3* vector, bool slide)> pushVector;
    std::function<void(std::uint32_t character, std::uint32_t pusher, const AiVec3* vector)> message45;  // 0x244fc8(character, 45, {pusher, vector})
    std::function<bool(std::uint32_t car)> isCharacter;                             // 0x245b00(car)
    std::function<void(std::uint32_t car, std::uint32_t character)> squash;         // 0x233594(car, character, 0): not ported (TRAFFIC-46)
    std::function<void(std::uint32_t entity, const AiVec3& delta)> moveNode;        // 0x23d39c(node +0x30, delta) then 0x24c118(node, matrix)
};

using TrafficPushHook = std::function<bool(AiVec3& move, std::size_t entry, std::vector<TrafficContactEntry>& entries, bool slide)>;

// 0x2baef0(car, move, entry, list, count, slide) as the hook `push` of slideTrafficCarContacts. The car and the world must outlive the hook.
// The result is 1 only after message 45 without slide and after the answer to a returning or blocking car; every other path gives 0.
TrafficPushHook makeTrafficCarPush(const TrafficPushCar& car, const TrafficPushWorld& world);

constexpr std::uint8_t kEntityKindVehicleA = 179;   // 0x283d0c: the kinds whose push can be a hit
constexpr std::uint8_t kEntityKindVehicle = 183;
constexpr std::uint8_t kEntityKindVehicleB = 243;
constexpr float kCarPushFrontScale = 0.5f;          // 0x283ea8: the front point is the car's half length times this along row 2
constexpr float kCarPushMinForwardSpeed = 5.0f;     // 0x283eb0
constexpr float kCarPushFrontAngle = 70.0f;         // 0x283eac: the front limit is cosf of this
constexpr std::uint8_t kCarPushHitKind = 3;         // message 0 byte +0x14
constexpr std::uint8_t kCarPushHitWeapon = 1;       // message 0 byte +0x17

// The pushing entity of message 45 (args +0) as 0x283d0c reads it.
struct CharacterPushSender {
    std::uint32_t entity = 0;     // identity, not 0
    std::uint8_t kind = 0;        // +0x12
    bool character = false;       // 0x245b00
    AiMatrix node{};              // 0x250810(entity +0x3c): row 2 the forward axis
    AiVec3 localPoint{};          // entity +0x60..+0x68
    float halfLength = 0.0f;      // entity +0x74
    float forwardSpeed = 0.0f;   // 0x229948(body +0x24, the velocity at [body +0x7c] +0x13c): signed, negative when reversing
};

struct CharacterPushResult {
    bool stateEvent17 = true;                          // 0x21e1d4(ctrl +0x50, character, 17, args): always, with or without a sender
    std::optional<std::uint32_t> senderEvent18;        // 0x21e1d4(sender ctrl +0x50, sender, 18, character) for a character sender
    std::optional<CharacterHitMessage> hit;            // message 0 to the character for a moving vehicle in front
    bool clearsPushBit = true;                         // ctrl +0x315 bit 2 cleared at the end
};

// 0x283d0c(character, ctrl, args). `characterPosition` is the character's node translation, `characterIsPlayer` the comparison with
// *0x3772c0 that sets the damage.
CharacterPushResult receiveCarPush(const std::optional<CharacterPushSender>& sender, const AiVec3& characterPosition, bool characterIsPlayer);

constexpr std::uint16_t kControllerStateCopWhistle = 0x1b2;   // table 0x357a08: update 0x29a2e0, enter 0x276338
constexpr unsigned kCopWhistleExtra = 19;                     // 0x2369a8(19): bit 19 of the extras mask *0x374724
constexpr std::uint16_t kCopWhistleRequestState = 1;          // ctrl +0x6c must be the idle state
constexpr std::uint8_t kCopWhistleCharacterClass = 3;         // byte +0x20 of the player's character row (*0x372e10, stride 0x28)
constexpr int kCopWhistleClipRow = 709;                       // 0x276338: tablesanimations row 0x2c5
constexpr float kCopWhistleBlend = 0.1f;
constexpr float kCopWhistleSpeed = 1.0f;
// STAND-IN TRAFFIC-S33: the game clock 0x241098 is the running process's time x 30 / 3000; one unit per update assumes a timer unit of 3000 per
// second (unread, SCRIPT-384). A trap measured +0.99 per stepped update, the use markers 2 per update (SCRIPT-S247, SCRIPT-S297).
constexpr std::uint32_t kGameClockUnitsPerUpdateStandIn = 1;

// What 0x200354 reads for the request of 0x1b2, in its order: the extras bit 19, the player's state, the D-pad Down press edge (slot 1,
// channel 13), the word ctrl +0x13c and the character row's byte +0x20. The player's branch only (the entity is *0x3772b8).
struct CopWhistleGate {
    bool extraActive = false;
    std::uint16_t state = 0;
    bool downPressed = false;
    std::uint32_t word13c = 0;
    std::uint8_t characterClass = 0;
};

bool copWhistleRequested(const CopWhistleGate& gate);

struct CopWhistleEnter {
    int clipRow = kCopWhistleClipRow;    // 0x21941c(row, blend 0.1, speed 1.0, mode 0)
    float blend = kCopWhistleBlend;
    float speed = kCopWhistleSpeed;
};

// 0x276338: the clip, then the tail 0x26e254 (startTrafficWhistleStop) with the game clock and the generator 0x25b2b8.
CopWhistleEnter enterCopWhistle(TrafficWhistleStop& stop, std::uint32_t clock, GameRandom& random);

// 0x29a2e0: when the clip's slot has finished (kind 6 of 0x22f560) the state requests the idle state 1.
std::optional<std::uint16_t> updateCopWhistle(bool clipFinished);

// The game clock 0x241098 `updates` updates after its start (STAND-IN TRAFFIC-S33: its unit and its value at the level start are not read).
std::uint32_t gameClockAfter(std::uint32_t updates);

}
