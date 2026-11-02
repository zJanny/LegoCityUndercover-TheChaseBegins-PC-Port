#pragma once
// The data side of the traffic routes (docs/re/traffic-ai-driver.md): the junction objects of the loaded levels (global list 0x515f40, class
// props/traffic_junction, set-up 0x1ca5a4) as `TrafficJunction`, the route list and the route -> junction pairs of the room set-up
// (0x2df574, 0x2dfc84), and the route index of a pregen point. No SDL dependency.
#include "assets/assets.hpp"
#include "runtime/traffic_route.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace chase::runtime {

constexpr std::size_t kTrafficJunctionPathSlots = 8;

// The junctions of the levels in object order: initialposition, InputPath01..08 and OutputPath01..08 (name hashes of the named paths, empty
// slots skipped). STAND-IN TRAFFIC-S9: the lane data of 0x1ca5a4 (lanes, conflicts, listed lanes) stays empty, so no junction ever stops a car.
// Throws for a junction without a 12-byte initialposition.
std::vector<TrafficJunction> trafficJunctions(const std::vector<const assets::Level*>& levels);

struct TrafficRoutes {
    std::vector<TrafficJunction> junctions;
    TrafficRouteSignals signals;
    std::size_t junctionsInReach = 0;
};

// The route part of the room set-up: 0x2df574 (route list of the junctions within 9000) into `table`, then the pairs 0x2dfc84. The caller loads
// the pregen data when `junctionsInReach` is not 0 (bit 0 of the table) and replays the pending list.
TrafficRoutes setUpTrafficRoutes(TrafficRouteTable& table, const std::vector<const assets::Level*>& levels);

// The route index of a pregen point: the +0x24 word of its path record that 0x2df574 wrote, the index in the route list. A path outside the list
// has none (STAND-IN TRAFFIC-S3: the original reads the stored word of the record).
std::optional<std::uint32_t> trafficPointRoute(const TrafficRouteTable& table, std::uint32_t pathHash);

// EnableTrafficPath (handler 0x27bc30): the path is named by its path hash.
void runEnableTrafficPath(TrafficRouteTable& table, const std::string& name, bool on);

}
