#include "runtime/traffic_manager.hpp"

#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace chase::runtime {
namespace {

std::int32_t truncateToInt(float value) {
    if (std::isnan(value)) return 0;
    if (value >= 2147483648.0f) return std::numeric_limits<std::int32_t>::max();
    if (value <= -2147483648.0f) return std::numeric_limits<std::int32_t>::min();
    return static_cast<std::int32_t>(value);
}

float length(const TrafficVec3& a, const TrafficVec3& b) {
    const float x = a[0] - b[0];
    const float y = a[1] - b[1];
    const float z = a[2] - b[2];
    return std::sqrt(x * x + y * y + z * z);
}

float distanceSquared(const TrafficVec3& a, const TrafficVec3& b) {
    const float x = a[0] - b[0];
    const float y = a[1] - b[1];
    const float z = a[2] - b[2];
    return x * x + y * y + z * z;
}

float flatDistanceSquared(const TrafficVec3& a, const TrafficVec3& b) {
    const float x = a[0] - b[0];
    const float z = a[2] - b[2];
    return x * x + z * z;
}

float dot(const TrafficVec3& a, const TrafficVec3& b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }

std::uint32_t floatBits(float value) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof bits);
    return bits;
}

std::uint32_t spacedCountdown(std::uint32_t live) {
    const std::uint32_t scaled = live + (live >> 1);
    return scaled + (scaled << 1);
}

}

TrafficSpawnBand trafficSpawnBand(const TrafficSpawnBand& lod, bool prefill) {
    if (prefill) return {0.0f, lod.farDistance};
    return {lod.nearDistance * kTrafficSpawnNearScale, lod.farDistance * kTrafficSpawnFarScale};
}

void TrafficManager::loadLevel(std::size_t carCount, std::optional<std::uint32_t> maxVehiclesAttribute, GameRandom& random) {
    if (carCount > kTrafficMaxCars) throw std::invalid_argument("traffic: more than 32 Traffic%02u objects");
    cars_.assign(carCount, TrafficCar{});
    fades_.assign(carCount, kTrafficOpaque);
    const std::uint32_t count = static_cast<std::uint32_t>(carCount);
    const std::uint32_t attribute = maxVehiclesAttribute.value_or(kTrafficDefaultMaxVehicles);
    maxVehicles_ = count < attribute ? count : attribute;
    order_.resize(kTrafficMaxCars);
    for (std::uint32_t i = 0; i < kTrafficMaxCars; ++i) order_[i] = i;
    for (std::uint32_t i = 0; i < count; ++i) {
        const std::uint32_t first = random.next() % count;
        const std::uint32_t second = random.next() % count;
        std::swap(order_[first], order_[second]);
    }
    order_.resize(carCount);
}

void TrafficManager::startRoom(bool routesLoaded, std::uint32_t routeCount, const TrafficFrame& frame, GameRandom& random,
                               std::vector<TrafficEvent>* events) {
    routes_.assign(routeCount, true);
    routesLoaded_ = routesLoaded;
    countdown_ = 0;
    prefill_ = true;
    for (unsigned i = 0; i < kTrafficPrefillUpdates; ++i) update(frame, random, events);
    prefill_ = false;
}

void TrafficManager::setMaxVehicles(float argument) {
    const std::uint32_t requested = static_cast<std::uint32_t>(truncateToInt(argument));
    const std::uint32_t count = static_cast<std::uint32_t>(cars_.size());
    maxVehicles_ = count <= requested ? count : requested;
}

void TrafficManager::setRouteEnabled(std::uint32_t route, bool on) {
    if (route >= routes_.size()) routes_.resize(route + 1, false);
    routes_[route] = on;
}

bool TrafficManager::routeEnabled(std::uint32_t route) const { return route < routes_.size() && routes_[route]; }

void TrafficManager::despawnCar(std::size_t car, std::vector<TrafficEvent>* events) {
    cars_[car].disabled = true;
    if (events) events->push_back({TrafficEventKind::Despawn, car});
}

void TrafficManager::lowerCountdown(std::uint32_t live) {
    const std::uint32_t next = spacedCountdown(live);
    if (countdown_ == 0 || countdown_ > next) countdown_ = next;
}

void TrafficManager::refreshFade(std::size_t car, const TrafficFrame& frame, std::vector<TrafficEvent>* events) {
    float fade = 255.0f;
    if (cars_[car].aiDriven) {
        for (const TrafficFadeBound& bound : frame.fadeBounds) {
            TrafficVec3 delta{cars_[car].position[0] - bound.centre[0], cars_[car].position[1] - bound.centre[1],
                              cars_[car].position[2] - bound.centre[2]};
            if (bound.rows) delta = {dot(delta, (*bound.rows)[0]), dot(delta, (*bound.rows)[1]), dot(delta, (*bound.rows)[2])};
            if (!(std::fabs(delta[0]) < bound.halfExtents[0]) || !(std::fabs(delta[1]) < bound.halfExtents[1]) ||
                !(std::fabs(delta[2]) < bound.halfExtents[2]))
                continue;
            const float depth = delta[2] < 0.0f ? 0.0f : delta[2] / bound.halfExtents[2];
            if (!(0.0f > depth) && static_cast<std::int32_t>(floatBits(depth)) <= static_cast<std::int32_t>(floatBits(1.0f)))
                fade = depth * fade;
            break;
        }
    }
    if (static_cast<float>(fades_[car]) == fade) return;
    fades_[car] = static_cast<std::uint8_t>(truncateToInt(fade));
    if (events) events->push_back({TrafficEventKind::Fade, car, fades_[car]});
}

