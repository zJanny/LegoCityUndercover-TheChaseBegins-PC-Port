#include "runtime/pad.hpp"
#include <cmath>
#include <cstring>

namespace chase::runtime {
namespace {
int integerSqrt(int value) {
    if (value <= 0) return 0;
    int shifted = value, guess = 1;
    if (value > 1)
        do {
            guess <<= 1;
            shifted >>= 1;
        } while (shifted > guess);
    int root, next = guess;
    do {
        root = next;
        next = (value / root + root) >> 1;
    } while (next < root);
    return root;
}

bool aboveOne(float value) {
    std::int32_t bits;
    std::memcpy(&bits, &value, sizeof bits);
    return bits > 0x3f800000;
}

bool belowMinusOne(float value) {
    std::uint32_t bits;
    std::memcpy(&bits, &value, sizeof bits);
    return bits > 0xbf800000u;
}

void radialDeadzone(Channel& x, Channel& y, float deadzone) {
    float squared = x.value * x.value + y.value * y.value;
    if (!(0.0f < squared)) return;
    float length = std::sqrt(squared);
    float scale = deadzoneAxis(length, deadzone) / length;
    x.value = scale * x.value;
    y.value = scale * y.value;
}
}

std::pair<std::int16_t, std::int16_t> circleClamp(int x, int y, int min, int max) {
    int squared = y * y + x * x;
    if (squared <= min * min) return {0, 0};
    int length = integerSqrt(squared << 14);
    int scaled = max * max <= squared ? (max - min) << 7 : length - (min << 7);
    if (length == 0) return {0, 0};
    return {std::int16_t(x * scaled / length), std::int16_t(y * scaled / length)};
}

float axisValue(int raw, int range) {
    if (raw == 0) return 0.0f;
    if (range <= raw) return 1.0f;
    return static_cast<float>(raw) / static_cast<float>(range);
}

float deadzoneAxis(float value, float deadzone) {
    float remapped = 0.0f;
    if (value <= 0.0f) {
        float r = (value + deadzone) / (1.0f - deadzone);
        if (0.0f < r) return 0.0f;
        remapped = r;
    } else {
        float r = (value - deadzone) / (1.0f - deadzone);
        if (r < 0.0f) return 0.0f;
        remapped = r;
    }
    if (belowMinusOne(remapped)) return -1.0f;
    if (aboveOne(remapped)) return 1.0f;
    return remapped;
}

PadDevice::PadDevice() = default;

const Channel& PadDevice::slot(std::size_t index) const { return channels_.at(kSlotChannels.at(index)); }

void PadDevice::poll(const PadState& state) {
    for (std::size_t i = 0; i < kChannelCount; ++i) {
        previous_[i] = channels_[i].value;
        channels_[i].value = 0;
        channels_[i].press = 0;
        channels_[i].release = 0;
    }
    const std::uint32_t held = state.held & ~0x2000u;
    const std::pair<std::uint32_t, std::size_t> buttons[] = {
        {PadUp, 12}, {PadDown, 13}, {PadLeft, 14}, {PadRight, 15}, {PadStart, 6}, {PadA, 16},
        {PadB, 17}, {PadX, 18}, {PadY, 19}, {PadL, 8}, {PadR, 10},
    };
    for (const auto& [bit, index] : buttons) channels_[index].value = (held & bit) ? 1.0f : 0.0f;
    auto [x, y] = circleClamp(state.stickX, state.stickY, stickMin, stickMax);
    const int range = static_cast<std::int16_t>(stickMax - stickMin);
    channels_[0].value = axisValue(x, range);
    channels_[1].value = axisValue(y, range);
    channels_[2].value = 0;
    channels_[3].value = 0;

    radialDeadzone(channels_[0], channels_[1], channels_[0].deadzone);
    radialDeadzone(channels_[2], channels_[3], channels_[2].deadzone);
    for (std::size_t i = 4; i < 24; ++i) channels_[i].value = deadzoneAxis(channels_[i].value, channels_[i].deadzone);

    for (std::size_t i = 0; i < kChannelCount; ++i) {
        Channel& c = channels_[i];
        const float v = c.value, t = c.threshold, p = previous_[i];
        if (t < v && p <= t) c.press = 1;
        if (v < -t && -t <= p) c.press = -1;
        if (v < t && t <= p) c.release = 1;
        if (-t < v && p <= -t) c.release = -1;
    }
}

}
