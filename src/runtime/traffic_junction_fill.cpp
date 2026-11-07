#include "runtime/traffic_junction_fill.hpp"
#include "assets/path_name_hash.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>

namespace chase::runtime {
namespace {

constexpr const char* kJunctionClass = "props/traffic_junction";
constexpr std::int32_t kUnitMinimumBits = 0x34000000;
constexpr std::int32_t kOneBits = 0x3f800000;

std::int32_t floatBits(float value) {
    std::int32_t bits = 0;
    std::memcpy(&bits, &value, sizeof bits);
    return bits;
}

TrafficRouteVec3 minus(const TrafficRouteVec3& a, const TrafficRouteVec3& b) {
    return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}

float dot3(const TrafficRouteVec3& a, const TrafficRouteVec3& b) {
    float sum = a[0] * b[0];
    sum += a[1] * b[1];
    sum += a[2] * b[2];
    return sum;
}

float dot2(const std::array<float, 2>& a, const std::array<float, 2>& b) {
    float sum = a[0] * b[0];
    sum += a[1] * b[1];
    return sum;
}

TrafficRouteVec3 unit(const TrafficRouteVec3& v) {
    const float size = std::sqrt(dot3(v, v));
    if (floatBits(size) <= kUnitMinimumBits) return {0.0f, 0.0f, 1.0f};
    const float inverse = 1.0f / size;
    return {v[0] * inverse, v[1] * inverse, v[2] * inverse};
}

TrafficRouteVec3 along(const TrafficRouteVec3& origin, const TrafficRouteVec3& direction, float scale) {
    return {origin[0] + direction[0] * scale, origin[1] + direction[1] * scale, origin[2] + direction[2] * scale};
}

float flatSquared(const TrafficRouteVec3& a, const TrafficRouteVec3& b) {
    const float dx = a[0] - b[0];
    const float dz = a[2] - b[2];
    float sum = dx * dx;
    sum += dz * dz;
    return sum;
}

TrafficJunctionEnd endOf(const assets::LevelPath& path, const TrafficRouteVec3& point, const TrafficRouteVec3& inner,
                         const TrafficRouteVec3& travel) {
    TrafficJunctionEnd end;
    end.route = path.nameHash;
    end.point = point;
    end.inner = inner;
    const TrafficRouteVec3 direction = unit(travel);
    end.direction = {direction[0], direction[2]};
    return end;
}

void needTwoPoints(const assets::LevelPath& path) {
    if (path.points.size() < 2) throw std::runtime_error("traffic junction: STAND-IN TRAFFIC-S13, a junction path of fewer than two points");
}

TrafficRouteVec3 hermite(const TrafficRouteVec3& p0, const TrafficRouteVec3& p1, const TrafficRouteVec3& t0, const TrafficRouteVec3& t1,
                         float t) {
    const float t2 = t * t;
    const float t3 = t2 * t;
    const float h01 = t2 * 3.0f - t3 * 2.0f;
    const float h11 = t3 - t2;
    const float h10 = (t3 - t2 * 2.0f) + t;
    const float h00 = (t3 * 2.0f - t2 * 3.0f) + 1.0f;
    TrafficRouteVec3 out{};
    for (int i = 0; i < 3; ++i) {
        out[i] = p0[i] * h00;
        out[i] = out[i] + t0[i] * h10;
        out[i] = out[i] + p1[i] * h01;
        out[i] = out[i] + t1[i] * h11;
    }
    return out;
}

std::string textOf(const assets::LevelParam& param) {
    if (param.blocks.empty() || param.blocks[0].empty()) return {};
    const auto& block = param.blocks[0];
    std::size_t length = 0;
    while (length < block.size() && block[length] != 0) ++length;
    return std::string(reinterpret_cast<const char*>(block.data()), length);
}

const assets::LevelParam* findParam(const assets::LevelObject& object, const char* name) {
    const std::uint32_t hash = assets::nameHash(name);
    for (const assets::LevelParam& param : object.params)
        if (param.hash == hash) return &param;
    return nullptr;
}

struct LevelIndex {
    const std::vector<const assets::Level*>& levels;

    const assets::LevelPath* path(const std::string& name) const {
        const std::uint32_t hash = assets::pathNameHash(name);
        for (const assets::Level* level : levels)
            for (const assets::LevelPath& path : level->paths)
                if (path.nameHash == hash) return &path;
        return nullptr;
    }

