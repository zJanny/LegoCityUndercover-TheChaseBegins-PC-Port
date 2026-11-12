#pragma once
// The AI driver of the spawned traffic cars connected to the route port (docs/re/traffic-ai-driver.md): the driver record of each car
// (0x1796e8), the driver fields the spawn 0x2def8c writes, and one vehicle update of a live car: the input provider 0x26d934 and the state
// change 0x1cb148 of the vehicle update 0x1571dc, then the post step 0x1bc79c of 0x16ce9c. Its calls are the ported route functions of
// the junctions' paths with the path follower 0x2bf2ec over the pregen cells, the junction test 0x2cdde4, the queue 0x22a080, the stop
// test 0x2ca504, the route mover 0x2613ac with the junction choice 0x2cb1e8, the leave call 0x21d6b8 and the ground probe 0x20ae2c.
// The record's +0x74 holds 0x20af24 (driver +0x74 - body +0x54) and the frames pass body +0x54 as 0: every reader takes the difference.
// No SDL, no entities.
#include "assets/pregen_traffic.hpp"
#include "runtime/ai_driver_input.hpp"
#include "runtime/game_random.hpp"
#include "runtime/traffic_car_body.hpp"
#include "runtime/traffic_car_lights.hpp"
#include "runtime/traffic_driver_calls.hpp"
#include "runtime/traffic_driver_states.hpp"
#include "runtime/traffic_driver_steering.hpp"
#include "runtime/traffic_junction_update.hpp"
#include "runtime/traffic_leave_snap.hpp"
#include "runtime/traffic_manager.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace chase::runtime {

// The route type functions of trafficRouteFunctions with the path follower 0x2bf2ec: the cell of `spawns` under the probe (0x1f47f0), the
// route bits of `table`, the probe's forward axis from the matrix row 2; a hit writes the route point (driver +0xc), the route (+4) and its
// parameter (+0x64). The three arguments must outlive the functions.
AiRouteFunctions trafficDriverRouteFunctions(const TrafficJunctions& junctions, const assets::PregenSpawns& spawns, const TrafficRouteTable& table);

// One car: its driver record (vehicle record +0x2a4) and the vehicle record fields the driver writes.
struct TrafficDriverCar {
    AiDriverRecord driver;
    AiDriverVehicle vehicle;
    TrafficSteeringState steering;  // the record's bytes +0x7a..+0xa3
    // The brake lights of the vehicle record (+0x2ac..+0x2d8). Their parts are not set up (TRAFFIC-33), so 0x278b78 does nothing yet.
    TrafficBrakeLights brakeLights;
};

// What the vehicle side gives of one car for its update.
struct TrafficCarBody {
    AiMatrix node{};        // the node's world matrix (0x250810, 0x2429d0)
    AiVec3 centre{};        // entity +0x60..+0x68
    AiVec3 halfExtents{};   // entity +0x6c..+0x74
    float radius = 0.0f;    // entity +0x5c: the length of the half extents
    float gearSpeed = 0.0f; // 0x20362c(body)
    // The physics body's own readings, which the states 4 to 8 use (docs/re/traffic-ai-driver.md): the speed along the forward row
    // 0x229948, the speed 0x230174, the activation state rigid +0xd8 and the wheels on the ground +0xda0 of the wheel count +0x2b4.
    float forwardSpeed = 0.0f;
    float moveSpeed = 0.0f;
    std::uint32_t activationState = 0;
    std::uint32_t wheelCount = 0;
    std::uint32_t groundedWheels = 0;
};

// An entity of the obstacle list of states 2 and 3; an entry of a traffic car names it, and a car skips its own entry.
struct TrafficObstacle {
    AiObstacle entity;
    std::optional<std::size_t> car;
};

