#pragma once
// The vehicle side of a driven traffic car (docs/re/traffic-car-body.md): what the AI driver and the traffic manager ask of the car's
// vehicle physics body, and the body's kinematic life: the switch 0x213c74 with the body commands of the world queue (0x1fb648, run by
// 0x2e3390), the world membership 0x243300 of the car messages 0x80000004 / 0x80000003, the node copy of the world update 0x2e2bc8 and
// the kinematic branch of the body step 0x33aea8 with the floor cell tracker 0x20c0e4. A car is named by its index in the traffic
// manager's table. No SDL, no entities.
#include "runtime/ai_driver_input.hpp"
#include "runtime/vehicle_collision.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace chase::runtime {

struct TrafficBodyHooks {
    // 0x22a270(entity, on) -> 0x213c74(body, on): the body kinematic (on) or dynamic (off).
    std::function<void(std::size_t car, bool on)> switchKinematic;
    // 0x2308e4(rigid, 1): the rigid body woken.
    std::function<void(std::size_t car)> wakeBody;
    // 0x242518(body, *0x376c40): the body's gravity back to the global one.
    std::function<void(std::size_t car)> normalGravity;
    // 0x23019c(entity): the driver's inputs copied into the body.
    std::function<void(std::size_t car)> copyInputs;
    // The vehicle-side step of a driven car after its driver ran: the body follows the node (kinematic) or the node follows the body.
    std::function<void(std::size_t car, float dt)> step;
    // The car messages 0x80000004 (on) and 0x80000003 (off): 0x243300(body, on), the body in or out of the physics world.
    std::function<void(std::size_t car, bool on)> setActive;
    // Body +0x8c: the floor cell the traffic spawn 0x2def8c wrote (-1 for a point in no cell).
    std::function<std::int32_t(std::size_t car)> floorCell;
    // Rigid +0xcc has bit 1 (value 2), the test of the route mover 0x2613ac at 0x26161c before it stores the velocity.
    std::function<bool(std::size_t car)> kinematic;
    // 0x229920(body, velocity): rigid +0x13c..+0x144 = velocity, +0x148 = 0.
    std::function<void(std::size_t car, const AiVec3& velocity)> setVelocity;
};

// Body +0xc4.
constexpr std::uint32_t kTrafficBodyFlagListed = 0x1;        // the world's entity list 0x2e2bc8 and the gravity of the add command need it
constexpr std::uint32_t kTrafficBodyFlagKinematic = 0x20;    // set by the switch 0x213c74
constexpr std::uint32_t kTrafficBodyFlagWideMask = 0x40;     // the dynamic command gives the mask 0xffff instead of 63
constexpr std::uint32_t kTrafficBodyFlag8000 = 0x8000;       // cleared by the switch; its meaning is not read
constexpr std::uint32_t kTrafficBodyFlagsPlaced = 0x88000;   // cleared by the placement 0x23091c; their meaning is not read
// Body byte +0x90.
constexpr std::uint8_t kTrafficBodyWanted = 0x1;             // 0x243300 asked for the body in the world
constexpr std::uint8_t kTrafficBodyBuilt = 0x2;              // the rigid body exists, so 0x243300 queues its commands
// Rigid +0xcc (Bullet's collision flags).
constexpr std::uint32_t kRigidFlagStatic = 0x1;
constexpr std::uint32_t kRigidFlagKinematic = 0x2;
constexpr std::uint32_t kRigidFlagOutOfWorld = 0x80;
constexpr std::uint32_t kRigidFlagBuilt = 0x100;             // set by the body build 0x243398
// Rigid +0xd8 (Bullet's activation state).
constexpr std::uint32_t kRigidActive = 1;
constexpr std::uint32_t kRigidSleeping = 2;
constexpr std::uint32_t kRigidNeverSleeps = 4;
constexpr std::uint32_t kRigidNoSimulation = 5;
// The collision filter of the commands 4 and 3 of 0x2e3390 (body hwords +0x76 group, +0x78 mask).
constexpr std::uint16_t kTrafficKinematicGroup = 2;
constexpr std::uint16_t kTrafficKinematicMask = 61;
constexpr std::uint16_t kTrafficDynamicGroup = 1;
constexpr std::uint16_t kTrafficDynamicMask = 63;
constexpr std::uint16_t kTrafficDynamicWideMask = 0xffff;
// 0x33b090..0x33b09c: a body with the kinematic flag whose rigid body is not kinematic yet counts 4 wheels on the ground (+0xda0).
constexpr int kTrafficSwitchingWheelsOnGround = 4;
// The mass scales 0x213dc8 gets from the globals 0x3745b4 (the player's vehicle, 0x2f7090 and 0x3200bc) and 0x3745b0 (a car the chase
// start 0x1afda0 puts into driver state 5). The static initialiser 0x34498c stores both (literals 0x3449e4, 0x3449e0).
constexpr float kPlayerVehicleMassScale = 2.0f;
constexpr float kChasedCarMassScale = 6.0f;

