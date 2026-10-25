#pragma once
// The routes of the traffic cars (docs/re/traffic.md): the route list of the traffic manager 0x527410 (0x2df574, EnableTrafficPath
// 0x2dece4 and its pending list, replayed by 0x2dedbc), the path follower 0x2bf2ec, the route type function (slot 0 of table 0x3581d4),
// the route → junction pairs (0x2dfc84, 0x20ff40) and the junction test 0x2cdde4. A route is named by the name hash of its level path
// (path record +0); a junction lane by whatever id the caller gives it. No SDL dependency, no vehicle types.
#include "assets/assets.hpp"
#include "assets/pregen_traffic.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace chase::runtime {

using TrafficRouteVec3 = std::array<float, 3>;

constexpr std::size_t kTrafficMaxJunctions = 32;
constexpr float kTrafficJunctionReach = 9000.0f;
constexpr std::size_t kTrafficMaxRoutes = 128;
constexpr std::size_t kTrafficMaxPendingRoutes = 128;
constexpr std::size_t kTrafficMaxRouteSignals = 128;
constexpr float kTrafficRouteSearchStart = 9999.0f;
constexpr std::uint32_t kTrafficRouteAheadBits = 0x3f000000u;

// One props/traffic_junction object (record +0x7c, 0x428 bytes; set-up 0x1ca5a4).
struct TrafficJunction {
    TrafficRouteVec3 position{};                   // node translation (0x250810 +0x30)
    std::vector<std::uint32_t> inputs;             // InputPath01..08: 40-byte entries at +0, count +0x421
    std::vector<std::uint32_t> outputs;            // OutputPath01..08: 40-byte entries at +0x140, count +0x422
    std::vector<std::uint32_t> lanes;              // the route of each 96-byte lane at *(+0x410) (lane +8), count +0x423
    std::vector<std::array<std::uint8_t, 2>> conflicts;  // lane index pairs at *(+0x414), count +0x420
    std::vector<std::uint32_t> listedLanes;        // the lanes named by the 8-byte entries at +0x284 (word 0 = lane), count +0x424
};

// The route state of the manager: bit 0 of +0x390, the route list +0xc4 (count +0x2d4; a route's index is written to its path record
// +0x24), the bit set +0x2c4 and the pending list +0xb9c (count +0xd9c).
struct TrafficRouteTable {
    bool loaded = false;
    std::vector<std::uint32_t> routes;
    std::vector<bool> enabled;
    std::vector<std::uint32_t> pending;
};

// The route part of 0x2df574: the junctions within 9000 of the origin in x and z (0x2c8718, list order, at most 32), then for each
// junction its inputs and then its outputs; a route not yet in the list is appended with its bit set. The list is emptied first (count
// +0x2d4 = 0 before 0x2c8718); the bits of earlier routes are not cleared. Returns the number of junctions taken; with none the list stays
// empty and the caller leaves bit 0 and the pregen data as they were. STAND-IN TRAFFIC-S4: a 129th route throws (the list +0xc4 holds
// 128 words before the bit set +0x2c4; the original does not check).
std::size_t collectTrafficRoutes(TrafficRouteTable& table, const std::vector<TrafficJunction>& junctions);

// Index of the route in the list (the path record +0x24 that 0x2df574 wrote).
std::optional<std::size_t> trafficRouteIndex(const TrafficRouteTable& table, std::uint32_t route);

// The bit test of 0x2bf2ec: index below the count +0x2d4 and its bit set. STAND-IN TRAFFIC-S3: a route missing from the list counts
// as disabled; the original tests the stored +0x24 word of the path record, and every shipped traffic point lies on a junction path.
bool trafficRouteEnabled(const TrafficRouteTable& table, std::uint32_t route);