struct TrafficDriveStep {
    float dt = 0.0f;  // 0x23602c
    // STAND-IN TRAFFIC-S27: the list 0x5863b4 + 0x335c (count +0x3058) and the player as the caller gathers them.
    std::vector<TrafficObstacle> obstacles;
    // 0x20ae2c: the ray cast and the colour receiver of the probe.
    TrafficGroundWorld ground;
    // The game clock 0x241098 of the whistle stop 0x2c87a8 (its unit is TRAFFIC-34; without a whistle the stop's end is 0 and no car stops).
    std::uint32_t clock = 0;
    // The clock 0x2355c0 that the driver's input provider reads (the time since the last contact message 0x80000002, driver +0xa8).
    // STAND-IN TRAFFIC-S37: the caller supplies it in seconds from the update count.
    float now = 0.0f;
    // The vehicle of a car; none for a car without a vehicle, which is not updated.
    std::function<std::optional<TrafficCarBody>(std::size_t car)> body;
    // The hook setListed of state 5: the car's chase markers (setTrafficChaseMarkers).
    std::function<void(std::size_t car, bool on)> chaseMarkers;
    // The grid gather 0x237f30 of the contact step 0x29cc38 (characters, mask 4) and the slide 0x21a73c of what it listed.
    // makeTrafficContactGather (traffic_character_gather.hpp) is the gather, set by chase-game (game_traffic_characters.cpp); unset it lists nothing.
    std::function<std::vector<TrafficContactEntry>(std::size_t car, const AiVec3& centre, const AiVec3& halfExtents,
                                                   const TrafficContactDescriptor& descriptor, unsigned maxEntries)>
        gatherContacts;
    TrafficContactSlideHooks slide;
    // The entity hword +0x10 of the traffic cars: bit 0x100 makes a car a pusher (no slide in 0x2c8f80). 0x1300 for the shipped traffic vehicles.
    std::uint16_t carFlags = 0;
    // The push of one car (makeTrafficCarPush over the world); when set it replaces `slide.push` for each car's contact slide.
    std::function<std::function<bool(AiVec3& move, std::size_t entry, std::vector<TrafficContactEntry>& entries, bool slide)>(std::size_t car)> pushOf;
    // *0x376c78 is set: entering state 6 fires VehicleChaseEnded a second time at its object.
    bool globalObject = false;
    // The cars' physics bodies (TrafficCarBodies::hooks): the kinematic switch 0x22a270, the mover's test of rigid +0xcc bit 1 and its
    // velocity 0x229920. Members left empty stand for a car without a physics body: no switch, not kinematic, no velocity.
    TrafficBodyHooks bodies;
    // The brake lights and the node culling bits of the cars (TRAFFIC-33, TRAFFIC-37); without it the lights are not set up and the node is
    // taken as hidden (STAND-IN TRAFFIC-S35).
    TrafficCarLights* lights = nullptr;
};

enum class TrafficDriveEventKind {
    Stop,      // the stop test 0x2ca504 set the brake input +0x290 to 1.0
    Go,        // the post step released the brake input again
    Junction,  // the car took its junction lane (state 3)
    Road,      // the car left the lane onto its next road (state 3 -> 2)
};

struct TrafficDriveEvent {
    TrafficDriveEventKind kind;
    std::size_t car = 0;
    std::uint32_t signal = 0;  // the junction's signal id (driver +0) at the event
};

struct TrafficCarMove {
    std::size_t car = 0;
    AiMatrix node{};  // 0x24c118(node, matrix)
};

struct TrafficCarStateEvent {
    std::size_t car = 0;
    TrafficStateEvent event;
};

struct TrafficDriveResult {
    std::vector<TrafficEvent> traffic;     // removals of the leave call 0x21d6b8
    std::vector<TrafficDriveEvent> events;
    std::vector<TrafficCarMove> moves;
    std::vector<TrafficCarStateEvent> states;  // the calls of states 4 to 8 (traffic_driver_states)
};

class TrafficDrivers {
public:
    // 0x1796e8 for each car of the level load 0x2dfa78 (+0x74 = the car's 0x20af24, +0xaf bit 0). Its 0x22a270(entity, 1) is done by the
    // load of the bodies (TrafficCarBodies::load). Its +0x18 copy of the level node is replaced by the spawn before any reader, so the
    // rows start at 0.
    void load(const std::vector<float>& heightOffsets);
    // 0x2def8c steps 3 and 5: +0x79 = 1, +0x78 = 0, +0xaf bit 0, +4 = the route, +0 = its signal object, +0x64 = the point's parameter,
    // +0x18 = the placement matrix.
    void start(std::size_t car, std::uint32_t route, std::uint32_t signal, float parameter, const AiMatrix& node);
    // One update of every car the manager has live: 0x26d934, 0x1cb148 and 0x1bc79c in that order, car by car. The manager's view of each
    // car (TrafficCar) takes the record's +0x78, +0x79, +0xad, +0x68, +0x64, the route's index and the node position.
    TrafficDriveResult drive(TrafficManager& manager, TrafficJunctionWorld& junctions, const AiRouteFunctions& routes, const TrafficDriveStep& step,
                             GameRandom& random);
    // The car message 0x80000006 of a removal as far as it reaches the driver: first the AI driver's part 0x19293c (which the vehicle handler
    // 0x1797c4 calls for a driver with +0xaf bit 0): with driver +0 set, 0x22a080(junction, car), which takes the car out of the junction's
    // queue when +0xaf bit 3 is set; then the vehicle handler's release 0x22a1f4 of the held car.
    void removed(std::size_t car, TrafficJunctionWorld& junctions);
    // 0x22a080(junction, car) for a car with its driver's +0xaf bit 0 and a signal object: the car leaves the junction's queue.
    void leaveQueue(std::size_t car, TrafficJunctionWorld& junctions);
    // removed() for each removal among `events`.
    void removed(const std::vector<TrafficEvent>& events, TrafficJunctionWorld& junctions);

    std::size_t size() const { return cars_.size(); }
    const TrafficDriverCar& car(std::size_t index) const { return cars_.at(index); }
    TrafficDriverCar& mutableCar(std::size_t index) { return cars_.at(index); }

    // The global 0x3745d4 of VehicleSetRaceTrim and the whistle stop of the cop whistle.
    TrafficHeldCar& held() { return held_; }
    TrafficWhistleStop& whistle() { return whistle_; }

private:
    std::vector<TrafficDriverCar> cars_;
    TrafficHeldCar held_;
    TrafficWhistleStop whistle_;
};

}
