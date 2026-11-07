#include "runtime/traffic_route_setup.hpp"

#include "runtime/traffic_junction_fill.hpp"

#include <utility>

namespace chase::runtime {

std::vector<TrafficJunction> trafficJunctions(const std::vector<const assets::Level*>& levels) {
    std::vector<TrafficJunction> junctions;
    for (TrafficJunctionFill& fill : trafficJunctionFills(levels)) junctions.push_back(std::move(fill.record.layout));
    return junctions;
}

TrafficRoutes setUpTrafficRoutes(TrafficRouteTable& table, const std::vector<const assets::Level*>& levels) {
    return setUpTrafficJunctionRoutes(table, trafficJunctions(levels));
}

TrafficRoutes setUpTrafficJunctionRoutes(TrafficRouteTable& table, std::vector<TrafficJunction> junctions) {
    TrafficRoutes routes;
    routes.junctions = std::move(junctions);
    routes.junctionsInReach = collectTrafficRoutes(table, routes.junctions);
    routes.signals = linkTrafficRouteSignals(table, routes.junctions);
    return routes;
}

std::optional<std::uint32_t> trafficPointRoute(const TrafficRouteTable& table, std::uint32_t pathHash) {
    const std::optional<std::size_t> index = trafficRouteIndex(table, pathHash);
    if (!index) return std::nullopt;
    return static_cast<std::uint32_t>(*index);
}

void runEnableTrafficPath(TrafficRouteTable& table, const std::string& name, bool on) {
    enableTrafficPath(table, assets::nameHash(name), on);
}

}
