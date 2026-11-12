#pragma once
// The calls of the traffic AI driver that ai_driver_input.hpp leaves to hooks, for states 5 and 8 (docs/re/traffic-ai-driver-calls.md):
// VehicleSetRaceTrim 0x286080 with the launch byte +0x7a (0x26d8c4) and the release of the held car 0x22a1f4, the body placement 0x23091c of
// the steering controller, the contact slide 0x21a73c / 0x2c8f80 of the contact step, the chase markers of state 5 (0x230dbc, 0x230e60),
// the stop of all traffic after the cop whistle (0x2c87a8, 0x26e254) and the brake lights 0x278b78 of the post step. No SDL dependency.
#include "runtime/ai_driver_input.hpp"
#include "runtime/game_random.hpp"
#include "runtime/objectives.hpp"
#include "runtime/traffic_driver_steering.hpp"
#include "runtime/vehicle_body.hpp"
#include "runtime/vehicle_drivetrain.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr std::uint8_t kTrafficHeldFade = 255;                 // entity byte +0xdd written by 0x26d8c4
constexpr std::uint8_t kTrafficHeldFadeBits = 0x30;            // bits 4 and 5 of byte +0xde
constexpr std::uint16_t kTrafficReleasedVehicleRequest = 4;    // vehicle record hword +4 written by 0x22a1f4

// The global 0x3745d4: the car that VehicleSetRaceTrim holds, 0 when none.
struct TrafficHeldCar {
    std::uint32_t entity = 0;
};

// What VehicleSetRaceTrim and the release read and write of one car.
struct TrafficRaceTrimCar {
    std::uint32_t entity = 0;                  // identity, not 0
    AiDriverRecord* driver = nullptr;          // vehicle record +0x2a4
    TrafficSteeringState* steering = nullptr;  // the same record's bytes +0x7a..+0xa3
    bool meshNode = false;                     // the node word & 31 equals *0x37674c
    std::uint8_t entityFade = 0;               // entity byte +0xdd (0x26d8c4 writes the entity, not its node)
    std::uint8_t entityFadeFlags = 0;          // entity byte +0xde
    std::uint8_t nodeFadeFlags = 0;            // node byte +0xde (0x22a1f4)
    std::uint16_t vehicleRequest = 0;          // vehicle record hword +4
};

struct TrafficRaceTrimResult {
    bool hideTargetHearts = false;  // 0x230f04(0): the hearts display of a chased target is switched off
};

// VehicleSetRaceTrim 0x286080(car, flag): nothing without a driver record. The flag is tested by its bits (any bits but 0, also -0.0).
// Set: +0xaf bit 2 and 0x26d8c4 (0x230f04(0), launch byte +0x7a = 1, and when no car is held yet the entity fade bytes and the hold).
// Clear: +0xaf bit 2 cleared and the release 0x22a1f4.
TrafficRaceTrimResult setTrafficRaceTrim(TrafficRaceTrimCar& car, float flag, TrafficHeldCar& held);

// 0x22a1f4(car): only for the held car. The node fade bits 4 and 5 are set for a mesh node, +0xaf bit 0 and +0x79 are cleared, the
// vehicle request becomes 4 and nobody is held. Also called by the car message 0x80000006.
void releaseTrafficHeldCar(TrafficRaceTrimCar& car, TrafficHeldCar& held);

constexpr std::uint32_t kTrafficPlacementClearedFlags = 0x88000u;  // body +0xc4 bits the placement clears first
constexpr std::uint32_t kTrafficRigidStatic = 0x1u;                // rigid +0xcc bit 0
constexpr std::uint32_t kTrafficRigidKinematic = 0x2u;             // rigid +0xcc bit 1, set by the kinematic switch 0x213c74

// The vehicle body as the placement 0x23091c(body, matrix, room) reads and writes it.
struct TrafficPlacedBody {
    std::uint32_t flags = 0;                  // body +0xc4
    std::uint32_t rigidFlags = 0x100u;        // rigid +0xcc (0x100 from the body build 0x243398)
    std::uint32_t activationState = 1;        // rigid +0xd8
    VehicleRigidBody rigid;                   // velocity +0x13c, angular velocity +0x14c, force +0x1a0, torque +0x1b0, time +0xdc
    AiVec3 interpolationVelocity{};           // rigid +0x84
    AiVec3 interpolationAngularVelocity{};    // rigid +0x94
    float idleRpm = 0.0f;                     // body +0x26c
    float field28c = 0.0f;                    // body +0x28c (meaning not read)
    DrivetrainState drivetrain;               // rpm +0x27c, the wheel spin rates w+0x88 of the wheels at +0x2b8 (count +0x2b4)
    std::uint32_t groundedWheels = 0;         // body +0xda0
    std::uint32_t nodeParent = 0;             // node +4, identity
};