// EnableTrafficPath (handler 0x27bc30 → 0x2a7004 → 0x2dece4). Loaded: the route's bit is set or cleared, a route missing from the list
// is ignored. Not loaded: enabling does nothing; disabling appends the route to the pending list unless it is there already.
// STAND-IN TRAFFIC-S4: a 129th pending route throws (the original writes over its count +0xd9c).
void enableTrafficPath(TrafficRouteTable& table, std::uint32_t route, bool on);

// The pending part of the room set-up 0x2dedbc (after 0x2df574 and 0x2dfc84): when loaded, each pending route found in the list gets its
// bit cleared; then the pending list is emptied.
void applyPendingTrafficRoutes(TrafficRouteTable& table);

// The pairs of 0x2dfc84: for each route of the list, each junction of the global list (0x2b4218, at most 128) and each entry, an input
// entry equal to the route appends (route, junction) to +0x594 / +0x394 (count +0x794), an output entry to +0x998 / +0x798 (count
// +0xb98). STAND-IN TRAFFIC-S4: a pair beyond 128 throws (the original does not check).
struct TrafficRouteSignal {
    std::uint32_t route = 0;
    std::size_t junction = 0;
};
struct TrafficRouteSignals {
    std::vector<TrafficRouteSignal> inputs;
    std::vector<TrafficRouteSignal> outputs;
};
TrafficRouteSignals linkTrafficRouteSignals(const TrafficRouteTable& table, const std::vector<TrafficJunction>& junctions);

// 0x20ff40: the junction of the first input pair of the route, or none (the driver's +0).
std::optional<std::size_t> trafficSignalForRoute(const TrafficRouteSignals& signals, std::uint32_t route);

// 0x2cdde4(junction, car): 0 (stop) when the car's route (driver +4) is lane k of the junction and a lane paired with k in the conflict
// list is one of the listed lanes; 1 otherwise, also when the route is no lane of the junction.
bool trafficJunctionAllowsCar(const TrafficJunction& junction, std::uint32_t carRoute);

// One point of a pregen traffic cell as 0x2bf2ec reads it: +0 position, +0x18 the path record, +0x20 the path parameter.
struct TrafficRoutePoint {
    TrafficRouteVec3 position{};
    std::uint32_t route = 0;
    float parameter = 0.0f;
};

// The cell 0x1f47f0 descends to under (x, z): the points of the list of findPregenSpawnCell. 0x1f47f0 returns an empty cell where
// 0x1eb490 returns none; for the follower both give no point.
std::vector<TrafficRoutePoint> trafficRouteCell(const assets::PregenSpawns& spawns, float x, float z);

struct TrafficRouteQuery {
    TrafficRouteVec3 position{};
    TrafficRouteVec3 forward{};
};

// The outputs of 0x2bf2ec (r2 position, r3 route, stack word parameter); the caller's fields stay as they were when nothing is found.
struct TrafficRouteHit {
    bool found = false;
    std::uint32_t route = 0;
    TrafficRouteVec3 position{};
    float parameter = 0.0f;
};

// 0x2bf2ec(position, forward, out position, out route, out parameter): of the cell's points whose route is enabled, the nearest one
// (strictly nearer than 9999 and than every earlier pick) whose unit direction from the position (0x257ea0, (0, 0, 1) for a length
// whose bits are at most 0x34000000) has a dot product with forward whose bits, compared as a signed integer, exceed 0x3f000000 (0.5).
TrafficRouteHit findTrafficRoutePoint(const std::vector<TrafficRoutePoint>& cell, const TrafficRouteTable& table, const TrafficRouteQuery& query);

// The route type function: slot 0 of the route's type in table 0x3581d4 (the same entries as 0x358f9c), called as (route +8, out
// position, out tangent, 1, parameter). Type 0 is 0x1c17c0, type 1 is 0x1b33fc (pedestrianLevelPath); every shipped traffic route is type 0.
struct TrafficRouteSample {
    TrafficRouteVec3 position{};
    TrafficRouteVec3 tangent{};
};
TrafficRouteSample sampleTrafficRoute(const assets::LevelPath& path, float parameter);

}
