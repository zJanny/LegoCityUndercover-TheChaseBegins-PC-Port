#include "runtime/super_brick.hpp"

#include <algorithm>
#include <cmath>

namespace chase::runtime {

std::array<float, 16> superBrickMatrix(const std::array<float, 3>& position, std::uint32_t clockTicks) {
    const float angle = (float(clockTicks) / kSuperBrickClockTicksPerSecond) * kSuperBrickSpinPerSecond;
    const float s = std::sin(angle), c = std::cos(angle);
    return {c, 0, -s, 0, 0, 1, 0, 0, s, 0, c, 0, position[0], position[1], position[2], 1};
}

void SuperBrickTint::update(float dt) {
    const float x = phase_ * float(kSuperBrickTintEntries);
    const unsigned index = (x > 0.0f ? unsigned(x) : 0u) % kSuperBrickTintEntries;
    const unsigned next = (index + 1) % kSuperBrickTintEntries;
    const float mix = x - float(index);
    for (std::size_t channel = 0; channel < 3; ++channel) {
        const float from = float(kSuperBrickTintTable[index][channel]) * 0.003921569f;
        const float to = float(kSuperBrickTintTable[next][channel]) * 0.003921569f;
        const int value = int((from + (to - from) * mix) * 255.0f);
        bytes_[channel] = std::uint8_t(std::clamp(value, 0, 255));
    }
    phase_ += dt * kSuperBrickTintPerSecond;
    if (phase_ > 1.0f) phase_ -= 1.0f;
}

float superBrickAlpha(float timer) {
    const int alpha = int(timer * 255.0f);
    return float(std::clamp(alpha, 0, 255)) / 255.0f;
}

void SuperBrickField::add(const std::string& instance, const std::array<float, 3>& position, std::uint16_t value) {
    SuperBrick brick;
    brick.instance = instance;
    brick.position = position;
    brick.value = value;
    bricks_.push_back(std::move(brick));
}

std::size_t SuperBrickField::count(SuperBrickState state) const {
    return std::size_t(std::count_if(bricks_.begin(), bricks_.end(), [state](const SuperBrick& brick) { return brick.state == state; }));
}

void SuperBrickField::updateOne(std::size_t index, float dt, const std::array<float, 3>& playerPosition, bool blocked,
                                const std::optional<std::array<float, 3>>& flightTarget, SuperBrickStep& step) {
    SuperBrick& brick = bricks_[index];
    if (brick.state == SuperBrickState::Idle) {
        if (blocked || brick.timer > 0.0f) return;
        const float dx = brick.position[0] - playerPosition[0], dy = brick.position[1] - playerPosition[1], dz = brick.position[2] - playerPosition[2];
        const float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
        if (!(distance < kSuperBrickPickupDistance)) return;
        brick.state = SuperBrickState::Collected;
        brick.timer = kSuperBrickCollectTime;
        step.collected.push_back({index});
        return;
    }
    if (brick.state != SuperBrickState::Collected) return;
    brick.timer -= dt;
    if (0.0f < brick.timer) {
        if (flightTarget) {
            const float rate = 1.0f / (kStudFlightRate * brick.timer);
            for (std::size_t axis = 0; axis < 3; ++axis) brick.position[axis] += ((*flightTarget)[axis] - brick.position[axis]) * rate;
        }
        return;
    }
    brick.state = SuperBrickState::Done;
    total_ += brick.value;
    SuperBrickBanked banked;
    banked.index = index;
    banked.value = brick.value;
    banked.sound = brick.value >= kSuperBrickBigValue ? kSuperBrickSoundBig : kSuperBrickSoundSmall;
    banked.total = total_;
    step.banked.push_back(banked);
}

}
