#include "audio/voice_params.hpp"
#include <cmath>
#include <cstring>
#include <limits>

namespace chase::audio {
namespace {
float floatOfBits(std::uint32_t bits) {
    float value;
    std::memcpy(&value, &bits, sizeof value);
    return value;
}

const float kPercentToVolume = floatOfBits(0x3c23d70a);
const float kSpeedOfSoundReciprocal = floatOfBits(0x3b3ec9f8);
const float kMinimumDoppler = floatOfBits(0x3a83126f);
constexpr float kMinimumDirectionLength = 1.1920929e-7f;
} // namespace

float initialVoiceVolume(std::uint32_t percent) { return float(std::int32_t(percent)) * kPercentToVolume; }

std::uint32_t boostedRate(std::uint32_t sourceRate) { return sourceRate + (sourceRate >> 1); }

std::int32_t scaledRate(float scale, std::uint32_t sourceRate, bool boost) {
    const float rate = float(boost ? boostedRate(sourceRate) : sourceRate);
    const float scaled = scale * rate;
    if (std::isnan(scaled)) return 0;
    if (scaled >= 2147483648.0f) return std::numeric_limits<std::int32_t>::max();
    if (scaled <= -2147483648.0f) return std::numeric_limits<std::int32_t>::min();
    return std::int32_t(scaled);
}

std::int32_t storedRate(std::int32_t requested, std::uint32_t sourceRate) {
    return requested == kResetRate ? std::int32_t(sourceRate) : requested;
}

float pitchOfRate(std::int32_t rate, float doppler, std::uint32_t sourceRate) {
    const float pitch = float(rate) * doppler / float(sourceRate);
    return pitch >= 0.0f ? pitch : 0.0f;
}

float dopplerFactor(const std::array<float, 3>& emitterVelocity, const std::array<float, 3>& listenerVelocity,
                    const std::array<float, 3>& direction, float scale) {
    std::array<float, 3> unit = {0.0f, 0.0f, 1.0f};
    const float length = std::sqrt(direction[0] * direction[0] + direction[1] * direction[1] + direction[2] * direction[2]);
    if (length > kMinimumDirectionLength) {
        const float inverse = 1.0f / length;
        unit = {direction[0] * inverse, direction[1] * inverse, direction[2] * inverse};
    }
    const float along = (emitterVelocity[0] - listenerVelocity[0]) * unit[0] + (emitterVelocity[1] - listenerVelocity[1]) * unit[1] +
                        (emitterVelocity[2] - listenerVelocity[2]) * unit[2];
    const float factor = 1.0f - along * kSpeedOfSoundReciprocal * scale;
    return factor < kMinimumDoppler ? kMinimumDoppler : factor;
}

} // namespace chase::audio
