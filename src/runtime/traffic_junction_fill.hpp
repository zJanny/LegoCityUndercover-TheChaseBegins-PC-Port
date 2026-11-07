#pragma once
// The props/traffic_junction class (docs/re/traffic-junction.md): its set-up 0x1ca5a4 (input and output ends, the sides of 0x159ab8, the
// choice table +0x418 / +0x310, the 4-point lanes at *(+0x410) with their Hermite curves 0x20b0c0, the conflict pairs *(+0x414) from the
// segment test 0x20af64, the TrafficLight%02u / SlaveLight pairs +0x2c0 with the lights' Path01..04 words +0xf0 of the light set-up
// 0x19ff70) and its update 0x1d8924 (the light cycle +0x300 / +0x308 / +0x30c). Pure steps on level data, no SDL, no entities.
#include "assets/assets.hpp"
#include "runtime/traffic_route_mover.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace chase::runtime {

constexpr std::uint32_t kTrafficJunctionTurnBackBits = 0xbf59999au;  // 0x1ca9f4: an output whose dot bits (unsigned) reach -0.85 is no choice
constexpr std::int32_t kTrafficJunctionStraightBits = 0x3f666666;     // 0x1caeb4, 0x159b94: dot bits (signed) above 0.9: straight on / parallel
constexpr std::uint16_t kTrafficLanePoints = 4;                       // 0x1cab7c: lane path hword +10
constexpr float kTrafficLaneFirstThird = 0.333333343f;                // 0x1caeb8 (0x3eaaaaab)
constexpr float kTrafficLaneSecondThird = 0.666666687f;               // 0x1caebc (0x3f2aaaab)
constexpr float kTrafficLaneCrossDistance = 0.1f;                     // 0x1caec0: squared, the x/z gap of crossing lane segments
constexpr std::size_t kTrafficJunctionConflictSlots = 64;             // the 8-byte pairs at sp+8 .. sp+0x208 of 0x1ca5a4
constexpr std::size_t kTrafficJunctionLightSlots = 8;                 // TrafficLight01..08
constexpr std::size_t kTrafficLightPathSlots = 4;                     // Path01..04 of the light set-up 0x19ff70
constexpr std::uint8_t kTrafficLightRed = 0;                          // light record +0x10c "red"
constexpr std::uint8_t kTrafficLightYellow = 1;                       // +0x108 "yellow"
constexpr std::uint8_t kTrafficLightGreen = 2;                        // +0x104 "green"
constexpr float kTrafficLightRedTime = 3.0f;                          // 0x374594, written by the constructor 0x34cdf0
constexpr float kTrafficLightYellowTime = 2.0f;                       // 0x374598
constexpr float kTrafficLightGreenTime = 6.0f;                        // 0x37459c

// STAND-IN TRAFFIC-S13: the port's route id of a lane; the original names a lane by the address of its embedded path record (lane +8,
// name word 0).
constexpr std::uint32_t kTrafficLaneIdBase = 0xf1a00000u;

// One 40-byte input (record +0) or output (record +0x140) entry of the set-up.
struct TrafficJunctionEnd {
    std::uint32_t route = 0;                // +0: the path record
    TrafficRouteVec3 point{};               // +4: last point of an input, first point of an output
    TrafficRouteVec3 inner{};               // +0x10: the point before it (input) or after it (output)
    std::array<float, 2> direction{};       // +0x1c: x and z of the unit travel direction (0x257ea0) at the point
    std::uint8_t side = 0;                  // +0x24: 0, or 1 / 2 for two parallel ends (0x159ab8)
};

// The ends of a path as 0x1ca5a4 builds them. Throws for a path of fewer than two points (STAND-IN TRAFFIC-S13).
TrafficJunctionEnd trafficJunctionInputEnd(const assets::LevelPath& path);
TrafficJunctionEnd trafficJunctionOutputEnd(const assets::LevelPath& path);

// 0x159ab8(entries, count): for each pair a < b whose direction dot (bits, signed) is not below 0.9, the side of a against the
// perpendicular (-b.z, b.x) of b: below 0 gives a = 1, b = 2, else a = 2, b = 1. A later pair overwrites an earlier one.
void markTrafficJunctionSides(std::vector<TrafficJunctionEnd>& ends);

