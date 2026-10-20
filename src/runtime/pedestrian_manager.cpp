#include "runtime/pedestrian_manager.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>

namespace chase::runtime {

namespace {

constexpr std::int32_t kNormalizeMinimumBits = 0x34000000;
constexpr float kPlacementUpLimit = 0.98f;

std::int32_t floatBits(float value) {
    std::int32_t bits = 0;
    std::memcpy(&bits, &value, sizeof bits);
    return bits;
}

float normalize(PedestrianVec3& v) {
    float squared = v[0] * v[0];
    squared = squared + v[1] * v[1];
    squared = squared + v[2] * v[2];
    const float length = std::sqrt(squared);
    if (floatBits(length) <= kNormalizeMinimumBits) {
        v = {0.0f, 0.0f, 1.0f};
        return length;
    }
    const float inverse = 1.0f / length;
    v = {v[0] * inverse, v[1] * inverse, v[2] * inverse};
    return length;
}

float dot(const PedestrianVec3& a, const PedestrianVec3& b) {
    float sum = a[0] * b[0];
    sum = sum + a[1] * b[1];
    sum = sum + a[2] * b[2];
    return sum;
}

float horizontalDistanceSquared(const PedestrianVec3& a, const PedestrianVec3& b) {
    const float dx = a[0] - b[0];
    const float dz = a[2] - b[2];
    float sum = dx * dx;
    sum = sum + dz * dz;
    return sum;
}

std::uint32_t randomBelow(GameRandom& random, std::uint32_t count) { return random.next() % count; }

}

std::string pedestrianSlotName(std::size_t number) {
    char name[16];
    std::snprintf(name, sizeof name, "Pedestrian%02u", static_cast<unsigned>(number));
    return name;
}

float DistanceFadeTable::word(int index) const {
    switch (index) {
    case 0: case 1: case 2: return radius[static_cast<std::size_t>(index)];
    case 3: case 4: case 5: return fadeIn[static_cast<std::size_t>(index - 3)];
    case 6: case 7: case 8: return fadeOut[static_cast<std::size_t>(index - 6)];
    default: throw std::out_of_range("distance fade table word " + std::to_string(index));
    }
}

PedestrianSpawnWindow pedestrianSpawnWindow(const DistanceFadeTable& table, int tier) {
    if (tier < -1 || tier > 2) throw std::out_of_range("pedestrian fade tier " + std::to_string(tier));
    return {table.word(3 + tier) * kPedestrianMinDistanceScale, table.word(6 + tier) * kPedestrianMaxDistanceScale};
}

std::array<PedestrianVec3, 4> pedestrianPlacement(const PedestrianVec3& position, const PedestrianVec3& facing) {
    std::array<PedestrianVec3, 4> rows{};
    rows[2] = facing;
    rows[3] = position;
    if (std::fabs(facing[1]) <= kPlacementUpLimit) rows[0] = {facing[2], 0.0f, -facing[0]};
    else rows[0] = {0.0f, -facing[2], facing[1]};
    normalize(rows[0]);
    const PedestrianVec3& a = rows[2];
    const PedestrianVec3& b = rows[0];
    rows[1] = {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
    return rows;
}

void PedestrianManager::init() {
    flags_ = static_cast<std::uint8_t>((flags_ & ~3u) | 2u);
}

std::vector<std::size_t> PedestrianManager::loadLevel(const std::function<bool(const std::string& name)>& exists, std::uint32_t limit) {
    pedestrians_.clear();
    std::vector<std::size_t> numbers;
    for (std::size_t number = 1; number <= kPedestrianSlotCount; ++number) {
        if (!exists(pedestrianSlotName(number))) continue;
        pedestrians_.push_back({});
        numbers.push_back(number);
    }
    limit_ = limit;
    return numbers;
}

void PedestrianManager::setSpawnDataLoaded(bool loaded) {
    flags_ = static_cast<std::uint8_t>((flags_ & ~1u) | (loaded ? 1u : 0u));
}

void PedestrianManager::setEnabled(bool enabled) {
    flags_ = static_cast<std::uint8_t>((flags_ & ~2u) | (enabled ? 2u : 0u));
}

std::size_t PedestrianManager::liveCount() const {
    std::size_t live = 0;
    for (const Pedestrian& pedestrian : pedestrians_)
        if (pedestrian.live) ++live;
    return live;
}

void PedestrianManager::lowerCountdown(std::uint32_t value) {
    if (countdown_ == 0 || static_cast<std::int32_t>(countdown_) > static_cast<std::int32_t>(value)) countdown_ = value;
}

bool PedestrianManager::spawn(std::size_t slot, const PedestrianWorld& world, GameRandom& random) {
    PedestrianVec3 view = world.view.position;
    if (world.view.playerVehicleSpeed > 0.0f) {
        const float ahead = world.view.playerVehicleSpeed * kPedestrianViewAheadPerSpeed;
        for (std::size_t axis = 0; axis < 3; ++axis) view[axis] = view[axis] + world.view.row2[axis] * ahead;
    }
    if (!world.spawnPoints || world.spawnPoints->empty()) return false;
    const std::vector<PedestrianSpawnPoint>& points = *world.spawnPoints;
    const bool forward = randomBelow(random, kPedestrianDirectionRange) >= kPedestrianDirectionForwardFrom;
    const float lateral = (gameRandomUnit(random) - 0.5f) * kPedestrianLateralScale;
    const std::int32_t facingBits = floatBits(kPedestrianSpawnFacing);
    for (unsigned attempt = 0; attempt < kPedestrianSpawnTries; ++attempt) {
        const PedestrianSpawnPoint& point = points[randomBelow(random, static_cast<std::uint32_t>(points.size()))];
        if (point.path == 0) continue;
        PedestrianVec3 toPoint{point.position[0] - view[0], point.position[1] - view[1], point.position[2] - view[2]};
        const float distance = normalize(toPoint);
        const float facing = dot(toPoint, world.view.row2);
        const PedestrianSpawnWindow window = pedestrianSpawnWindow(world.fade, world.fadeTier(slot));
        if (!point.roomActive) continue;
        if (window.minimum > distance) continue;
        if (distance > window.maximum) continue;
        if (facingBits > floatBits(facing)) continue;
        if (world.spawnBlocked(slot, point.position)) continue;
        bool crowded = false;
        for (std::size_t other = 0; other < pedestrians_.size() && !crowded; ++other) {
            if (other == slot || !pedestrians_[other].live || pedestrians_[other].path != point.path) continue;
            crowded = horizontalDistanceSquared(pedestrians_[other].placement[3], point.position) < kPedestrianSpacingSquared;
        }
        if (crowded) continue;

        Pedestrian& pedestrian = pedestrians_[slot];
        pedestrian.fadeStarted = world.fadeNode(slot);
        pedestrian.requested = 1;
        pedestrian.state = 0;
        pedestrian.path = point.path;
        pedestrian.forward = forward;
        pedestrian.hold = 0;
        const float unit = gameRandomUnit(random);
        pedestrian.speed = kPedestrianSpeedBase + unit * kPedestrianSpeedRange;
        pedestrian.lateral = lateral;
        pedestrian.pathParameter = point.pathParameter;
        PedestrianVec3 direction = point.direction;
        if (!forward)
            for (float& component : direction) component = component * -1.0f;
        pedestrian.placement = pedestrianPlacement(point.position, direction);
        pedestrian.live = true;
        return true;
    }
    return false;
}

PedestrianUpdate PedestrianManager::update(const PedestrianWorld& world, GameRandom& random) {
    PedestrianUpdate result;
    if (!enabled()) {
        for (std::size_t slot = 0; slot < pedestrians_.size(); ++slot) {
            if (!pedestrians_[slot].live) continue;
            pedestrians_[slot].live = false;
            result.removed.push_back({slot, false});
        }
        return result;
    }
    std::uint32_t live = static_cast<std::uint32_t>(liveCount());
    if (!spawnDataLoaded()) return result;
    const std::uint32_t count = static_cast<std::uint32_t>(pedestrians_.size());
    if (limit_ > live && limit_ <= count) {
        countdown_ = static_cast<std::int32_t>(countdown_ - 1u) > 0 ? countdown_ - 1u : 0;
        if (countdown_ == 0) {
            const std::uint32_t slot = randomBelow(random, count);
            if (!pedestrians_[slot].live && spawn(slot, world, random)) {
                ++live;
                lowerCountdown(live * kPedestrianCountdownPerLive);
                result.spawned = slot;
            }
        }
    }
    for (std::size_t slot = 0; slot < pedestrians_.size(); ++slot) {
        Pedestrian& pedestrian = pedestrians_[slot];
        if (!pedestrian.live || pedestrian.requested == 1 || pedestrian.state == 1) continue;
        if (world.roomActive(slot)) {
            if (pedestrian.hold != 0) continue;
            const bool remove = world.onRoomRoot(slot) || world.hidden(slot) || (pedestrian.requested == 0 && pedestrian.state == 0);
            if (!remove) continue;
        }
        pedestrian.live = false;
        result.removed.push_back({slot, true});
        lowerCountdown((live - 1) * kPedestrianCountdownPerLive);
        return result;
    }
    return result;
}

}
