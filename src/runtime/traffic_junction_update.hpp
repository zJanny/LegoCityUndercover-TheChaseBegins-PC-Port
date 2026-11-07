#pragma once
// The props/traffic_junction objects while the level runs (docs/re/traffic-junction.md): the records of the set-up 0x1ca5a4 kept from the
// scene enter, the class update 0x1d8924 (descriptor 0x1bc31c, slot +0xc) as the entity pass 0x1a3f80 calls it, and the paths of the
// junctions' routes (the level paths they name and their own lanes) behind the route type functions. No SDL, no entities.
#include "assets/assets.hpp"
#include "runtime/ai_driver_input.hpp"
#include "runtime/traffic_junction_fill.hpp"
#include "runtime/traffic_route_mover.hpp"

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace chase::runtime {

// The path of each route a junction knows: its inputs and outputs (the first level path with that name hash) and its lanes.
class TrafficRoutePaths {
public:
    void add(const assets::LevelPath& path);
    const assets::LevelPath* find(std::uint32_t route) const;
    std::size_t size() const { return paths_.size(); }

private:
    std::vector<assets::LevelPath> paths_;
    std::unordered_map<std::uint32_t, std::size_t> index_;
};

struct TrafficJunctions {
    TrafficJunctionWorld world;
    TrafficRoutePaths paths;
};

// The layouts of the fills, for the route set-up (setUpTrafficJunctionRoutes).
std::vector<TrafficJunction> trafficJunctionLayouts(const std::vector<TrafficJunctionFill>& fills);

// The scene enter: the records of `fills` in object order (the index of `signals`), their lane paths and the level paths of their routes.
// A route without a level path has none (the fill skipped such names already).
TrafficJunctions keepTrafficJunctions(std::vector<TrafficJunctionFill> fills, const std::vector<const assets::Level*>& levels,
                                      const TrafficRouteSignals& signals);

struct TrafficJunctionLightEvent {
    std::size_t junction = 0;
    TrafficLightChange change;
};

// The entity pass 0x1a3f80 over the junctions: 0x1d8924(junction, dt) for each, in object order, with the dt of the pass. Returns every
// 0x20b19c change (a red light that waits for an empty queue repeats red over red at each update).
// STAND-IN TRAFFIC-S17: every junction of the level data updates at every update; the original updates the junction entities of its update
// list 0x580564 (added by 0x173534 when 0x23c1a8 places an enabled one in a room of type below 16, layer class 2; skipped with entity
// +0x10 bits 0x24), in the order of the list sorted by the priority byte +0x19 and then the class type (TRAFFIC-24).
std::vector<TrafficJunctionLightEvent> runTrafficJunctionUpdates(TrafficJunctionWorld& world, float dt);

// The route type functions over the junctions' paths: evaluate is slot 0 of the route's type (sampleTrafficRoute), length the point count
// (path hword +10), signal 0x20ff40 as the driver's signal id. They hold a reference to `junctions`, which must outlive them. A route
// without a path throws std::out_of_range. They reach the driver as
// the callbacks of STAND-IN TRAFFIC-S5; `ahead` (+8), `nearest` (+0xc) and the path follower `follow` are not set yet (TRAFFIC-23).
AiRouteFunctions trafficRouteFunctions(const TrafficJunctions& junctions);

}
