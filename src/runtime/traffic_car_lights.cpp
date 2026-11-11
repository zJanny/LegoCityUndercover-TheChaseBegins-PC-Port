#include "runtime/traffic_car_lights.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace chase::runtime {

namespace {

bool sameName(const std::string& a, const std::string& b) {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) {
               return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y));
           });
}

}

std::int32_t trafficLightNodeIndex(const std::vector<std::string>& nodeNames, const std::string& name) {
    if (name.empty()) return -1;
    for (std::size_t index = 0; index < nodeNames.size(); ++index)
        if (sameName(nodeNames[index], name)) return static_cast<std::int32_t>(index);
    return -1;
}

TrafficBrakeLights makeTrafficBrakeLights(const TrafficBrakeLightClass& values, const std::vector<std::string>& nodeNames) {
    TrafficBrakeLights lights;
    for (std::size_t i = 0; i < kTrafficBrakeLightParts; ++i) lights.parts[i] = trafficLightNodeIndex(nodeNames, values.models[i]);
    for (std::size_t i = 0; i < kTrafficBrakeGlows; ++i) lights.glowParts[i] = trafficLightNodeIndex(nodeNames, values.locators[i]);
    lights.glowEffect = static_cast<std::uint32_t>(values.particle);
    return lights;
}

void TrafficCarLights::load(std::size_t carCount) {
    cars_.assign(carCount, Car{});
    pending_ = {};
}

void TrafficCarLights::setup(std::size_t car, const TrafficBrakeLights& lights) {
    Car& entry = cars_.at(car);
    entry.lights = lights;
    entry.ready = true;
    for (const std::int32_t part : lights.parts)
        if (part >= 0) pending_.parts.push_back({car, {part, false, 0}});
}

bool TrafficCarLights::configured(std::size_t car) const { return cars_.at(car).ready; }

void TrafficCarLights::setNodeCulled(std::size_t car, bool culled) { cars_.at(car).culled = culled; }

void TrafficCarLights::run(std::size_t car, float brake, float dt) {
    Car& entry = cars_.at(car);
    if (!entry.ready) return;
    TrafficBrakeLightFrame frame;
    frame.nodeCulled = entry.culled;
    frame.brake = brake;
    frame.dt = dt;
    const TrafficBrakeLightResult result = runTrafficBrakeLights(entry.lights, frame);
    for (const TrafficBrakeLightPart& part : result.parts) pending_.parts.push_back({car, part});
    for (const std::size_t slot : result.glowStarts)
        pending_.glows.push_back({car, slot, entry.lights.glowParts[slot], entry.lights.glowEffect, true, 0});
    for (const std::size_t slot : result.glowStops)
        pending_.glows.push_back({car, slot, entry.lights.glowParts[slot], entry.lights.glowEffect, false, entry.lights.glows[slot]});
}

void TrafficCarLights::disable(std::size_t car) {
    Car& entry = cars_.at(car);
    if (!entry.ready) return;
    entry.lights.level = 0.0f;
    for (const std::int32_t part : entry.lights.parts)
        if (part > 0) pending_.parts.push_back({car, {part, false, 0}});
    for (std::size_t slot = 0; slot < kTrafficBrakeGlows; ++slot)
        if (entry.lights.glows[slot] != 0) {
            pending_.glows.push_back({car, slot, entry.lights.glowParts[slot], entry.lights.glowEffect, false, entry.lights.glows[slot]});
            entry.lights.glows[slot] = 0;
        }
}

void TrafficCarLights::glowStarted(std::size_t car, std::size_t slot, std::uint32_t handle) {
    if (slot >= kTrafficBrakeGlows) throw std::out_of_range("traffic brake light glow slot");
    cars_.at(car).lights.glows[slot] = handle;
}

void TrafficCarLights::glowEvent(std::size_t car, std::uint32_t handle, int event) { endTrafficBrakeGlow(cars_.at(car).lights, handle, event); }

TrafficCarLights::Changes TrafficCarLights::takeChanges() {
    Changes out = std::move(pending_);
    pending_ = {};
    return out;
}

}
