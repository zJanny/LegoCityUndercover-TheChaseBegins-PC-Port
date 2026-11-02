#pragma once
// The kinematic movement of a traffic car in AI driver states 2 and 3 (docs/re/traffic-ai-driver.md): the route mover 0x2613ac that the
// post step 0x1bc79c runs, the stop test 0x2ca504, the junction choice 0x2cb1e8 and the junction queue 0x22a080. Pure steps on plain
// inputs: the junctions are those of traffic_route.hpp plus the record parts these functions read, a driver's signal object (+0) is the
// junction's index + 1 (0 = none). No SDL, no entities.
#include "runtime/ai_driver_input.hpp"
#include "runtime/game_random.hpp"
#include "runtime/traffic_route.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace chase::runtime {

constexpr std::uint8_t kTrafficMoverSnapAfter = 5;             // 0x2613d4: +0xae above 5 runs 0x20ae2c and restarts at 0
constexpr float kTrafficMoverTurnSpeed = 1.0f;                 // 0x261690: the car turns only above this speed
constexpr float kTrafficMoverFastSpeed = 20.0f;                // 0x261698
constexpr float kTrafficMoverTurnRate = 4.0f;                  // 0x2616a0: turn blend per second up to 20
constexpr float kTrafficMoverFastTurnRate = 6.0f;              // 0x26169c: above 20
constexpr std::uint16_t kTrafficMoverShortRoute = 4;           // 0x261510: route length (hword +10) of the short step
constexpr float kTrafficMoverShortRouteStep = 0.15f;           // 0x2616a4 (0x3e19999a)
constexpr float kTrafficMoverRouteStep = 0.4f;                 // 0x2616a8 (0x3ecccccd)
constexpr std::int32_t kTrafficMoverSpeedSnapBits = 0x3a83126f;  // 0x2616b0: |target - speed| at or below 0.001 takes the target
constexpr float kTrafficMoverSpeedBlend = 0.18f;               // 0x2616b4 (0x3e3851ec)
constexpr float kTrafficJunctionChoiceMargin = 0.5f;           // 0x2cb420: no choice before length - 0.5
constexpr unsigned kTrafficJunctionNearestSamples = 10;        // 0x2cb378
constexpr float kTrafficJunctionNearestScale = 1.0f;           // 0x2cb424
constexpr std::uint32_t kTrafficJunctionChoiceSpread = 8;      // 0x2cb2a8: random % (count x 8), then >> 3
constexpr std::size_t kTrafficJunctionChoices = 8;             // 32 bytes per input at +0x310
constexpr std::size_t kTrafficJunctionLightPairs = 8;          // +0x2c0 .. +0x300
constexpr std::size_t kTrafficJunctionQueueSize = 52;          // 8-byte entries from +0x280 up to the count byte +0x424
constexpr std::size_t kTrafficLightRoutes = 4;                 // light record +0xf0 .. +0x100

// A TrafficLight%02u object as 0x2786a8 reads it: the routes (record +0xf0, count +0x100) it controls.
struct TrafficLight {
    std::vector<std::uint32_t> routes;
};

// The ends of a 96-byte lane at *(+0x410): lane +0 points to an input entry, lane +4 to an output entry (as indices here).
struct TrafficJunctionLaneEnds {
    std::size_t input = 0;
    std::size_t output = 0;
};

// One props/traffic_junction record (+0x7c of the object) as the four functions read it. `layout` holds the paths and lanes
// (traffic_route.hpp); its listedLanes are the lane words (+0x284) of the queue, `queuedCars` the car words (+0x280) of the same entries.
// The set-up 0x1ca5a4 and the light update 0x1d8924 that write the other fields are not read (TRAFFIC-4): the caller fills them.
struct TrafficJunctionRecord {
    TrafficJunction layout;
    std::vector<std::vector<std::size_t>> choices;                     // per input i: count byte +0x418 + i, output indices +0x310 + 32 i
    std::vector<TrafficJunctionLaneEnds> laneEnds;                     // per lane of layout.lanes
    std::vector<std::array<std::optional<TrafficLight>, 2>> lightPairs;  // the light pointers at +0x2c0 (0 = none), count +0x304
    std::uint32_t passPair = 0;                                        // +0x300: the pair whose lights let cars pass
    bool passOpen = false;                                             // byte +0x308: 0 stops the cars of every pair
    std::vector<std::uint32_t> queuedCars;                             // word +0x280 of each queue entry, count +0x424
};

// The junctions, the route → junction pairs (0x20ff40) and the manager's route bits (0x2b9f84) that the junction choice reads.
struct TrafficJunctionWorld {
    std::vector<TrafficJunctionRecord> junctions;
    TrafficRouteSignals signals;
    TrafficRouteTable routes;
};

// The driver's signal object (+0) for a junction index of trafficSignalForRoute (0x20ff40): index + 1, 0 = none.
std::uint32_t trafficSignalId(std::optional<std::size_t> junction);
// The junction of a signal object, none for 0 or an unknown index.
const TrafficJunctionRecord* trafficSignalJunction(const TrafficJunctionWorld& world, std::uint32_t signal);
TrafficJunctionRecord* trafficSignalJunction(TrafficJunctionWorld& world, std::uint32_t signal);