struct TrafficPlacementHooks {
    std::function<std::uint32_t(const AiVec3& point)> roomAt;             // 0x239838: the room holding the point, 0 for none
    std::function<std::uint32_t(std::uint32_t room)> roomNode;            // room +0x14: the node the car is attached to in that room
};

struct TrafficPlacementResult {
    bool placed = false;       // the return value: false when no room holds the matrix position (only the flags are cleared then)
    bool resetFromNode = false;  // kinematic body: vtable +0x18 (0x33c434) resets the body from its node; the matrix is not used
    bool movedToMatrix = false;  // dynamic body: 0x2429fc(body, matrix) and vtable +0xc (0x33c3e8) with the matrix and the node
    bool queueCommand9 = false;  // 0x23f0d4: command 9 of the body queue to the world 0x5863b4 (not read)
    std::uint32_t room = 0;
    bool relinked = false;     // the node was attached to room +0x14 (0x245048, 0x245008, 0x229bf0)
    bool trackCell = false;    // byte 0x376c35 set: the cell tracker 0x20c0e4 runs
};

// 0x23091c(body, matrix, room). The activation 0x2308e4(rigid, 0) wakes only a body without the static and kinematic bits. `room` 0 asks 0x239838 for the room at the matrix position. `trackCells` is the byte 0x376c35.
TrafficPlacementResult placeTrafficBody(TrafficPlacedBody& body, const AiMatrix& matrix, std::uint32_t room, bool trackCells,
                                        const TrafficPlacementHooks& hooks);

// A grid entry the contact step 0x29cc38 gathered (0x237f30, mask 4: characters).
struct TrafficContactEntry {
    std::uint32_t word = 0;           // entry +0: bit 1 skips it in 0x2c8f80, bit 7 is the mark of 0x21a73c
    std::uint32_t owner = 0;          // entry +0x10, identity
    std::uint32_t ownerNode = 0;      // owner +0x3c, identity, 0 = none
    std::uint32_t ownerNodeParent = 0;  // node +4 of the owner's node
};

struct TrafficContactCar {
    std::uint32_t node = 0;           // entity +0x3c, identity
    std::uint32_t nodeParent = 0;     // its node +4
    bool pusher = false;              // entity hword +0x10 bit 0x100
    float radius = 0.0f;              // entity +0x5c
};

constexpr std::uint32_t kTrafficContactSkipBit = 0x2u;
constexpr std::uint32_t kTrafficContactMarkBit = 0x80u;
constexpr float kTrafficContactMoveScale = 0.5f;  // 0x2c9070: the move is cut to half the radius after a push

struct TrafficContactSlideHooks {
    // 0x1fc508(car, owner, 0): the pair does not collide.
    std::function<bool(std::uint32_t owner)> ignorePair;
    // 0x2baef0(car, move, entry, list, count, slide): the push of one character (message 45, its move); makeTrafficCarPush (traffic_car_push.hpp).
    std::function<bool(AiVec3& move, std::size_t entry, std::vector<TrafficContactEntry>& entries, bool slide)> push;
};

// 0x21a73c(car, (move, 0.0), list, count, 255, ...): the entries the car does not collide with get bit 7, 0x2c8f80 pushes the characters
// (the wall part of 0x21a73c is skipped for the mode 255), the marks are cleared. Returns the result of 0x2c8f80: whether a character was
// pushed. 0x2c8f80 stops after the first push when the car is a pusher and cuts the move to half the car's radius after any push.
bool slideTrafficCarContacts(const TrafficContactCar& car, AiVec3& move, std::vector<TrafficContactEntry>& entries,
                             const TrafficContactSlideHooks& hooks);

constexpr std::size_t kTrafficChaseArrowList = 1;      // the enemy arrows (0x230dbc, SubObjectiveArrowEnemyEnable)
constexpr std::uint8_t kTrafficChaseGroupType = 1;     // the enemy group of 0x1fce14 / 0x1fcc68

