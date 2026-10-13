#pragma once
// Pad input pipeline of the game (docs/re/arm-executable.md, 2026-10-13 entries): raw 3DS pad
// state -> circle clamp -> 20-byte channels -> deadzones -> press/release edges -> slot table.
// No SDL dependency.
#include <array>
#include <cstdint>
#include <utility>

namespace chase::runtime {

// HID button bits as the game reads them (0x10a4dc).
enum PadButton : std::uint32_t {
    PadA = 0x1, PadB = 0x2, PadSelect = 0x4, PadStart = 0x8, PadRight = 0x10, PadLeft = 0x20,
    PadUp = 0x40, PadDown = 0x80, PadR = 0x100, PadL = 0x200, PadX = 0x400, PadY = 0x800,
};

struct PadState {
    std::uint32_t held = 0;
    std::int16_t stickX = 0, stickY = 0;
};

// Channel record {f32 value, u8 type, f32 deadzone, f32 threshold, s16 press, s16 release}.
// Defaults read from the running game (oracle `channels`, 2026-10-13): every channel 0.1 / 0.75.
struct Channel {
    float value = 0;
    std::uint8_t type = 0;
    float deadzone = 0.1f;
    float threshold = 0.75f;
    std::int16_t press = 0;
    std::int16_t release = 0;
};

constexpr std::size_t kChannelCount = 42;

// Circle clamp 0x119724 on the raw circle-pad pair: zero inside `min`, length reduced by `min`
// and capped at max - min, in the game's fixed-point integer arithmetic.
std::pair<std::int16_t, std::int16_t> circleClamp(int x, int y, int min, int max);
// 0x10fcb0: clamped raw axis / range, capped at +1 (the negative side is not capped).
float axisValue(int raw, int range);
// 0x1035d0: dead zone remap clamp((v -+ dz) / (1 - dz)) towards zero.
float deadzoneAxis(float value, float deadzone);

// Pad device 0x168c9c with the per-frame poll 0x25cb2c (mode 1: pad), writer 0x103dc8 and edges 0x25b4b4.
class PadDevice {
public:
    PadDevice();
    void poll(const PadState& state);
    const Channel& channel(std::size_t index) const { return channels_.at(index); }
    Channel& channel(std::size_t index) { return channels_.at(index); }
    // Slot table 0x3769e8: slot index -> channel index.
    const Channel& slot(std::size_t index) const;

    int stickMin = 40, stickMax = 145;

private:
    std::array<Channel, kChannelCount> channels_;
    std::array<float, kChannelCount> previous_{};
};

constexpr std::array<std::uint8_t, 18> kSlotChannels = {12, 13, 14, 15, 16, 17, 16, 17, 18, 19, 6, 7, 8, 10, 0, 1, 2, 3};

}
