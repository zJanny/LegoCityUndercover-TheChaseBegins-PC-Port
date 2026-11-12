#pragma once
// The calls of the traffic AI driver in states 4 to 8 (docs/re/traffic-driver-states.md): the hooks of ai_driver_input.hpp that the states
// 4 to 8 reach, connected to the ported steering 0x27872c, contact step 0x29cc38, chase markers, the state 6 entry (0x229be4, 0x23cee8,
// 0x22a124), the destroy message of state 7 (0x244fc8) and the throw of state 5 (0x254250, 0x1bc660 with +0xaf bit 1, set by
// VehicleSetThrowObjects 0x2ad940); the input copy 0x23019c of the driver's record inputs; the request of a driver state. No SDL dependency.
#include "assets/assets.hpp"
#include "runtime/ai_driver_input.hpp"
#include "runtime/game_random.hpp"
#include "runtime/traffic_driver_calls.hpp"
#include "runtime/traffic_driver_steering.hpp"
#include "runtime/vehicle_damage.hpp"
#include "runtime/vehicle_drivetrain.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

class TrafficDrivers;

// Driver +0x79 = state: the state change 0x1cb148 enters it at the car's next update. The writers in the original are the contact message
// 0x80000002 (4), the damage message 0 of a chased car (6), VehicleSetChased (5), the release 0x22a1f4 (0) and the driver itself.
void requestTrafficDriverState(TrafficDrivers& drivers, std::size_t car, std::uint8_t state);

// VehicleSetThrowObjects 0x2ad940(car, flag): nothing without a driver record; +0xaf bit 1 follows the word of the flag (any bits but 0, also
// -0.0). The handler returns 1.
void setVehicleThrowObjects(AiDriverRecord* driver, float flag);

constexpr float kVehicleThrowSpeedScale = 1.0f;        // s17 of 0x1cb148 (0x1cb4e4) passed to 0x1bc660
constexpr float kVehicleThrowTangentLimit = 1.0f;      // 0x28ac40: the launch is at most 45 degrees up while the target is not higher than far
constexpr float kVehicleThrowShortest = 1.1920929e-7f;  // 0x23ba38: a horizontal length whose bits are at most 0x34000000 gives (0, 1)

// 0x254250(local, node): the local point of the hook rearEffect ((side, 0.5, -2.0), side = (0x2345dc() - 0.5) x 2) by the node matrix with
// its translation; 0x1cb148 hands it to 0x1bc660 as the target.
AiVec3 vehicleThrowTarget(const AiVec3& local, const AiMatrix& node);

// 0x28abd4(from, to, out, s0 speed, s1 gravity): the horizontal direction (0x23ba14) times the speed, and an up speed that reaches `to`
// under the gravity: speed x (dy / h + gravity x h / (2 speed^2)). When that tangent is above 1.0 and dy is below h, the speed is
// sqrt(gravity h^2 / (2 (h - dy))) and the tangent 1.0.
AiVec3 vehicleThrowLaunch(const AiVec3& from, const AiVec3& to, float speed, float gravity);

// One object of the throw object list 0x515fc0 (class `GOVehThrowObject`, kind 208 `projectiles/vehthrow_object`), as 0x1bc660 reads it.
struct VehicleThrowObject {
    std::uint32_t entity = 0;        // identity
    bool disabled = true;            // entity hword +0x10 bit 0: free to throw (0x242cf0 enables it)
    bool frozen = false;             // entity word +0xc bit 0x10
    std::uint8_t timer = 0;          // record +0x3b
    std::uint8_t maxTime = 0;        // record +0x3c bits 0..6 (the create 0x1caec4: `MaxTime`, below 127)
    float speed = 3.0f;              // record +0x30 (`speed`, default 3.0)
    float gravity = 0.01f;           // record +0x34 (`gravity`, default 0.01)
    AiVec3 velocity{};               // record +0x24
    std::uint32_t owner = 0;         // record +0x20: the car that threw it
    AiVec3 centre{};                 // node +0xa0: the point launched from, in the node
    AiMatrix node{};
    std::uint32_t parent = 0;        // node +4
};