// The hook `setListed` of state 5 (0x230dbc(car, on), then 0x230e60(0x230ef4(), car, on, 0)): the car's enemy arrow and its place in the
// enemy group whose key is the carry-it waypoint (*(0x529c18 + 0x24)). `carEnabled` is the target flag test of 0x1fce14.
void setTrafficChaseMarkers(ObjectiveState& objectives, const std::string& carryItWaypoint, const std::string& car, bool on, bool carEnabled);

constexpr std::uint32_t kTrafficWhistleStopBase = 120;   // 0x26e270
constexpr std::uint32_t kTrafficWhistleStopRandom = 60;  // 0x26e260

// *(*(0x3746d4 + 0x20) + 0x18): the clock value (0x241098) until which every traffic car stops; 0 after the reset 0x18772c.
struct TrafficWhistleStop {
    std::uint32_t until = 0;
};

// 0x26e254, the tail of the enter 0x276338 of the player state 0x1b2 COPWHISTLE: until = clock + random % 60 + 120.
void startTrafficWhistleStop(TrafficWhistleStop& stop, std::uint32_t clock, GameRandom& random);
// 0x2c87a8 (the hook `stopAll` of the post step): clock < until, unsigned.
bool trafficWhistleStops(const TrafficWhistleStop& stop, std::uint32_t clock);

constexpr float kTrafficBrakeLightFade = 2.0f;  // 0x278d34: the level falls by 2 x dt per update
constexpr float kTrafficBrakeLightAlpha = 255.0f;
constexpr std::size_t kTrafficBrakeLightParts = 2;
constexpr std::size_t kTrafficBrakeGlows = 4;

// The brake lights of the vehicle record: mesh parts +0x2ac, +0x2b0 (-1 = none), glow parts +0x2b4..+0x2c0 (-1 = none), the glow effect
// handles +0x2c4..+0x2d0 (0 = none), the level +0x2d4 and the glow effect id +0x2d8.
struct TrafficBrakeLights {
    std::array<std::int32_t, kTrafficBrakeLightParts> parts{-1, -1};
    std::array<std::int32_t, kTrafficBrakeGlows> glowParts{-1, -1, -1, -1};
    std::array<std::uint32_t, kTrafficBrakeGlows> glows{};
    float level = 0.0f;
    std::uint32_t glowEffect = 0;
};

struct TrafficBrakeLightFrame {
    bool hasBody = true;       // entity +0x84 set
    int gear = 1;              // body +0x294 (0 = reverse)
    bool nodeCulled = false;   // the node word bit 0x40000
    float brake = 0.0f;        // s0: 1.0 from the post step when driver +0x70 or +0x68 is 0, else 0.0
    float dt = 0.0f;           // s2
};

struct TrafficBrakeLightPart {
    std::int32_t part = -1;
    bool shown = false;        // 0x2363d4(node, part, shown)
    std::uint8_t alpha = 0;    // 0x23f124(node, alpha, part, 0) when shown
};

struct TrafficBrakeLightResult {
    std::vector<std::size_t> glowStarts;   // 0x23de24(+0x2d8, the part's position, node, 1, 0.0, 0, 0) for the glow slot; the caller stores
                                           // the handle, clears its flag 0x2000, sets its matrix (0x241540) and the end call 0x2cb42c
    std::vector<std::size_t> glowStops;    // 0x23ddb4(handle, 0.0); the handle stays until the effect's end call clears it
    std::vector<TrafficBrakeLightPart> parts;  // only when the level changed
};

// 0x278b78(entity, brake, 0.0, dt): nothing without a first light part. The level is 1.0 while braking in a forward gear or without a body,
// else it falls by 2 x dt to 0. While it is 1.0 and the node is not culled, a glow starts in every glow slot without one; otherwise the
// running glows are stopped. When the level changed, the parts are shown with alpha (int)(255 x level) & 255, or hidden at alpha 0.
TrafficBrakeLightResult runTrafficBrakeLights(TrafficBrakeLights& lights, const TrafficBrakeLightFrame& frame);

// 0x2cb42c(handle, event, entity): at event 2 (the effect ended) every glow slot holding the handle is cleared.
void endTrafficBrakeGlow(TrafficBrakeLights& lights, std::uint32_t handle, int event);

}
