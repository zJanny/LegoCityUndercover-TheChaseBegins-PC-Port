#include "runtime/buildit_wobble.hpp"

#include <cmath>

namespace chase::runtime {

BuilditWobble::BuilditWobble(std::size_t bricks, const UnitRandom& random) : bricks_(bricks) {
    for (BrickWobble& brick : bricks_) {
        brick.phase = random() * kWobbleHalfTurn;
        brick.amplitude = kWobbleAmplitudeBase + random() * kWobbleAmplitudeRange;
        brick.speed = kWobbleSpeedBase + random() * kWobbleSpeedRange;
    }
}

std::vector<WobbleSound> BuilditWobble::update(bool hopping, const std::vector<bool>& mayHop, const UnitRandom& random) {
    std::vector<WobbleSound> sounds;
    unsigned crossings = 0;
    for (std::size_t i = 0; i < bricks_.size(); ++i) {
        BrickWobble& brick = bricks_[i];
        if (i >= mayHop.size() || !mayHop[i]) {
            brick.offset = 0;
            continue;
        }
        if (!hopping) {
            if (hopped_) brick.offset = 0;
            continue;
        }
        brick.offset = std::sin(brick.phase * kWobbleDegreesToRadians) * brick.amplitude;
        brick.phase += brick.speed;
        if (!(brick.phase > kWobbleHalfTurn)) continue;
        ++crossings;
        if (crossings < kWobbleSoundLimit) sounds.push_back({i, kWobblePitchBase - random() * kWobblePitchRange});
        brick.phase -= kWobbleHalfTurn;
        brick.amplitude = kWobbleAmplitudeBase + random() * kWobbleAmplitudeRange;
        brick.speed = kWobbleSpeedBase + random() * kWobbleSpeedRange;
    }
    hopped_ = hopping;
    return sounds;
}

}
