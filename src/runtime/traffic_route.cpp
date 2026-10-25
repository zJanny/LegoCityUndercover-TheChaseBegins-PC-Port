#include "runtime/traffic_route.hpp"

#include "runtime/pedestrian_walk.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string>

namespace chase::runtime {
namespace {

std::int32_t floatBits(float value) {
    std::int32_t bits = 0;
    std::memcpy(&bits, &value, sizeof bits);
    return bits;
}

float normalizeInPlace(TrafficRouteVec3& v) {
    const float length = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (floatBits(length) > 0x34000000) {
        const float inverse = 1.0f / length;
        v = {v[0] * inverse, v[1] * inverse, v[2] * inverse};
    } else {
        v = {0.0f, 0.0f, 1.0f};
    }
    return length;
}

}

std::size_t collectTrafficRoutes(TrafficRouteTable& table, const std::vector<TrafficJunction>& junctions) {
    std::vector<const TrafficJunction*> taken;
    for (const TrafficJunction& junction : junctions) {
        const float distance = std::sqrt(junction.position[0] * junction.position[0] + junction.position[2] * junction.position[2]);
        if (!(kTrafficJunctionReach > distance)) continue;
        taken.push_back(&junction);
        if (taken.size() == kTrafficMaxJunctions) break;
    }
    table.routes.clear();
    if (taken.empty()) return 0;
    const auto add = [&table](std::uint32_t route) {
        if (std::find(table.routes.begin(), table.routes.end(), route) != table.routes.end()) return;
        if (table.routes.size() == kTrafficMaxRoutes)
            throw std::length_error("traffic routes: more than " + std::to_string(kTrafficMaxRoutes) + " junction paths");
        const std::size_t index = table.routes.size();
        if (table.enabled.size() <= index) table.enabled.resize(index + 1, false);
        table.enabled[index] = true;
        table.routes.push_back(route);
    };
    for (const TrafficJunction* junction : taken) {
        for (std::uint32_t route : junction->inputs) add(route);
        for (std::uint32_t route : junction->outputs) add(route);
    }
    return taken.size();
}

std::optional<std::size_t> trafficRouteIndex(const TrafficRouteTable& table, std::uint32_t route) {
    const auto found = std::find(table.routes.begin(), table.routes.end(), route);
    if (found == table.routes.end()) return std::nullopt;
    return std::size_t(found - table.routes.begin());
}

bool trafficRouteEnabled(const TrafficRouteTable& table, std::uint32_t route) {
    const std::optional<std::size_t> index = trafficRouteIndex(table, route);
    return index && *index < table.enabled.size() && table.enabled[*index];
}

void enableTrafficPath(TrafficRouteTable& table, std::uint32_t route, bool on) {
    if (table.loaded) {
        const std::optional<std::size_t> index = trafficRouteIndex(table, route);
        if (!index) return;
        if (table.enabled.size() <= *index) table.enabled.resize(*index + 1, false);
        table.enabled[*index] = on;
        return;
    }
    if (on) return;
    if (std::find(table.pending.begin(), table.pending.end(), route) != table.pending.end()) return;
    if (table.pending.size() == kTrafficMaxPendingRoutes)
        throw std::length_error("EnableTrafficPath: more than " + std::to_string(kTrafficMaxPendingRoutes) + " pending routes");
    table.pending.push_back(route);
}

void applyPendingTrafficRoutes(TrafficRouteTable& table) {
    if (table.loaded)
        for (std::uint32_t route : table.pending) {
            const std::optional<std::size_t> index = trafficRouteIndex(table, route);
            if (index && *index < table.enabled.size()) table.enabled[*index] = false;
        }
    table.pending.clear();
}

TrafficRouteSignals linkTrafficRouteSignals(const TrafficRouteTable& table, const std::vector<TrafficJunction>& junctions) {
    TrafficRouteSignals signals;
    const std::size_t count = std::min(junctions.size(), kTrafficMaxRouteSignals);
    const auto append = [](std::vector<TrafficRouteSignal>& list, std::uint32_t route, std::size_t junction) {
        if (list.size() == kTrafficMaxRouteSignals)
            throw std::length_error("traffic route signals: more than " + std::to_string(kTrafficMaxRouteSignals) + " pairs");
        list.push_back({route, junction});
    };
    for (std::uint32_t route : table.routes)
        for (std::size_t j = 0; j < count; ++j) {
            for (std::uint32_t input : junctions[j].inputs)
                if (input == route) append(signals.inputs, route, j);
            for (std::uint32_t output : junctions[j].outputs)
                if (output == route) append(signals.outputs, route, j);
        }
    return signals;
}

std::optional<std::size_t> trafficSignalForRoute(const TrafficRouteSignals& signals, std::uint32_t route) {
    for (const TrafficRouteSignal& pair : signals.inputs)
        if (pair.route == route) return pair.junction;
    return std::nullopt;
}

bool trafficJunctionAllowsCar(const TrafficJunction& junction, std::uint32_t carRoute) {
    const auto lane = std::find(junction.lanes.begin(), junction.lanes.end(), carRoute);
    if (lane == junction.lanes.end()) return true;
    const std::uint32_t k = std::uint32_t(lane - junction.lanes.begin());
    for (const std::array<std::uint8_t, 2>& pair : junction.conflicts) {
        std::uint32_t other = 0;
        if (pair[0] == k) {
            other = pair[1];
        } else if (pair[1] == k) {
            other = pair[0];
        } else {
            continue;
        }
        for (std::uint32_t listed : junction.listedLanes)
            if (listed == other) return false;
    }
    return true;
}

std::vector<TrafficRoutePoint> trafficRouteCell(const assets::PregenSpawns& spawns, float x, float z) {
    std::vector<TrafficRoutePoint> cell;
    const std::int32_t index = assets::findPregenSpawnCell(spawns, x, z);
    if (index < 0) return cell;
    for (std::uint16_t point : spawns.cells[std::size_t(index)].points) {
        const assets::PregenSpawnPoint& source = spawns.points.at(point);
        cell.push_back({source.position, source.pathHash, source.pathPosition});
    }
    return cell;
}

TrafficRouteHit findTrafficRoutePoint(const std::vector<TrafficRoutePoint>& cell, const TrafficRouteTable& table, const TrafficRouteQuery& query) {
    TrafficRouteHit hit;
    float best = kTrafficRouteSearchStart;
    for (const TrafficRoutePoint& point : cell) {
        if (!trafficRouteEnabled(table, point.route)) continue;
        TrafficRouteVec3 direction{point.position[0] - query.position[0], point.position[1] - query.position[1],
                                   point.position[2] - query.position[2]};
        const float distance = normalizeInPlace(direction);
        if (!(best > distance)) continue;
        const float ahead = direction[0] * query.forward[0] + direction[1] * query.forward[1] + direction[2] * query.forward[2];
        if (floatBits(ahead) <= std::int32_t(kTrafficRouteAheadBits)) continue;
        best = distance;
        hit.found = true;
        hit.route = point.route;
        hit.position = point.position;
        hit.parameter = point.parameter;
    }
    return hit;
}

TrafficRouteSample sampleTrafficRoute(const assets::LevelPath& path, float parameter) {
    TrafficRouteSample sample;
    pedestrianLevelPath(path).sample(parameter, sample.position, sample.tangent);
    return sample;
}

}
