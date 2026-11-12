#pragma once
// The AI driver of the traffic cars (docs/re/traffic.md): the driver record (vehicle record +0x2a4) made by 0x1796e8, the input provider
// 0x26d934, the state change 0x1cb148 run by the vehicle update 0x1571dc, and the traffic post step 0x1bc79c run by 0x16ce9c. No SDL
// dependency. Functions of the original that are not ported here are hooks, called in the original's order.
#include "runtime/game_random.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <vector>

namespace chase::runtime {

using AiVec3 = std::array<float, 3>;
// Rows x, y, z and position, as in the original's 64-byte matrices.
using AiMatrix = std::array<AiVec3, 4>;

constexpr std::uint8_t kAiDriverFlagAi = 0x01;           // +0xaf bit 0: the vehicle update 0x1571dc uses 0x26d934
constexpr std::uint8_t kAiDriverFlagEffect = 0x02;       // +0xaf bit 1: state 5 runs 0x1bc660 every 64 updates
constexpr std::uint8_t kAiDriverFlagFullSpeed = 0x04;    // +0xaf bit 2: state 5 limits +0x70 with factor 1.0, else 0.8
constexpr std::uint8_t kAiDriverFlagSignal = 0x08;       // +0xaf bit 3: the car is queued at its signal object
constexpr std::uint8_t kAiDriverFlagListed = 0x10;       // +0xaf bit 4: tested before 0x230dbc on entering state 5

constexpr std::uint8_t kAiStateHold = 0;
constexpr std::uint8_t kAiStateStart = 1;
constexpr std::uint8_t kAiStateRoute = 2;
constexpr std::uint8_t kAiStateJunction = 3;
constexpr std::uint8_t kAiStateKnocked = 4;
constexpr std::uint8_t kAiStateSteer = 5;
constexpr std::uint8_t kAiStateStopping = 6;
constexpr std::uint8_t kAiStateBreak = 7;
constexpr std::uint8_t kAiStateReturn = 8;

constexpr std::uint8_t kAiHoldUpdates = 8;
constexpr std::uint32_t kAiCheckPeriod = 5;
constexpr float kAiObstacleMargin = 3.0f;
constexpr float kAiObstacleSideMargin = 0.5f;
constexpr float kAiSteerLookAhead = 8.0f;
constexpr float kAiSteerLookStep = 0.5f;
constexpr unsigned kAiNearestSamples = 10;
constexpr float kAiNearestScale = 1.0f;
constexpr float kAiSteerFullFactor = 1.0f;
constexpr float kAiSteerFactor = 0.8f;
constexpr float kAiReturnProbe = 8.0f;
constexpr float kAiReturnAlignedDot = 0.9f;
constexpr float kAiReturnNearAligned = 10.0f;
constexpr float kAiReturnNear = 4.0f;
constexpr float kAiCruiseBase = 8.0f;
constexpr float kAiCruiseRange = 4.0f;
constexpr float kAiTargetStep = 0.1f;
constexpr std::uint8_t kAiStartHearts = 100;
constexpr std::uint32_t kAiKeepRandomRange = 60;
constexpr std::uint8_t kAiKeepBase = 30;
constexpr std::uint32_t kAiEffectPeriodMask = 63;
constexpr float kAiRouteEndMargin = 2.0f;
constexpr float kAiJunctionLength = 2.8f;
constexpr float kAiJunctionRandomStart = 0.4f;
constexpr float kAiKnockedSpeed = 0.5f;
constexpr float kAiKnockedSeconds = 0.5f;
constexpr float kAiStoppedSpeed = 0.1f;
constexpr std::uint32_t kAiStoppingUpdates = 90;
constexpr std::uint8_t kAiKeepOffScreen = 30;

// The 176-byte driver record (0x1796e8 allocates it zero-filled with 0x25c030 and its zero flag).
struct AiDriverRecord {
    std::uint32_t signal = 0;          // +0: the route's signal object (0x20ff40), 0 = none
    std::uint32_t route = 0;           // +4
    std::uint32_t nextRoute = 0;       // +8: the route 0x2cb1e8 chose at the junction
    AiVec3 target{};                   // +0xc: the route point 0.1 past +0x64
    AiMatrix path{};                   // +0x18 (rows +0x18, +0x28, +0x38, +0x48)
    AiVec3 entryPosition{};            // +0x58: the node position when state 2, 5 or 8 was entered
    float routeDistance = 0.0f;        // +0x64
    float speed = 0.0f;                // +0x68
    float cruiseSpeed = 0.0f;          // +0x6c
    float targetSpeed = 0.0f;          // +0x70
    float heightOffset = 0.0f;         // +0x74: attribute TrafficHeightOffRoad (default 1.0)
    std::uint8_t state = 0;            // +0x78
    std::uint8_t requestedState = 0;   // +0x79
    std::uint32_t stateUpdates = 0;    // +0xa4: updates since the state was entered (0x1cb148)
    float contactTime = 0.0f;          // +0xa8: 0x2355c0 at the last contact message 0x80000002
    std::uint8_t hold = 0;             // +0xac
    std::uint8_t keep = 0;             // +0xad: the traffic manager keeps the car while it is not 0
    std::uint8_t moveCounter = 0;      // +0xae: cleared with the target, counted by 0x2613ac
    std::uint8_t flags = 0;            // +0xaf
};

// The vehicle record fields the driver code writes.
struct AiDriverVehicle {
    float steerInput = 0.0f;           // record +0x288
    float accelerateInput = 0.0f;      // record +0x28c
    float brakeInput = 0.0f;           // record +0x290
    float handBrakeInput = 0.0f;       // record +0x294
    std::uint8_t health = 0;           // record +0x30c
    std::uint8_t hearts = 0;           // record +0x30d
    std::uint8_t maxHearts = 0;        // record +0x30e
    std::uint8_t forcedStop = 0;       // record +0x310 (0x229be4)
    bool entityFlag20000 = false;      // entity word +0xc bit 0x20000
};

// STAND-IN TRAFFIC-S5: the route type functions (table 0x3581d4, five words per type byte route +8) and the path follower 0x2bf2ec are
// supplied by the caller until the route port (traffic_route.*) is connected.
struct AiRouteFunctions {
    // Entry +0 with r3 = 1: the point and, when `direction` is set, the tangent at `distance`.
    std::function<void(std::uint32_t route, float distance, AiVec3* position, AiVec3* direction)> evaluate;
    // Entry +8: the distance `ahead` along the route from `from`, walked in steps of `step`.
    std::function<float(std::uint32_t route, float ahead, float step, float from)> ahead;
    // Entry +0xc: the distance of the route sample nearest to `position` (`samples` per segment, distances divided by `scale`).
    std::function<float(std::uint32_t route, const AiVec3& position, unsigned samples, float scale)> nearest;
    // 0x2bf2ec(probe, matrix row 2, driver +0xc, driver +4, driver +0x64): writes the matrix rows it finds and returns whether the car is
    // back on a route.
    std::function<bool(const AiVec3& probe, AiMatrix& matrix, AiVec3& target, std::uint32_t& route, float& distance)> follow;
    // Route hword +10.
    std::function<std::uint16_t(std::uint32_t route)> length;
    // 0x20ff40.
    std::function<std::uint32_t(std::uint32_t route)> signal;
};

// Functions of the original the driver code calls and the port does not contain yet. A hook the taken path needs and the caller did not set
// is refused with std::logic_error.
struct AiDriverHooks {
    std::function<void(const AiMatrix& target, float dt)> steerToward;     // 0x27872c(entity, target, dt)
    std::function<void()> copyInputs;                                       // 0x23019c(entity)
    std::function<void(const AiMatrix& matrix)> setNodeMatrix;              // 0x24c118(node, matrix)
    std::function<bool(std::uint32_t signal)> signalLetsPass;               // 0x2cdde4(signal, entity)
    std::function<void(std::uint32_t signal)> leaveSignalQueue;             // 0x22a080: the queue of the signal object
    std::function<void(bool kinematic)> setKinematic;                       // 0x22a270(entity, on) -> 0x213c74
    std::function<void(bool on)> setListed;                                 // 0x230dbc, 0x230ef4, 0x230e60
    std::function<void()> snapTarget;                                       // 0x20ae2c(entity)
    std::function<bool(AiDriverRecord& driver, std::uint32_t& next)> chooseNextRoute;  // 0x2cb1e8(entity, &next)
    std::function<void()> leaveTraffic;                                     // 0x21d6b8(entity)
    std::function<void()> enterStopping;                                    // 0x229be4, 0x23cee8, 0x22a124(entity, 0x11f)
    std::function<void()> destroy;                                          // 0x244fc8: message 0 to itself with word +0xc = 1000
    std::function<void()> wakeBody;                                         // 0x2308e4(rigid, 1)
    std::function<void(const AiVec3& local)> rearEffect;                    // 0x254250 of `local` by the node, then 0x1bc660
    std::function<bool()> stopAll;                                          // 0x2c87a8()
    std::function<void(bool stopped, float dt)> lights;                     // 0x278b78(entity, stopped ? 1 : 0, 0, dt)
    std::function<bool()> stopAtRouteEnd;                                   // 0x2ca504(entity)
    std::function<void(AiDriverRecord& driver)> moveAlongRoute;             // 0x2613ac(entity)
    std::function<void()> normalGravity;                                    // 0x242518(body, *0x376c40)
    std::function<void()> steerStep;                                        // 0x29cc38(entity)
    std::function<bool()> nodeHidden;                                       // 0x23de70(entity)
};

// Another entity of the obstacle rule of states 2 and 3.
struct AiObstacle {
    AiMatrix node{};
    AiVec3 halfExtents{};              // entity +0x6c..+0x74
};

struct AiDriverInputFrame {
    AiMatrix node{};                   // the car's node matrix (0x250810)
    AiVec3 halfExtents{};              // entity +0x6c..+0x74
    float bodyBase = 0.0f;             // body +0x54 (0x20af24 = driver +0x74 - body +0x54)
    float dt = 0.0f;                   // 0x23602c
    bool playerInVehicle = false;      // 0x24255c(*0x3772b8) != 0
    float gearSpeed = 0.0f;            // 0x20362c(entity +0x84)
    bool hasBody = true;               // entity +0x84 set
    std::uint32_t collisionFlags = 0;  // body +0x7c +0xcc
    bool layerBit = false;             // 0x1ffbd8(entity byte +0x18, room byte +0x3a)
    // The list at 0x5863b4 + 0x335c (count +0x3058) without the car, then the player entity.
    std::vector<AiObstacle> obstacles;
    const AiRouteFunctions* routes = nullptr;
};

struct AiDriverInputResult {
    bool clearCollisionFlag200 = false;  // state 5: body +0x7c +0xcc bit 0x200 cleared
};

// 0x26d934: jumps by driver +0x78 - 2 through the table at 0x26d97c.
AiDriverInputResult runAiDriverInput(AiDriverRecord& driver, const AiDriverVehicle& vehicle, const AiDriverInputFrame& frame,
                                     const AiDriverHooks& hooks);

struct AiDriverStateFrame {
    AiMatrix node{};                   // the node matrix (0x250810)
    AiMatrix body{};                   // the rigid body's basis rows rigid +4, +0x14, +0x24 and origin +0x34 (physics layout)
    float bodyBase = 0.0f;             // body +0x54
    float bodySpeed = 0.0f;            // 0x229948(body)
    std::uint32_t activationState = 0; // rigid +0xd8
    const AiRouteFunctions* routes = nullptr;
};

// 0x1cb148, run by the vehicle update 0x1571dc (call at 0x1574a4) for a car whose driver has +0xaf bit 0.
void runAiDriverStateChange(AiDriverRecord& driver, AiDriverVehicle& vehicle, const AiDriverStateFrame& frame, const AiDriverHooks& hooks,
                            GameRandom& random);

struct AiDriverPostFrame {
    AiMatrix node{};                   // the node matrix (0x2429d0)
    float dt = 0.0f;                   // 0x23602c
    float bodySpeed = 0.0f;            // 0x229948(body)
    float bodyMoveSpeed = 0.0f;        // 0x230174(body)
    float now = 0.0f;                  // 0x2355c0()
    std::uint32_t wheelCount = 0;      // body +0x2b4
    std::uint32_t groundedWheels = 0;  // body +0xda0
    const AiRouteFunctions* routes = nullptr;
};

// 0x1bc79c, run by the post step 0x16ce9c instead of 0x23019c for a car whose driver has +0xaf bit 0.
void runAiDriverPostStep(AiDriverRecord& driver, AiDriverVehicle& vehicle, const AiDriverPostFrame& frame, const AiDriverHooks& hooks,
                         GameRandom& random);

// 0x1796e8 (from the traffic manager's level load 0x2dfa78): the zero-filled record, +0x74 = the attribute, +0x18 = the node matrix,
// +0xaf |= 1, then 0x22a270(entity, 1).
AiDriverRecord createAiDriverRecord(float heightOffset, const AiMatrix& node, const AiDriverHooks& hooks);

}