// The commands 0x1fb648 queues at the world 0x5863b4 for a rigid body; 0x1ed6a8 runs them at the start of the world update 0x2e2bc8.
enum class TrafficBodyCommand : std::uint8_t {
    Add = 0,        // 0x243300 with 1
    Remove = 1,     // 0x243300 with 0
    Dynamic = 3,    // 0x23f0fc
    Kinematic = 4,  // 0x2432d8
};

struct TrafficBodyWheel {
    float radius = 0.0f;     // w+0x40
    float mountX = 0.0f;     // w+0x50: a negative mount turns the wheel node half a turn and spins it the other way
    float mountY = 0.0f;     // w+0x54
    float restY = 0.0f;      // w+0x60: the y of the vector at w+0x5c, whose meaning is not read
    float spinAngle = 0.0f;  // w+0x80
    float spinRate = 0.0f;   // w+0x88
    float rayLength = 0.0f;  // w+0xe0
};

struct TrafficBodyState {
    std::uint32_t flags = 0;                           // body +0xc4
    std::uint8_t world = kTrafficBodyBuilt;            // body byte +0x90
    std::uint32_t rigidFlags = kRigidFlagBuilt | kRigidFlagOutOfWorld;  // rigid +0xcc
    std::uint32_t activation = kRigidActive;           // rigid +0xd8
    float deactivationTime = 0.0f;                     // rigid +0xdc
    float gravity = 0.0f;                              // the y of the rigid body's gravity (0x242518 -> 0x1e64bc)
    bool massless = false;                             // 0x1f3124 with mass 0 (command 4) or the body's mass +0xc8 (command 3)
    std::uint16_t group = kTrafficDynamicGroup;        // body hword +0x76
    std::uint16_t mask = kTrafficDynamicMask;          // body hword +0x78
    AiVec3 centreOfMass{};                             // body +0x50
    AiMatrix rigid{};                                  // rigid +4..+0x40 in the node layout: rows right, up, forward, origin
    AiMatrix matrix{};                                 // body +4: the rigid transform as the last body step copied it
    AiVec3 velocity{};                                 // rigid +0x13c
    float forwardSpeed = 0.0f;                         // body +0xc0
    FloorTracker floor;                                // body +0x8c and +0x1a4
    int wheelsOnGround = 0;                            // body +0xda0
    std::vector<TrafficBodyWheel> wheels;
    std::vector<TrafficBodyCommand> queue;
};