bool TrafficManager::trySpawn(std::size_t car, const TrafficFrame& frame, GameRandom& random, std::vector<TrafficEvent>* events) {
    if (frame.cell.empty()) return false;
    const TrafficVec3& focus = prefill_ ? frame.player : frame.camera;
    const std::uint32_t cellSize = static_cast<std::uint32_t>(frame.cell.size());
    for (unsigned attempt = 0; attempt < kTrafficSpawnTries; ++attempt) {
        const std::uint32_t pointIndex = random.next() % cellSize;
        const TrafficSpawnPoint& point = frame.cell[pointIndex];
        if (!point.route || !routeEnabled(*point.route)) continue;
        if (!point.roomActive) continue;
        const float distance = length(point.position, focus);
        const TrafficSpawnBand band = trafficSpawnBand(frame.bands.at(cars_[car].lodClass), prefill_);
        if (band.nearDistance > distance || distance > band.farDistance) continue;
        bool crowded = false;
        for (std::size_t other = 0; other < cars_.size() && !crowded; ++other) {
            if (other == car) continue;
            const TrafficCar& driver = cars_[other];
            if (driver.route != point.route) continue;
            if (driver.driveState == 0 && driver.driveStage == 0) continue;
            crowded = kTrafficRouteSpacingSquared > flatDistanceSquared(driver.position, point.position);
        }
        if (crowded) continue;
        refreshFade(car, frame, events);
        TrafficCar& spawned = cars_[car];
        spawned.driveStage = 1;
        spawned.driveState = 0;
        spawned.aiDriven = true;
        spawned.route = point.route;
        spawned.cruiseSpeed = point.speed;
        if (frame.place && !frame.place(car, point)) return false;
        spawned.disabled = false;
        if (events) events->push_back({TrafficEventKind::Spawn, car, fades_[car], pointIndex});
        return true;
    }
    return false;
}

void TrafficManager::update(const TrafficFrame& frame, GameRandom& random, std::vector<TrafficEvent>* events) {
    if (!enabled_) {
        for (std::size_t car = 0; car < cars_.size(); ++car) {
            if (cars_[car].disabled || cars_[car].pinned) continue;
            despawnCar(car, events);
            fades_[car] = kTrafficOpaque;
        }
        return;
    }
    if (!routesLoaded_) return;

    std::uint32_t live = 0;
    for (const TrafficCar& car : cars_)
        if (!car.disabled) ++live;
    countdown_ = countdown_ > 1 ? countdown_ - 1 : 0;
    const std::uint32_t count = static_cast<std::uint32_t>(cars_.size());
    if (maxVehicles_ > live && countdown_ == 0) {
        for (std::uint32_t slot = 0; slot < maxVehicles_; ++slot) {
            const std::uint32_t car = order_[slot % count];
            if (!cars_[car].disabled) continue;
            if (!trySpawn(car, frame, random, events)) break;
            order_.erase(order_.begin() + slot);
            order_.push_back(car);
            ++live;
            lowerCountdown(live);
            break;
        }
    }

    for (std::size_t index = 0; index < cars_.size(); ++index) {
        TrafficCar& car = cars_[index];
        if (car.disabled) continue;
        if (car.driveStage == 1 || car.driveState == 1) continue;
        if (car.driveState == 2 || car.driveState == 3) refreshFade(index, frame, events);
        if (car.roomActive && car.driveState != 0 && car.driveStage != 0) {
            const std::uint8_t fade = fades_[index];
            const bool visible = fade != 0 && (fade > kTrafficFadeKeepLimit || car.driveValue > 0.0f);
            if (visible) {
                if (car.keepWhenFar) continue;
                if (distanceSquared(car.position, frame.player) <= car.despawnRadius * car.despawnRadius) continue;
            }
        }
        if (!car.pinned) {
            despawnCar(index, events);
            fades_[index] = kTrafficOpaque;
        }
        lowerCountdown(live - 1);
        return;
    }
}

void runSetTrafficMaxVehicles(TrafficManager& manager, const std::vector<float>& values) {
    if (values.size() != 1) throw std::invalid_argument("SetTrafficMaxVehicles: expected 1 number, got " + std::to_string(values.size()));
    manager.setMaxVehicles(values[0]);
}

}