// 0x2786a8(light, route): the route is one of the light's routes.
bool trafficLightControls(const TrafficLight& light, std::uint32_t route);

// 0x2ca504(entity), the stop test of states 2 and 3 in the post step 0x1bc79c. False without a signal object or without light pairs. The car's
// route (driver +4) is searched among the junction's inputs; a car on one of its lanes is never stopped. Otherwise, for each light of each
// pair that controls the matched input route (0 when the route is no input), the car stops when the pair is not +0x300 or byte +0x308 is 0.
bool trafficStopAtLights(const AiDriverRecord& driver, const TrafficJunctionWorld& world);

// 0x22a080(junction, car): only with driver +0xaf bit 3. The queue entry of the car is searched from the last one; the count byte is
// decremented in any case and the last entry moves into the found one (so a car that is not found drops the last entry). Bit 3 is cleared.
// STAND-IN TRAFFIC-S11: an empty queue throws (the original wraps the count byte to 255).
void leaveTrafficJunctionQueue(TrafficJunctionRecord& junction, AiDriverRecord& driver, std::uint32_t car);

struct TrafficJunctionChoiceFrame {
    std::uint32_t car = 0;            // the entity, stored in the queue entry
    AiVec3 nodePosition{};            // translation of the node matrix 0x250810
    float bodyBase = 0.0f;            // body +0x54 (0x20af24 = driver +0x74 - body +0x54)
    const AiRouteFunctions* routes = nullptr;
};

// 0x2cb1e8(entity, &out). Returns false without writing `signal` while length - 0.5 (route hword +10) > driver +0x64. Without a junction
// for the route (0x20ff40): signal = 0, true. Otherwise the input entry equal to the route picks an output: none with count 0, the only one
// with count 1, else random % (count x 8) >> 3 drawn until the output's route bit is set (0x2b9f84). The lane joining that input and
// output becomes the route (+4), the output the next route (+8); +0x64 = the route sample nearest to the node (0x21d5b0, 10 samples, scale
// 1.0); +0xc = the route point there with y + 0x20af24; +0 = the junction, +0xaf bit 3 set, (car, lane) appended to the queue; signal = the
// junction. STAND-IN TRAFFIC-S11: no lane for the pair, no enabled output among the choices (the original loops for ever) and a full
// queue throw.
bool chooseTrafficJunction(AiDriverRecord& driver, const TrafficJunctionChoiceFrame& frame, TrafficJunctionWorld& world, GameRandom& random,
                           std::uint32_t& signal);

struct TrafficMoverFrame {
    std::uint32_t car = 0;
    AiMatrix node{};                  // 0x2429d0(entity +0x3c): the node's world matrix
    float dt = 0.0f;                  // 0x23602c
    float radius = 0.0f;              // entity +0x5c
    float bodyBase = 0.0f;            // body +0x54
    bool kinematic = false;           // rigid flags (body +0x7c) +0xcc bit 1 (value 2)
    float gearSpeed = 0.0f;           // 0x20362c(body)
    float engineFrom = 0.0f;          // body +0x26c
    float engineTo = 0.0f;            // body +0x264
    const AiRouteFunctions* routes = nullptr;
};

struct TrafficMoverResult {
    AiMatrix node{};                  // 0x24c118(node, matrix)
    std::optional<AiVec3> velocity;   // 0x229920(body, v): rigid +0x13c..+0x144, +0x148 = 0; only for a kinematic body
    float engineBlend = 0.0f;         // body +0x27c (0x2286d4)
};

// 0x2613ac(entity), the route mover of states 2 and 3:
// 1. +0xae += 1; above 5, 0x20ae2c (hook snapTarget) and +0xae = 0.
// 2. Above speed 1.0 the node rows turn toward the look-at matrix 0x22859c(node, target +0xc) by dt x 4 (dt x 6 above 20) per row
//    (0x221e5c), are orthonormalised (0x2361d4) and copied to rows 0..2 of +0x18.
// 3. The node moves dt x +0x68 along its row 2.
// 4. Within entity +0x5c of the target: past the route length the target moves on by row 2; otherwise +0x64 grows by 0.15 (length 4) or
//    0.4, and at the length +0x64 = the length and, with +0x70 > 0, the junction choice 0x2cb1e8 runs from the node position before this
//    move (a junction requests state 3, none calls 0x21d6b8, hook leaveTraffic); then +0xc = the route point at +0x64 with y + 0x20af24.
// 5. The node matrix is set, a kinematic body gets the velocity row 2 x +0x68, body +0x27c = lerp(+0x26c, +0x264, +0x68 / 0x20362c).
// 6. +0x68 takes +0x70 when they differ by at most 0.001, else moves 0.18 of the way.
TrafficMoverResult runTrafficRouteMover(AiDriverRecord& driver, const TrafficMoverFrame& frame, TrafficJunctionWorld& world,
                                        const AiDriverHooks& hooks, GameRandom& random);

}
