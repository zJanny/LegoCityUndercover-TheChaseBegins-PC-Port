#include "runtime/traffic_junction_update.hpp"

#include <stdexcept>
#include <string>
#include <utility>

namespace chase::runtime {
namespace {

const assets::LevelPath* levelPath(const std::vector<const assets::Level*>& levels, std::uint32_t route) {
    for (const assets::Level* level : levels)
        for (const assets::LevelPath& path : level->paths)
            if (path.nameHash == route) return &path;
    return nullptr;
}

const assets::LevelPath& needPath(const TrafficRoutePaths& paths, std::uint32_t route) {
    const assets::LevelPath* path = paths.find(route);
    if (!path) throw std::out_of_range("traffic route " + std::to_string(route) + " has no path");
    return *path;
}

}

void TrafficRoutePaths::add(const assets::LevelPath& path) {
    if (index_.count(path.nameHash)) return;
    index_.emplace(path.nameHash, paths_.size());
    paths_.push_back(path);
}

const assets::LevelPath* TrafficRoutePaths::find(std::uint32_t route) const {
    const auto found = index_.find(route);
    return found == index_.end() ? nullptr : &paths_[found->second];
}

std::vector<TrafficJunction> trafficJunctionLayouts(const std::vector<TrafficJunctionFill>& fills) {
    std::vector<TrafficJunction> layouts;
    for (const TrafficJunctionFill& fill : fills) layouts.push_back(fill.record.layout);
    return layouts;
}

TrafficJunctions keepTrafficJunctions(std::vector<TrafficJunctionFill> fills, const std::vector<const assets::Level*>& levels,
                                      const TrafficRouteSignals& signals) {
    TrafficJunctions kept;
    kept.world.signals = signals;
    for (TrafficJunctionFill& fill : fills) {
        for (const std::vector<std::uint32_t>* routes : {&fill.record.layout.inputs, &fill.record.layout.outputs})
            for (const std::uint32_t route : *routes)
                if (const assets::LevelPath* path = levelPath(levels, route)) kept.paths.add(*path);
        for (const assets::LevelPath& lane : fill.lanePaths) kept.paths.add(lane);
        kept.world.junctions.push_back(std::move(fill.record));
    }
    return kept;
}

std::vector<TrafficJunctionLightEvent> runTrafficJunctionUpdates(TrafficJunctionWorld& world, float dt) {
    std::vector<TrafficJunctionLightEvent> events;
    for (std::size_t i = 0; i < world.junctions.size(); ++i)
        if (const std::optional<TrafficLightChange> change = runTrafficJunctionLights(world.junctions[i], dt)) events.push_back({i, *change});
    return events;
}

AiRouteFunctions trafficRouteFunctions(const TrafficJunctions& junctions) {
    AiRouteFunctions functions;
    functions.evaluate = [&junctions](std::uint32_t route, float distance, AiVec3* position, AiVec3* direction) {
        const TrafficRouteSample sample = sampleTrafficRoute(needPath(junctions.paths, route), distance);
        if (position) *position = sample.position;
        if (direction) *direction = sample.tangent;
    };
    functions.length = [&junctions](std::uint32_t route) { return static_cast<std::uint16_t>(needPath(junctions.paths, route).points.size()); };
    functions.signal = [&junctions](std::uint32_t route) { return trafficSignalId(trafficSignalForRoute(junctions.world.signals, route)); };
    return functions;
}

}
