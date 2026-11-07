#include "runtime/traffic_route_mover.hpp"

#include "runtime/pedestrian_walk.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string>

namespace chase::runtime {
namespace {

constexpr std::int32_t kUnitMinimumBits = 0x34000000;

template <typename Hook>
const Hook& need(const Hook& hook, const char* name) {
    if (!hook) throw std::logic_error(std::string("traffic route mover: hook ") + name + " is not set");
    return hook;
}

const AiRouteFunctions& needRoutes(const AiRouteFunctions* routes) {
    if (routes == nullptr) throw std::logic_error("traffic route mover: no route functions");
    return *routes;
}

std::int32_t floatBits(float value) {
    std::int32_t bits = 0;
    std::memcpy(&bits, &value, sizeof bits);
    return bits;
}

AiVec3 unitOf(const AiVec3& v) {
    float squared = v[0] * v[0];
    squared += v[1] * v[1];
    squared += v[2] * v[2];
    const float size = std::sqrt(squared);
    if (floatBits(size) <= kUnitMinimumBits) return {0.0f, 0.0f, 1.0f};
    const float inverse = 1.0f / size;
    return {v[0] * inverse, v[1] * inverse, v[2] * inverse};
}

AiVec3 crossOf(const AiVec3& a, const AiVec3& b) {
    return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}

AiVec3 lerpOf(const AiVec3& a, const AiVec3& b, float t) {
    return {a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t, a[2] + (b[2] - a[2]) * t};
}

void orthonormalise(AiMatrix& m) {
    m[2] = unitOf(m[2]);
    m[1] = unitOf(crossOf(m[2], m[0]));
    m[0] = crossOf(m[1], m[2]);
}

float distanceOf(const AiVec3& a, const AiVec3& b) {
    const float dx = a[0] - b[0];
    const float dy = a[1] - b[1];
    const float dz = a[2] - b[2];
    float squared = dx * dx;
    squared += dy * dy;
    squared += dz * dz;
    return std::sqrt(squared);
}

void routeTarget(AiDriverRecord& driver, const AiRouteFunctions& routes, float bodyBase) {
    need(routes.evaluate, "evaluate")(driver.route, driver.routeDistance, &driver.target, nullptr);
    driver.target[1] += driver.heightOffset - bodyBase;
}

std::optional<std::size_t> findInput(const TrafficJunctionRecord& junction, std::uint32_t route) {
    for (std::size_t i = 0; i < junction.layout.inputs.size(); ++i)
        if (junction.layout.inputs[i] == route) return i;
    return std::nullopt;
}

std::size_t pickOutput(const TrafficJunctionRecord& junction, std::size_t input, const TrafficRouteTable& table, GameRandom& random) {
    const std::vector<std::size_t>& choices = junction.choices.at(input);
    if (choices.size() > kTrafficJunctionChoices) throw std::runtime_error("traffic junction: more than 8 choices for an input");
    if (choices.size() == 1) return choices[0];
    const bool anyEnabled = std::any_of(choices.begin(), choices.end(), [&](std::size_t output) {
        return trafficRouteEnabled(table, junction.layout.outputs.at(output));
    });
    if (!anyEnabled) throw std::runtime_error("traffic junction: STAND-IN TRAFFIC-S11, no enabled output for the input");
    const std::uint32_t spread = static_cast<std::uint32_t>(choices.size()) * kTrafficJunctionChoiceSpread;
    for (;;) {
        const std::size_t output = choices[(random.next() % spread) >> 3];
        if (trafficRouteEnabled(table, junction.layout.outputs.at(output))) return output;
    }
}

}

std::uint32_t trafficSignalId(std::optional<std::size_t> junction) {
    return junction ? static_cast<std::uint32_t>(*junction + 1) : 0u;
}

const TrafficJunctionRecord* trafficSignalJunction(const TrafficJunctionWorld& world, std::uint32_t signal) {
    if (signal == 0 || signal > world.junctions.size()) return nullptr;
    return &world.junctions[signal - 1];
}

TrafficJunctionRecord* trafficSignalJunction(TrafficJunctionWorld& world, std::uint32_t signal) {
    if (signal == 0 || signal > world.junctions.size()) return nullptr;
    return &world.junctions[signal - 1];
}

bool trafficLightControls(const TrafficLight& light, std::uint32_t route) {
    if (light.routes.size() > kTrafficLightRoutes) throw std::runtime_error("traffic light: more than 4 routes");
    return std::find(light.routes.begin(), light.routes.end(), route) != light.routes.end();
}

bool trafficStopAtLights(const AiDriverRecord& driver, const TrafficJunctionWorld& world) {
    if (driver.signal == 0) return false;
    const TrafficJunctionRecord* junction = trafficSignalJunction(world, driver.signal);
    if (junction == nullptr) throw std::logic_error("traffic stop test: unknown signal object");
    if (junction->lightPairs.empty()) return false;
    if (junction->lightPairs.size() > kTrafficJunctionLightPairs) throw std::runtime_error("traffic junction: more than 8 light pairs");
    std::uint32_t inputRoute = 0;
    const std::optional<std::size_t> input = findInput(*junction, driver.route);
    if (input) inputRoute = junction->layout.inputs[*input];
    if (inputRoute == 0) {
        const std::vector<std::uint32_t>& lanes = junction->layout.lanes;
        if (std::find(lanes.begin(), lanes.end(), driver.route) != lanes.end()) return false;
    }
    for (std::size_t pair = 0; pair < junction->lightPairs.size(); ++pair) {
        for (const std::optional<TrafficLight>& light : junction->lightPairs[pair]) {
            if (!light || !trafficLightControls(*light, inputRoute)) continue;
            if (junction->passPair != pair || junction->lightPhase == 0) return true;
        }
    }
    return false;
}

void leaveTrafficJunctionQueue(TrafficJunctionRecord& junction, AiDriverRecord& driver, std::uint32_t car) {
    if ((driver.flags & kAiDriverFlagSignal) == 0) return;
    std::vector<std::uint32_t>& cars = junction.queuedCars;
    std::vector<std::uint32_t>& lanes = junction.layout.listedLanes;
    if (cars.size() != lanes.size()) throw std::logic_error("traffic junction queue: car and lane lists differ");
    if (cars.empty()) throw std::runtime_error("traffic junction queue: STAND-IN TRAFFIC-S11, leaving an empty queue");
    std::size_t found = cars.size();
    for (std::size_t i = cars.size(); i-- > 0;) {
        if (cars[i] == car) {
            found = i;
            break;
        }
    }
    const std::size_t last = cars.size() - 1;
    if (found < last) {
        cars[found] = cars[last];
        lanes[found] = lanes[last];
    }
    cars.pop_back();
    lanes.pop_back();
    driver.flags = static_cast<std::uint8_t>(driver.flags & ~kAiDriverFlagSignal);
}

bool chooseTrafficJunction(AiDriverRecord& driver, const TrafficJunctionChoiceFrame& frame, TrafficJunctionWorld& world, GameRandom& random,
                           std::uint32_t& signal) {
    const AiRouteFunctions& routes = needRoutes(frame.routes);
    const float length = static_cast<float>(static_cast<std::int32_t>(need(routes.length, "length")(driver.route)));
    if (length - kTrafficJunctionChoiceMargin > driver.routeDistance) return false;
    const std::uint32_t junctionId = trafficSignalId(trafficSignalForRoute(world.signals, driver.route));
    TrafficJunctionRecord* junction = trafficSignalJunction(world, junctionId);
    if (junction == nullptr) {
        signal = 0;
        return true;
    }
    const std::optional<std::size_t> input = findInput(*junction, driver.route);
    std::optional<std::size_t> output;
    if (input && !junction->choices.at(*input).empty()) output = pickOutput(*junction, *input, world.routes, random);
    std::optional<std::size_t> lane;
    if (input && output) {
        for (std::size_t i = 0; i < junction->laneEnds.size(); ++i) {
            if (junction->laneEnds[i].input == *input && junction->laneEnds[i].output == *output) {
                lane = i;
                break;
            }
        }
    }
    if (!lane) throw std::runtime_error("traffic junction: STAND-IN TRAFFIC-S11, no lane joins the car's input and the chosen output");
    if (junction->queuedCars.size() >= kTrafficJunctionQueueSize) throw std::runtime_error("traffic junction: STAND-IN TRAFFIC-S11, queue full");
    driver.route = junction->layout.lanes.at(*lane);
    driver.nextRoute = junction->layout.outputs.at(*output);
    driver.routeDistance = need(routes.nearest, "nearest")(driver.route, frame.nodePosition, kTrafficJunctionNearestSamples,
                                                           kTrafficJunctionNearestScale);
    routeTarget(driver, routes, frame.bodyBase);
    driver.signal = junctionId;
    driver.flags = static_cast<std::uint8_t>(driver.flags | kAiDriverFlagSignal);
    junction->queuedCars.push_back(frame.car);
    junction->layout.listedLanes.push_back(static_cast<std::uint32_t>(*lane));
    signal = junctionId;
    return true;
}

TrafficMoverResult runTrafficRouteMover(AiDriverRecord& driver, const TrafficMoverFrame& frame, TrafficJunctionWorld& world,
                                        const AiDriverHooks& hooks, GameRandom& random) {
    const AiRouteFunctions& routes = needRoutes(frame.routes);
    driver.moveCounter = static_cast<std::uint8_t>(driver.moveCounter + 1);
    if (driver.moveCounter > kTrafficMoverSnapAfter) {
        need(hooks.snapTarget, "snapTarget")();
        driver.moveCounter = 0;
    }
    AiMatrix node = frame.node;
    if (driver.speed > kTrafficMoverTurnSpeed) {
        const float turn = frame.dt * (driver.speed > kTrafficMoverFastSpeed ? kTrafficMoverFastTurnRate : kTrafficMoverTurnRate);
        AiMatrix look = node;
        pedestrianLookAt(look, node[3], driver.target);
        for (int row = 0; row < 3; ++row) node[row] = lerpOf(node[row], look[row], turn);
        orthonormalise(node);
        for (int row = 0; row < 3; ++row) driver.path[row] = node[row];
    }
    const float step = frame.dt * driver.speed;
    for (int i = 0; i < 3; ++i) node[3][i] += node[2][i] * step;
    if (distanceOf(node[3], driver.target) < frame.radius) {
        const std::uint16_t length = need(routes.length, "length")(driver.route);
        const float end = static_cast<float>(length);
        if (!(driver.routeDistance <= end)) {
            for (int i = 0; i < 3; ++i) driver.target[i] += node[2][i];
        } else {
            driver.routeDistance += length == kTrafficMoverShortRoute ? kTrafficMoverShortRouteStep : kTrafficMoverRouteStep;
            if (!(driver.routeDistance < end)) {
                driver.routeDistance = end;
                std::uint32_t signal = 0;
                if (driver.targetSpeed > 0.0f) {
                    const TrafficJunctionChoiceFrame choice{frame.car, frame.node[3], frame.bodyBase, frame.routes};
                    if (chooseTrafficJunction(driver, choice, world, random, signal)) {
                        if (signal != 0) driver.requestedState = kAiStateJunction;
                        else need(hooks.leaveTraffic, "leaveTraffic")();
                    }
                }
            }
            routeTarget(driver, routes, frame.bodyBase);
        }
    }
    TrafficMoverResult result;
    result.node = node;
    if (frame.kinematic) result.velocity = AiVec3{node[2][0] * driver.speed, node[2][1] * driver.speed, node[2][2] * driver.speed};
    const float ratio = driver.speed / frame.gearSpeed;
    result.engineBlend = frame.engineFrom + (frame.engineTo - frame.engineFrom) * ratio;
    const float difference = driver.targetSpeed - driver.speed;
    if (floatBits(std::fabs(difference)) <= kTrafficMoverSpeedSnapBits) driver.speed = driver.targetSpeed;
    else driver.speed += difference * kTrafficMoverSpeedBlend;
    return result;
}

}
