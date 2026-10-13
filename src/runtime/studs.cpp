#include "runtime/studs.hpp"
#include <algorithm>
#include <cmath>

namespace chase::runtime {

unsigned studSpriteFrame(std::uint32_t clockTicks) {
    const std::uint32_t high = std::uint32_t((std::uint64_t(clockTicks) * 0x51eb851fu) >> 32);
    return (high >> 6) & 15;
}

std::array<float, 3> studPickupPoint(float x, float y, float z, float heading) {
    return {x + kMeasuredPlayerPickupPoint[2] * std::sin(heading), y + kMeasuredPlayerPickupPoint[1], z + kMeasuredPlayerPickupPoint[2] * std::cos(heading)};
}

std::array<float, 3> unprojectTopScreen(float screenX, float screenY, float depth, const float eye[3], float yaw, float pitch,
                                        float verticalFov) {
    const float scaleY = 1.0f / std::tan(verticalFov * 0.5f), scaleX = scaleY / (kTopScreenWidth / kTopScreenHeight);
    const float viewX = (screenX / (kTopScreenWidth * 0.5f) - 1.0f) * depth / scaleX;
    const float viewY = (1.0f - screenY / (kTopScreenHeight * 0.5f)) * depth / scaleY;
    const float forward[3] = {std::cos(pitch) * std::sin(yaw), std::sin(pitch), std::cos(pitch) * std::cos(yaw)};
    const float right[3] = {std::cos(yaw), 0.0f, -std::sin(yaw)};
    const float up[3] = {forward[1] * right[2] - forward[2] * right[1], forward[2] * right[0] - forward[0] * right[2],
                         forward[0] * right[1] - forward[1] * right[0]};
    std::array<float, 3> world{};
    for (int axis = 0; axis < 3; ++axis) world[std::size_t(axis)] = eye[axis] + viewX * right[axis] + viewY * up[axis] + depth * forward[axis];
    return world;
}

StudField::StudField(const std::vector<StudSpawn>& spawns) {
    studs_.reserve(spawns.size());
    for (const StudSpawn& spawn : spawns) {
        Stud stud;
        stud.kind = spawn.kind;
        stud.group = spawn.group;
        stud.state = spawn.group ? StudState::Hidden : StudState::Idle;
        stud.centre = {spawn.position[0], spawn.position[1] + kStudCentreHeight, spawn.position[2]};
        studs_.push_back(stud);
    }
}

std::vector<StudCollected> StudField::update(float dt, const std::array<float, 3>& pickupPoint) {
    std::vector<StudCollected> collected;
    for (std::size_t index = 0; index < studs_.size(); ++index) {
        Stud& stud = studs_[index];
        if (stud.state == StudState::Idle) {
            if (--stud.skip >= 1) continue;
            const float dx = stud.centre[0] - pickupPoint[0], dy = stud.centre[1] - pickupPoint[1], dz = stud.centre[2] - pickupPoint[2];
            const float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
            if (kMeasuredStudPickupRadius > distance) {
                stud.state = StudState::Collecting;
                stud.timer = kStudCollectSeconds;
                collected.push_back({index, stud.kind, kStudCollectSounds[std::size_t(stud.kind)]});
            } else {
                stud.skip = std::min(int((distance - kMeasuredStudPickupRadius) / kStudSkipUnit), kStudSkipLimit);
            }
        } else if (stud.state == StudState::Collecting) {
            stud.timer -= dt;
            if (stud.timer > 0) {
                if (flightTarget_) {
                    const float step = 1.0f / (kStudFlightRate * stud.timer);
                    for (int axis = 0; axis < 3; ++axis) stud.centre[axis] += ((*flightTarget_)[axis] - stud.centre[axis]) * step;
                }
            } else {
                stud.state = StudState::Done;
                counter_ = std::min(counter_ + kStudValues[std::size_t(stud.kind)], kStudCounterLimit);
                banked_.push_back({stud.kind, counter_});
            }
        }
    }
    return collected;
}

std::vector<StudBanked> StudField::takeBanked() {
    std::vector<StudBanked> taken;
    taken.swap(banked_);
    return taken;
}

std::size_t StudField::count(StudState state) const {
    return std::size_t(std::count_if(studs_.begin(), studs_.end(), [&](const Stud& stud) { return stud.state == state; }));
}

}