// 0x243300(body, on): with the body built and the wanted bit changing, the Add or Remove command is queued; the wanted bit follows `on`.
void setTrafficBodyInWorld(TrafficBodyState& body, bool on);
// 0x213c74(body, on). On: gravity 0, activation 4, each wheel's ray length = w+0x54 - w+0x60 + radius, the kinematic flag set and bit
// 0x8000 cleared; unless the rigid body is kinematic already, its transform is taken from `node` (vtable +0x18 = 0x33c434) and the
// Kinematic command is queued. Off, only with the kinematic flag set: activation 1, gravity `globalGravity`, the flag cleared and the
// Dynamic command queued.
void switchTrafficBodyKinematic(TrafficBodyState& body, bool on, const AiMatrix& node, float globalGravity);
// 0x1ed6a8 -> 0x2e3390 for each queued command of the body, in order.
void runTrafficBodyCommands(TrafficBodyState& body, float globalGravity);
// 0x2308e4(rigid, 1): activation 1 unless it is 4 or 5, and the deactivation time 0.
void wakeTrafficBody(TrafficBodyState& body);
// 0x229920.
void setTrafficBodyVelocity(TrafficBodyState& body, const AiVec3& velocity);
// 0x20cc30(body, node) -> 0x2429fc: the rigid transform = the node's rotation with the origin at the node's image of the centre of mass.
void placeTrafficBodyAtNode(TrafficBodyState& body, const AiMatrix& node);
// The test of the world update 0x2e2bc8 (0x2e2cb4..0x2e2d08) before the world step: a body of the world that is neither asleep (2) nor
// out of the simulation (5), whose entity hword +0x10 has neither bit 0 nor bit 5 and whose rigid body is kinematic, takes its node.
bool trafficBodyFollowsNode(const TrafficBodyState& body, bool entityDisabled, bool entityFlag20);

enum class TrafficBodyStepKind {
    Kinematic,  // the wheels spun by the forward speed, nothing else
    Switching,  // the kinematic flag without a kinematic rigid body: 4 wheels on the ground, nothing else
    Dynamic,    // the dynamic body step follows (not part of this file)
};

// One world step 0x33aea8(body, dt) as far as a kinematic car goes: body +4 = the rigid transform, +0xc0 = the velocity along the
// forward row, the floor cell tracker 0x20c0e4 (the byte 0x376c35 is 1 in the data image) when the level has a floor graph, then for a
// kinematic rigid body per wheel the spin rate = forward speed / (radius x 2 pi) x 2 pi and the spin angle += rate x dt.
TrafficBodyStepKind stepTrafficBody(TrafficBodyState& body, float dt, const FloorMesh* floor);

// The bodies of the traffic cars with the hooks the AI driver and the traffic manager use.
class TrafficCarBodies {
public:
    struct Car {
        AiVec3 centreOfMass{};
        std::vector<TrafficBodyWheel> wheels;
        std::uint32_t flags = 0;  // body +0xc4 after the build
    };
    // Each body as the level load leaves it: built and out of the world, then the driver creation 0x1796e8 (0x22a270(entity, 1)) with
    // its command run: kinematic, rigid flags 0x182. The link 0x156f74 puts the body into the world and takes it out again before;
    // that pair is left out.
    void load(const std::vector<Car>& cars, const std::vector<AiMatrix>& nodes, float globalGravity);

    // What the hooks cannot know themselves; all three must outlive the hooks.
    struct World {
        std::function<AiMatrix(std::size_t car)> node;      // the node's world matrix (0x2429d0)
        const FloorMesh* floor = nullptr;                    // the level's floor graph, null without one
        std::function<void(std::size_t car)> copyInputs;    // 0x23019c of a dynamic car; may be empty while no car turns dynamic
    };
    TrafficBodyHooks hooks(const World& world);

    // 0x2def8c at 0x2df3c4..0x2df48c: body +0x8c = the spawn point's floor cell, the transform from the placed node (vtable +0x18), then
    // the placement 0x23091c as far as it reaches these fields: flags 0x88000 cleared, the velocity, the wheels' spin rates and the
    // wheels on the ground 0, the wake 0x2308e4(rigid, 0) of a body that is neither static nor kinematic, and the cell tracker
    // 0x20c0e4, which walks from the body's last position +0x1a4 with the cell just written.
    void spawn(std::size_t car, std::int32_t floorCell, const AiMatrix& node, const FloorMesh* floor);
    // The start of the world update 0x2e2bc8 for every body: the queued commands, then the node copy of the kinematic bodies.
    // `skip(car)` is true for a car whose entity hword +0x10 has bit 0 or bit 5.
    void beforeWorldStep(const World& world, const std::function<bool(std::size_t car)>& skip);

    std::size_t size() const { return bodies_.size(); }
    const TrafficBodyState& body(std::size_t car) const { return bodies_.at(car); }
    TrafficBodyState& body(std::size_t car) { return bodies_.at(car); }

private:
    std::vector<TrafficBodyState> bodies_;
    float globalGravity_ = 0.0f;
};

}
