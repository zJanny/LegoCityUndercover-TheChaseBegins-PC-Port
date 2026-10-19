#include "glow.hpp"

namespace chase::runtime {

float glowBlurOffsetTexels(int iteration) { return 4.5f - float(iteration) * 1.5f; }

float glowBlurWeight(int iteration) {
    const float weight = 1.5f - float(iteration) * 0.28f;
    return weight > 1.0f ? weight : 1.0f;
}

std::uint8_t glowBlurWeightAlphaByte(int iteration) { return std::uint8_t(int(glowBlurWeight(iteration) * 128.0f + 0.5f)); }

bool glowRuns(float strength, bool flag1a3888) { return strength != 0.0f || flag1a3888; }

namespace {
std::uint8_t strengthByte(float value) {
    const int byte = int(value * 255.0f + 0.5f);
    return std::uint8_t(byte < 0 ? 0 : byte > 0xff ? 0xff : byte);
}
}

GlowComposite glowComposite(float strength, float second) {
    GlowComposite out;
    if (strength + second > 1.0f) {
        strength *= 0.5f;
        second *= 0.5f;
        out.doubled = true;
    }
    out.colour = strengthByte(strength);
    out.alpha = strengthByte(second);
    return out;
}

}
