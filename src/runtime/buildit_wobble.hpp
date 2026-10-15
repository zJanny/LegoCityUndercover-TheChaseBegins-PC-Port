#pragma once
// The hop of the unbuilt bricks of a build-it while the player can use it: record fields +0 amplitude, +4 speed, +8 phase of each brick and the update
// 0x206ea0, with the brick set-up 0x18275c. Rules and measurements: docs/re/buildit.md. No SDL dependency.
#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

namespace chase::runtime {

constexpr float kWobbleAmplitudeBase = 0.2f;   // literal 0x207274
constexpr float kWobbleAmplitudeRange = 0.3f;  // 0x20727c
constexpr float kWobbleSpeedBase = 15.0f;      // 0x207284
constexpr float kWobbleSpeedRange = 10.0f;     // 0x207280
constexpr float kWobbleHalfTurn = 180.0f;      // 0x207278
constexpr float kWobblePitchBase = 1.1f;       // 0x207270
constexpr float kWobblePitchRange = 0.2f;      // 0x207274, subtracted
constexpr float kWobbleDegreesToRadians = 0.017453292f;  // 0x20726c
constexpr unsigned kWobbleSoundLimit = 4;      // crossings below this count make the sound

using UnitRandom = std::function<float()>;  // 0x2345dc: the low 16 bits of 0x25b2b8 times 2^-16

struct BrickWobble {
    float amplitude = 0;  // record +0
    float speed = 0;      // +4, degrees per update
    float phase = 0;      // +8, degrees
    float offset = 0;     // +0xc, the height added to the brick's node
};

struct WobbleSound {
    std::size_t brick = 0;
    float pitch = 1.0f;  // 1.1 - 0.2 r, argument of 0x22f770 for sound 5 (not played yet)
};

class BuilditWobble {
public:
    BuilditWobble() = default;
    // 0x18275c per brick in order: phase = 180 r, amplitude = 0.2 + 0.3 r, speed = 15 + 10 r.
    BuilditWobble(std::size_t bricks, const UnitRandom& random);
    explicit BuilditWobble(std::vector<BrickWobble> bricks) : bricks_(std::move(bricks)) {}

    // 0x206ea0. `hopping` is the condition (the player can use the build-it); `mayHop[i]` is false for a brick that is building or built.
    // A brick that may hop gets offset = sin(phase) x amplitude and its phase advances; a brick that passes 180 degrees draws a new amplitude and
    // speed and, for the first three of the update, a sound. Without the condition a brick that hopped goes back to offset 0.
    std::vector<WobbleSound> update(bool hopping, const std::vector<bool>& mayHop, const UnitRandom& random);

    const std::vector<BrickWobble>& bricks() const { return bricks_; }
    float offset(std::size_t brick) const { return brick < bricks_.size() ? bricks_[brick].offset : 0.0f; }

private:
    std::vector<BrickWobble> bricks_;
    bool hopped_ = false;  // record +0x36 bit 0
};

}