// The choices of one input (the loop at 0x1ca908): with one input and one output just that output; otherwise every output, in order,
// whose direction dot with the input stays above -0.85 and that passes the side rule (both sides set: equal; else straight on, an input
// without side, or the turn whose cross product sign matches the input's side: above 0 for side 2, else side 1).
std::vector<std::size_t> trafficJunctionChoices(const TrafficJunctionEnd& input, const std::vector<TrafficJunctionEnd>& outputs,
                                                std::size_t inputCount);

// The lane path from an input end to an output end (0x1cab68): four points, the two ends and the Hermite points 0x20b0c0 at 1/3 and 2/3
// with tangents (direction x, 0, direction z) scaled by the x/z distance of the ends. Type 0, open, like the zero-filled record.
assets::LevelPath trafficJunctionLanePath(const TrafficJunctionEnd& input, const TrafficJunctionEnd& output, std::uint32_t id);

// 0x20af64(a0, a1, b0, b1, ...): the closest points of two segments with both parameters within 0..1 (false for parallel segments).
struct TrafficSegmentClosest {
    bool found = false;
    TrafficRouteVec3 onA{};
    TrafficRouteVec3 onB{};
};
TrafficSegmentClosest trafficSegmentClosest(const TrafficRouteVec3& a0, const TrafficRouteVec3& a1, const TrafficRouteVec3& b0,
                                            const TrafficRouteVec3& b1);

// The conflict test of two lanes (0x1cad30): segment i of a and segment i of b (the same index; 0x1cad58 takes both from a's counter)
// have closest points whose x/z squared distance is at most 0.1 x 0.1. False when either has fewer than 2 points. STAND-IN TRAFFIC-S13:
// b with fewer points than a throws (the original reads past b's points; every lane has 4).
bool trafficLanesCross(const assets::LevelPath& a, const assets::LevelPath& b);

// What 0x1ca5a4 reads of a junction object: its input and output paths and its light pairs (each TrafficLight%02u found, with the
// SlaveLight of that light), each light given by the routes of its Path01..04.
struct TrafficJunctionSource {
    TrafficRouteVec3 position{};
    std::vector<const assets::LevelPath*> inputs;
    std::vector<const assets::LevelPath*> outputs;
    std::vector<std::array<std::optional<TrafficLight>, 2>> lights;
};

struct TrafficJunctionFill {
    TrafficJunctionRecord record;
    std::vector<TrafficJunctionEnd> inputEnds;
    std::vector<TrafficJunctionEnd> outputEnds;
    std::vector<assets::LevelPath> lanePaths;  // per lane of record.layout.lanes, nameHash = its id
};

// 0x1ca5a4 on one junction. Lanes get the ids firstLaneId, firstLaneId + 1, ... The light cycle starts at pair 0, green, timer 6.
// STAND-IN TRAFFIC-S13: more than 64 conflict pairs throw (the original writes over its stack).
TrafficJunctionFill fillTrafficJunction(const TrafficJunctionSource& source, std::uint32_t firstLaneId);

// The junction objects of the levels in object order, their paths resolved among the levels' paths by name hash (an empty name or one
// without a level path is skipped: 0x22a3b0 returns 0) and their lights among the levels' objects by object reference (none: no pair).
// STAND-IN TRAFFIC-S13: paths and objects are searched in all given levels (the original searches the object's scene), a path name is
// hashed whole (0x22a3b0 hashes the part after its last `.`, 0x23895c), and a lane id equal to a level path's name hash throws.
std::vector<TrafficJunctionFill> trafficJunctionFills(const std::vector<const assets::Level*>& levels);

// 0x20b19c(light, phase, previous) for each light of `pair`: the light shows the part of `phase` and hides the part of `previous`.
struct TrafficLightChange {
    std::size_t pair = 0;
    std::uint8_t phase = 0;
    std::uint8_t previous = 0;
};

// 0x1d8924(object, dt), only with more than one light pair: the timer +0x30c counts down to 0; at 0 green turns yellow (2 s), yellow
// turns red (3 s), and red waits until the junction's queue (+0x424) is empty, then the next pair turns green (6 s).
std::optional<TrafficLightChange> runTrafficJunctionLights(TrafficJunctionRecord& junction, float dt);

}