struct VehicleThrowResult {
    std::optional<std::size_t> thrown;  // the first disabled object of the list
    bool relinked = false;              // node +4 differed from the car's: 0x245048, 0x245008(car node +4, node), 0x242b08(object)
};

// 0x1bc660(target, car, node matrix, scale): the first disabled object of the list is enabled (0x242cf0), its +0xc bit 0x10 cleared, its
// timer set to its MaxTime, its node set to the car's matrix (0x24c118); it is launched from its centre (0x250da8) at the target with
// speed x scale and its gravity (0x28abd4) and linked to the car's parent node; its owner is the car.
VehicleThrowResult throwVehicleObject(std::vector<VehicleThrowObject>& objects, const AiVec3& target, std::uint32_t car, std::uint32_t carParent,
                                      const AiMatrix& node, float scale);

// 0x23019c for the four record inputs (+0x288 steer, +0x28c accelerate, +0x290 brake, +0x294 hand brake): states 3, 4 and 11 set the hand
// brake to 1.0; a forced stop (+0x310) sets hand brake 1.0, accelerate 0.0 and brake 1.0, and brake 0.0 again for gear 0 (body +0x294) or
// a body speed (0x229948) of 0.1 or less. The four values go to the body (+0x1bc..+0x1c8) and the record inputs are cleared.
BodyInputs copyVehicleRecordInputs(AiDriverVehicle& vehicle, std::uint16_t vehicleState, int gear, float bodySpeed);

constexpr std::uint32_t kAiDestroyHearts = 1000;  // 0x1cb5b0: the damage message of state 7

// The message 0 the car sends itself on entering state 7 (0x1cb5b0..0x1cb5dc): word +0xc = 1000, every other field 0.
DamageMessage trafficDestroyMessage();

struct TrafficStoppingEntry {
    std::string event;            // 0x23cee8(*0x375fe0, car, car, 0, 1): VehicleChaseEnded with the car as owner
    bool globalEvent = false;     // the second 0x23cee8 at the object 0x23c188(*(*0x376c78 + 0x28)) with 255, only when *0x376c78 is set
    int smokeEffect = 0;          // 0x22a124(car, 0x11f)
};

// Entering state 6 after 0x22a270(car, 0): 0x229be4(car, 1) sets the forced stop +0x310, then the events and the smoke.
TrafficStoppingEntry enterTrafficStopping(std::uint8_t& forcedStop, bool globalObject);

enum class TrafficStateEventKind {
    Placed,    // 0x27872c placed the body at `matrix` (0x23091c)
    Stopping,  // state 6 entered: `stopping`
    Destroy,   // state 7 entered: `message`
    Throw,     // state 5 every 64 updates with +0xaf bit 1: `target`
};

struct TrafficStateEvent {
    TrafficStateEventKind kind = TrafficStateEventKind::Placed;
    AiMatrix matrix{};
    TrafficStoppingEntry stopping;
    DamageMessage message;
    AiVec3 target{};
};

// What the hooks of states 4 to 8 read and write of one car. The pointers must outlive the hooks.
struct TrafficStateContext {
    AiDriverRecord* driver = nullptr;
    AiDriverVehicle* vehicle = nullptr;
    TrafficSteeringState* steering = nullptr;
    AiMatrix* node = nullptr;             // the car's node matrix, also written by a placement
    bool* moved = nullptr;                // set when the node was written
    float gearSpeed = 0.0f;               // 0x20362c(body)
    float radius = 0.0f;                  // entity +0x5c
    bool globalObject = false;            // *0x376c78 set
    std::function<void()> copyInputs;     // 0x23019c(entity)
    std::function<void()> normalGravity;  // 0x242518(body, *0x376c40)
    std::function<void()> wakeBody;       // 0x2308e4(rigid, 1)
    std::function<void(bool on)> chaseMarkers;  // 0x230dbc, 0x230e60 (setTrafficChaseMarkers)
    TrafficContactHooks contacts;         // the gather and slide of 0x29cc38
    std::vector<TrafficStateEvent>* events = nullptr;
};

