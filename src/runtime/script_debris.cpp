#include "runtime/script_debris.hpp"

#include <cmath>

namespace chase::runtime {

std::uint8_t scriptDebrisByte(float value) {
    if (std::isnan(value)) return 0;
    if (value >= 2147483648.0f) return 0xff;
    if (value <= -2147483648.0f) return 0;
    return static_cast<std::uint8_t>(std::uint32_t(std::int32_t(value)) & 0xffu);
}

std::uint32_t scriptDebrisColour(float r, float g, float b) {
    return std::uint32_t(scriptDebrisByte(r)) | std::uint32_t(scriptDebrisByte(g)) << 8 | std::uint32_t(scriptDebrisByte(b)) << 16 | 0xff000000u;
}

DebrisBurst scriptDebrisBurst(const PropDestroyEffects& classEffects, const DebrisVec3& position, std::uint32_t colour, std::optional<std::uint32_t> secondColour) {
    DebrisBurst burst = propDebrisBurst(classEffects, position);
    if (!classEffects.hasColour) burst.colour = colour;
    if (!classEffects.hasSecondColour) burst.secondColour = secondColour.value_or(kDebrisNoSecondColour);
    return burst;
}

}