    const assets::LevelObject* object(std::uint32_t hash) const {
        if (hash == 0) return nullptr;
        for (const assets::Level* level : levels)
            for (const assets::LevelObject& object : level->objects)
                if (assets::nameHash(object.instance) == hash) return &object;
        return nullptr;
    }

    std::vector<const assets::LevelPath*> paths(const assets::LevelObject& object, const char* format, std::size_t slots) const {
        std::vector<const assets::LevelPath*> list;
        for (std::size_t slot = 1; slot <= slots; ++slot) {
            char name[24];
            std::snprintf(name, sizeof name, format, static_cast<unsigned>(slot));
            const assets::LevelParam* param = findParam(object, name);
            if (param == nullptr) continue;
            if (const assets::LevelPath* found = path(textOf(*param))) list.push_back(found);
        }
        return list;
    }

    TrafficLight light(const assets::LevelObject& object) const {
        TrafficLight light;
        for (const assets::LevelPath* path : paths(object, "Path%02u", kTrafficLightPathSlots)) light.routes.push_back(path->nameHash);
        return light;
    }

    const assets::LevelObject* linked(const assets::LevelObject& object, const char* name) const {
        const assets::LevelParam* param = findParam(object, name);
        return param == nullptr ? nullptr : this->object(assets::objectReferenceHash(*param));
    }
};

TrafficRouteVec3 positionOf(const assets::LevelObject& object) {
    const assets::LevelParam* param = findParam(object, "initialposition");
    if (param == nullptr || param->type != 2 || param->blocks.empty() || param->blocks[0].size() < 12)
        throw std::runtime_error("traffic junction " + object.instance + " has no initialposition");
    TrafficRouteVec3 position{};
    std::memcpy(position.data(), param->blocks[0].data(), 12);
    return position;
}

}

TrafficJunctionEnd trafficJunctionInputEnd(const assets::LevelPath& path) {
    needTwoPoints(path);
    const std::size_t n = path.points.size();
    return endOf(path, path.points[n - 1], path.points[n - 2], minus(path.points[n - 1], path.points[n - 2]));
}

TrafficJunctionEnd trafficJunctionOutputEnd(const assets::LevelPath& path) {
    needTwoPoints(path);
    return endOf(path, path.points[0], path.points[1], minus(path.points[1], path.points[0]));
}

void markTrafficJunctionSides(std::vector<TrafficJunctionEnd>& ends) {
    for (std::size_t a = 0; a < ends.size(); ++a)
        for (std::size_t b = a + 1; b < ends.size(); ++b) {
            if (floatBits(dot2(ends[a].direction, ends[b].direction)) < kTrafficJunctionStraightBits) continue;
            const std::array<float, 2> across{-ends[b].direction[1], ends[b].direction[0]};
            const std::array<float, 2> gap{ends[a].point[0] - ends[b].point[0], ends[a].point[2] - ends[b].point[2]};
            if (dot2(across, gap) < 0.0f) {
                ends[a].side = 1;
                ends[b].side = 2;
            } else {
                ends[a].side = 2;
                ends[b].side = 1;
            }
        }
}

std::vector<std::size_t> trafficJunctionChoices(const TrafficJunctionEnd& input, const std::vector<TrafficJunctionEnd>& outputs,
                                                std::size_t inputCount) {
    if (inputCount == 1 && outputs.size() == 1) return {0};
    std::vector<std::size_t> choices;
    for (std::size_t o = 0; o < outputs.size(); ++o) {
        const TrafficJunctionEnd& output = outputs[o];
        const float dot = dot2(input.direction, output.direction);
        if (static_cast<std::uint32_t>(floatBits(dot)) >= kTrafficJunctionTurnBackBits) continue;
        bool take = false;
        if (input.side != 0 && output.side != 0) {
            take = input.side == output.side;
        } else if (floatBits(dot) > kTrafficJunctionStraightBits || input.side == 0) {
            take = true;
        } else {
            float cross = input.direction[0] * output.direction[1];
            cross -= input.direction[1] * output.direction[0];
            take = cross > 0.0f ? input.side == 2 : input.side == 1;
        }
        if (take) choices.push_back(o);
    }
    return choices;
}

assets::LevelPath trafficJunctionLanePath(const TrafficJunctionEnd& input, const TrafficJunctionEnd& output, std::uint32_t id) {
    const float reach = std::sqrt(flatSquared(input.point, output.point));
    const TrafficRouteVec3 t0{input.direction[0] * reach, 0.0f * reach, input.direction[1] * reach};
    const TrafficRouteVec3 t1{output.direction[0] * reach, 0.0f * reach, output.direction[1] * reach};
    assets::LevelPath lane;
    lane.nameHash = id;
    lane.type = 0;
    lane.closed = false;
    lane.points = {input.point, hermite(input.point, output.point, t0, t1, kTrafficLaneFirstThird),
                   hermite(input.point, output.point, t0, t1, kTrafficLaneSecondThird), output.point};
    return lane;
}

TrafficSegmentClosest trafficSegmentClosest(const TrafficRouteVec3& a0, const TrafficRouteVec3& a1, const TrafficRouteVec3& b0,
                                            const TrafficRouteVec3& b1) {
    const TrafficRouteVec3 u = minus(a1, a0);
    const TrafficRouteVec3 v = minus(b1, b0);
    const TrafficRouteVec3 w = minus(a0, b0);
    const float uu = dot3(u, u);
    const float uv = dot3(u, v);
    const float uw = dot3(u, w);
    const float vv = dot3(v, v);
    const float vw = dot3(v, w);
    float denominator = uu * vv;
    denominator -= uv * uv;
    TrafficSegmentClosest closest;
    if (denominator <= 0.0f) return closest;
    float s = uv * vw;
    s -= uw * vv;
    s /= denominator;
    if (0.0f > s || floatBits(s) > kOneBits) return closest;
    float t = uu * vw;
    t -= uv * uw;
    t /= denominator;
    if (t < 0.0f || floatBits(t) > kOneBits) return closest;
    closest.found = true;
    closest.onA = along(a0, u, s);
    closest.onB = along(b0, v, t);
    return closest;
}

bool trafficLanesCross(const assets::LevelPath& a, const assets::LevelPath& b) {
    const float limit = kTrafficLaneCrossDistance * kTrafficLaneCrossDistance;
    if (a.points.size() < 2 || b.points.size() < 2) return false;
    if (b.points.size() < a.points.size())
        throw std::runtime_error("traffic junction: STAND-IN TRAFFIC-S13, a lane with fewer points than the lane it is tested against");
    for (std::size_t i = 1; i < a.points.size(); ++i) {
        const TrafficSegmentClosest closest = trafficSegmentClosest(a.points[i - 1], a.points[i], b.points[i - 1], b.points[i]);
        if (closest.found && flatSquared(closest.onA, closest.onB) <= limit) return true;
    }
    return false;
}

TrafficJunctionFill fillTrafficJunction(const TrafficJunctionSource& source, std::uint32_t firstLaneId) {
    TrafficJunctionFill fill;
    TrafficJunction& layout = fill.record.layout;
    layout.position = source.position;
    for (const assets::LevelPath* path : source.inputs) {
        fill.inputEnds.push_back(trafficJunctionInputEnd(*path));
        layout.inputs.push_back(path->nameHash);
    }
    for (const assets::LevelPath* path : source.outputs) {
        fill.outputEnds.push_back(trafficJunctionOutputEnd(*path));
        layout.outputs.push_back(path->nameHash);
    }
    markTrafficJunctionSides(fill.inputEnds);
    markTrafficJunctionSides(fill.outputEnds);
    if (source.lights.size() > kTrafficJunctionLightSlots) throw std::runtime_error("traffic junction: more than 8 light pairs");
    fill.record.lightPairs = source.lights;
    fill.lightSlots = source.lightSlots;
    fill.lightInstances = source.lightInstances;
    fill.record.passPair = 0;
    fill.record.lightPhase = kTrafficLightGreen;
    fill.record.lightTimer = kTrafficLightGreenTime;
    for (std::size_t i = 0; i < fill.inputEnds.size(); ++i) {
        fill.record.choices.push_back(trafficJunctionChoices(fill.inputEnds[i], fill.outputEnds, fill.inputEnds.size()));
        for (std::size_t output : fill.record.choices.back()) {
            const std::uint32_t id = firstLaneId + static_cast<std::uint32_t>(fill.lanePaths.size());
            fill.lanePaths.push_back(trafficJunctionLanePath(fill.inputEnds[i], fill.outputEnds[output], id));
            fill.record.laneEnds.push_back({i, output});
            layout.lanes.push_back(id);
        }
    }
    for (std::size_t a = 0; a < fill.lanePaths.size(); ++a)
        for (std::size_t b = a + 1; b < fill.lanePaths.size(); ++b) {
            if (fill.record.laneEnds[a].input == fill.record.laneEnds[b].input) continue;
            if (fill.record.laneEnds[a].output == fill.record.laneEnds[b].output) continue;
            if (!trafficLanesCross(fill.lanePaths[a], fill.lanePaths[b])) continue;
            if (layout.conflicts.size() >= kTrafficJunctionConflictSlots)
                throw std::runtime_error("traffic junction: STAND-IN TRAFFIC-S13, more than 64 conflict pairs");
            layout.conflicts.push_back({static_cast<std::uint8_t>(a), static_cast<std::uint8_t>(b)});
        }
    return fill;
}

std::vector<TrafficJunctionFill> trafficJunctionFills(const std::vector<const assets::Level*>& levels) {
    const LevelIndex index{levels};
    std::vector<TrafficJunctionFill> fills;
    std::uint32_t nextLaneId = kTrafficLaneIdBase;
    for (const assets::Level* level : levels)
        for (const assets::LevelObject& object : level->objects) {
            if (object.className != kJunctionClass) continue;
            TrafficJunctionSource source;
            source.position = positionOf(object);
            source.inputs = index.paths(object, "InputPath%02u", 8);
            source.outputs = index.paths(object, "OutputPath%02u", 8);
            for (std::size_t slot = 1; slot <= kTrafficJunctionLightSlots; ++slot) {
                char name[24];
                std::snprintf(name, sizeof name, "TrafficLight%02u", static_cast<unsigned>(slot));
                const assets::LevelObject* light = index.linked(object, name);
                if (light == nullptr) continue;
                std::array<std::optional<TrafficLight>, 2> pair{index.light(*light), std::nullopt};
                std::array<std::string, 2> instances{light->instance, std::string()};
                if (const assets::LevelObject* slave = index.linked(*light, "SlaveLight")) {
                    pair[1] = index.light(*slave);
                    instances[1] = slave->instance;
                }
                source.lights.push_back(pair);
                source.lightSlots.push_back(slot - 1);
                source.lightInstances.push_back(instances);
            }
            fills.push_back(fillTrafficJunction(source, nextLaneId));
            nextLaneId += static_cast<std::uint32_t>(fills.back().lanePaths.size());
        }
    for (const assets::Level* level : levels)
        for (const assets::LevelPath& path : level->paths)
            if (path.nameHash >= kTrafficLaneIdBase && path.nameHash < nextLaneId)
                throw std::runtime_error("traffic junction: STAND-IN TRAFFIC-S13, a lane id equals a level path's name hash");
    return fills;
}

std::optional<TrafficLightChange> runTrafficJunctionLights(TrafficJunctionRecord& junction, float dt) {
    const std::size_t pairs = junction.lightPairs.size();
    if (pairs <= 1) return std::nullopt;
    float timer = junction.lightTimer - dt;
    if (timer < 0.0f) timer = 0.0f;
    junction.lightTimer = timer;
    if (timer != 0.0f) return std::nullopt;
    TrafficLightChange change;
    change.previous = junction.lightPhase;
    change.pair = junction.passPair;
    change.phase = kTrafficLightRed;
    float next = 0.0f;
    if (change.previous == kTrafficLightRed) {
        if (junction.queuedCars.empty()) {
            next = kTrafficLightGreenTime;
            change.phase = kTrafficLightGreen;
            change.pair = (junction.passPair + 1) % pairs;
        }
    } else if (change.previous == kTrafficLightYellow) {
        next = kTrafficLightRedTime;
    } else if (change.previous == kTrafficLightGreen) {
        change.phase = kTrafficLightYellow;
        next = kTrafficLightYellowTime;
    }
    junction.passPair = static_cast<std::uint32_t>(change.pair);
    junction.lightPhase = change.phase;
    junction.lightTimer = next;
    return change;
}

}