// Sets the hooks steerToward, copyInputs, steerStep, normalGravity, wakeBody, setListed, enterStopping, destroy and rearEffect of `hooks`
// for the car of `context`. steerToward runs 0x27872c and writes its inputs to the record (+0x288, and +0x28c or +0x290), each placement
// sets the node; rearEffect turns the local point by the node into a Throw event.
void connectTrafficDriverStates(AiDriverHooks& hooks, const TrafficStateContext& context);

// The parts of a script car (a driver record of VehicleSetAI or VehicleSetChased on a vehicle with a physics body) that its driver's update
// writes besides the record. The pointers must outlive the call.
struct ScriptCarDriver {
    AiDriverRecord* driver = nullptr;
    AiDriverVehicle* vehicle = nullptr;
    TrafficSteeringState* steering = nullptr;
    TrafficBrakeLights* lights = nullptr;
    bool* kinematic = nullptr;        // 0x22a270(entity, on)
    BodyInputs* bodyInputs = nullptr;  // body +0x1bc..+0x1c8, written by the input copy 0x23019c
};

struct ScriptCarFrame {
    AiMatrix node{};                  // the vehicle's node matrix
    AiVec3 halfExtents{};             // entity +0x6c..+0x74
    float radius = 0.0f;              // entity +0x5c
    float dt = 0.0f;                  // 0x23602c
    float gearSpeed = 0.0f;           // 0x20362c(body)
    float bodySpeed = 0.0f;           // 0x229948(body)
    std::uint16_t vehicleState = 0;   // vehicle record hword +2
    int gear = 1;                     // body +0x294
    bool playerInVehicle = false;     // 0x24255c(*0x3772b8) != 0
    bool globalObject = false;        // *0x376c78 set
    const AiRouteFunctions* routes = nullptr;
    std::function<void()> snapTarget;            // 0x20ae2c(entity)
    std::function<void(bool on)> chaseMarkers;   // setTrafficChaseMarkers for the car
};

struct ScriptCarStep {
    std::optional<AiMatrix> node;              // 0x24c118 or a placement of 0x27872c: the vehicle is put there
    std::vector<TrafficStateEvent> events;
};

// The route type functions over the paths of `level` (a script car's route, driver +4, is the name hash of a level path): evaluate is
// sampleTrafficRoute, ahead 0x29fb6c, nearest 0x21d5b0, the length the point count (hword +10). STAND-IN TRAFFIC-S32: the signal (0x20ff40)
// is 0 and the path follower 0x2bf2ec finds nothing. A route that is no path of the level throws std::invalid_argument. `level` must
// outlive the functions.
AiRouteFunctions levelPathRouteFunctions(const assets::Level& level);

// The vehicle update 0x1571dc of a car whose driver has +0xaf bit 0: the input provider 0x26d934 and the state change 0x1cb148, with the
// hooks of states 4 to 8 connected. The junction choice 0x2cb1e8 finds no junction for a route with signal 0 (out = 0, true), and the
// leave call 0x21d6b8 does nothing for a car that is not in the traffic manager's table. STAND-IN TRAFFIC-S32: the contact gather lists
// nothing, the whistle stop 0x2c87a8 is false, the node counts as hidden (TRAFFIC-S35); normalGravity and wakeBody leave the physics body
// alone.
ScriptCarStep runScriptCarDriver(const ScriptCarDriver& car, const ScriptCarFrame& frame, GameRandom& random);
// The post step 0x16ce9c of that car: 0x1bc79c instead of the input copy 0x23019c.
ScriptCarStep runScriptCarPostStep(const ScriptCarDriver& car, const ScriptCarFrame& frame, GameRandom& random);

}
