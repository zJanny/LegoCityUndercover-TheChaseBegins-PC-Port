#include "runtime/traffic_route_setup.hpp"

#include <cstdio>
#include <cstring>
#include <stdexcept>

namespace chase::runtime {
namespace {

constexpr const char* kJunctionClass = "props/traffic_junction";

std::string pathName(const assets::LevelParam& param) {
    if (param.blocks.empty() || param.blocks[0].empty()) return {};
    const auto& block = param.blocks[0];
    std::size_t length = 0;
    while (length < block.size() && block[length] != 0) ++length;
    return std::string(reinterpret_cast<const char*>(block.data()), length);
}

void addPaths(std::vector<std::uint32_t>& list, const assets::LevelObject& object, const char* format) {
    for (std::size_t slot = 1; slot <= kTrafficJunctionPathSlots; ++slot) {
        char name[16];
        std::snprintf(name, sizeof name, format, static_cast<unsigned>(slot));
        const std::uint32_t hash = assets::nameHash(name);
        for (const assets::LevelParam& param : object.params)
            if (param.hash == hash) list.push_back(assets::nameHash(pathName(param)));
    }
}

}

std::vector<TrafficJunction> trafficJunctions(const std::vector<const assets::Level*>& levels) {
    std::vector<TrafficJunction> junctions;
    const std::uint32_t positionHash = assets::nameHash("initialposition");
    for (const assets::Level* level : levels)
        for (const assets::LevelObject& object : level->objects) {
            if (object.className != kJunctionClass) continue;
            TrafficJunction junction;
            bool placed = false;
            for (const assets::LevelParam& param : object.params)
                if (param.hash == positionHash && param.type == 2 && !param.blocks.empty() && param.blocks[0].size() >= 12) {
                    std::memcpy(junction.position.data(), param.blocks[0].data(), 12);
                    placed = true;
                }
            if (!placed) throw std::runtime_error("traffic junction " + object.instance + " has no initialposition");
            addPaths(junction.inputs, object, "InputPath%02u");
            addPaths(junction.outputs, object, "OutputPath%02u");
            junctions.push_back(std::move(junction));
        }
    return junctions;
}

TrafficRoutes setUpTrafficRoutes(TrafficRouteTable& table, const std::vector<const assets::Level*>& levels) {
    TrafficRoutes routes;
    routes.junctions = trafficJunctions(levels);
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
