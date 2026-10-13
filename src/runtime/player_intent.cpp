#include "runtime/player_intent.hpp"
#include <cmath>
#include <cstring>

namespace chase::runtime {
namespace {
constexpr std::uint32_t kSign = 0x80000000u;
constexpr std::uint32_t kInfinity = 0x7f800000u;

std::uint32_t bitsOf(float value) {
    std::uint32_t bits;
    std::memcpy(&bits, &value, sizeof bits);
    return bits;
}

float floatOf(std::uint32_t bits) {
    float value;
    std::memcpy(&value, &bits, sizeof value);
    return value;
}

// vcvt.s32.f32: round toward zero, saturating, NaN to zero.
std::int32_t convertToInt(float value) {
    if (std::isnan(value)) return 0;
    if (value >= 2147483648.0f) return INT32_MAX;
    if (value <= -2147483648.0f) return INT32_MIN;
    return static_cast<std::int32_t>(value);
}

float absolute(float value) { return floatOf(bitsOf(value) & ~kSign); }

float headingScale() { return floatOf(0x4622f983u); }
}

float gameAtan2(float first, float second) {
    float s0 = first, s1 = second;
    std::uint32_t r0 = bitsOf(first), r1 = bitsOf(second);

    constexpr std::uint32_t offset = 0xe8000000u, limit = 0xe5000000u;
    const bool inRange = offset + (r0 << 1) < limit && offset + (r1 << 1) < limit;
    if (!inRange) {
        const std::uint32_t first2 = r0 << 1, second2 = r1 << 1;
        if (first2 > 0xff000000u || second2 > 0xff000000u) return first + second;
        if (((r0 | r1) << 1) == 0) {
            r1 |= kInfinity;
            s1 = floatOf(r1);
        } else if (first2 == 0xff000000u && second2 == 0xff000000u) {
            r0 &= ~0x40000000u;
            r1 &= ~0x40000000u;
            s0 = floatOf(r0);
            s1 = floatOf(r1);
        } else if (first2 == 0xff000000u || second2 == 0) {
            r0 |= kInfinity;
            r1 &= kSign;
        } else if (((second2 ^ first2) & kSign) == 0) {
            const float scale = static_cast<std::int32_t>(second2) >= 0 ? floatOf(0x4f800000u) : floatOf(0x2f800000u);
            s0 = s0 * scale;
            s1 = s1 * scale;
            r0 = bitsOf(s0);
            r1 = bitsOf(s1);
        }
    }

    const std::uint32_t firstShifted = r0 << 1, secondShifted = r1 << 1;
    const std::int32_t exponentDifference =
        static_cast<std::int32_t>(firstShifted >> 24) - static_cast<std::int32_t>(secondShifted >> 24);
    if (exponentDifference > 27) return floatOf((r0 & kSign) ? 0xbfc90fdbu : 0x3fc90fdbu);
    if (exponentDifference < -26) {
        if ((r1 & kSign) == 0) return s0 / s1;
        return floatOf((r0 & kSign) ? 0xc0490fdbu : 0x40490fdbu);
    }

    float s2, s3;
    if (firstShifted > secondShifted) {
        const bool negative = (r0 & kSign) != 0;
        s3 = floatOf(negative ? 0xbfc90000u : 0x3fc90000u);
        s2 = floatOf(negative ? 0xb9fdaa22u : 0x39fdaa22u);
        const std::uint32_t newSecond = r0 ^ kSign;
        const float newFirst = s1;
        s1 = -s0;
        s0 = newFirst;
        r0 = r1;
        r1 = newSecond;
    } else if ((r1 & kSign) == 0) {
        s2 = 0.0f;
        s3 = 0.0f;
    } else {
        const bool negative = (r0 & kSign) != 0;
        s3 = floatOf(negative ? 0xc0490000u : 0x40490000u);
        s2 = floatOf(negative ? 0xba7daa22u : 0x3a7daa22u);
    }

    float s4;
    if (((r1 - r0) << 1) >= 0x01000000u) {
        s4 = s0 / s1;
    } else {
        const float lowAtanHalf = floatOf(0x37ce0ac3u), highAtanHalf = floatOf(0x3eed6000u);
        const bool signsDiffer = ((r0 ^ r1) & kSign) != 0;
        s4 = floatOf(signsDiffer ? 0xbf000000u : 0x3f000000u);
        if (signsDiffer) {
            s2 = s2 - lowAtanHalf;
            s3 = s3 - highAtanHalf;
        } else {
            s2 = s2 + lowAtanHalf;
            s3 = s3 + highAtanHalf;
        }
        float numerator = s0;
        float product = s4 * s1;
        numerator = numerator - product;
        float denominator = s1;
        product = s0 * s4;
        denominator = denominator + product;
        s4 = numerator / denominator;
    }

    const float squared = s4 * s4;
    float s5 = floatOf(0x3dd5b88fu);
    float product = squared * floatOf(0xbd65ad2du);
    s5 = s5 + product;
    const float cubed = s4 * squared;
    float s7 = floatOf(0xbe11b50fu);
    product = squared * s5;
    s7 = s7 + product;
    float s6 = floatOf(0x3e4cc861u);
    product = squared * s7;
    s6 = s6 + product;
    float s1Poly = floatOf(0xbeaaaaa8u);
    product = squared * s6;
    s1Poly = s1Poly + product;
    product = cubed * s1Poly;
    s2 = s2 + product;
    float result = s2 + s4;
    result = result + s3;
    return result;
}

StickIntent stickIntent(const StickIntentInput& input) {
    StickIntent intent;
    const bool active = absolute(input.slot14.value) > input.slot14.deadzone ||
                        input.slot15.deadzone < absolute(input.slot15.value);
    if (input.byte372d44 != 0) {
        intent.heldClear = 0x3;
        intent.pressClear = 0x3;
        return intent;
    }
    if (!active) return intent;
    intent.active = true;
    intent.heldSet = kHeldMove;

    float x = input.slot14.value, y = input.slot15.value;
    if (input.entityType != 0x14b && input.entityType != 0x189) {
        x = std::sqrt(absolute(x)) * x;
        y = std::sqrt(absolute(y)) * y;
    }
    const std::uint16_t stickHeading = static_cast<std::uint16_t>(convertToInt(gameAtan2(x, y) * headingScale()));

    const float rawX = absolute(input.slot14.value), rawY = absolute(input.slot15.value);
    float squaredLength = rawX * rawX;
    const float squaredY = rawY * rawY;
    squaredLength = squaredLength + squaredY;
    if (static_cast<std::int32_t>(bitsOf(squaredLength)) < 0x3f000000) intent.heldSet |= kHeldSlow;

    const std::int32_t cameraHeading = convertToInt(input.cameraYaw * headingScale());
    intent.heading = static_cast<std::int16_t>(static_cast<std::uint16_t>(cameraHeading + stickHeading));
    return intent;
}

namespace {
struct Reduced {
    float value = 0;
    int quadrant = 0;     // n & 3, or -1 for |x| < 2^-12
    bool large = false;
};

Reduced reduceAngle(float x) {
    Reduced reduced;
    const std::uint32_t bits = bitsOf(x);
    if (!(0x7e921fb6u <= (bits << 1))) {
        reduced.value = x;
        reduced.quadrant = (bits << 1) < 0x73000000u ? -1 : 0;
        return reduced;
    }
    if (0x46490e49u <= (bits & ~kSign)) {
        reduced.large = true;
        return reduced;
    }
    const float scaled = x * floatOf(0x3f22f983u);
    const float magic = floatOf(0x4b000000u);
    float rounded;
    if ((bits & kSign) == 0) {
        rounded = (scaled + magic) - magic;
    } else {
        const float shifted = scaled - magic;
        rounded = shifted + magic;
    }
    reduced.quadrant = convertToInt(rounded) & 3;
    float value = x;
    value = value - rounded * floatOf(0x3fc90000u);
    value = value - rounded * floatOf(0x39fda000u);
    value = value - rounded * floatOf(0x33a22000u);
    value = value - rounded * floatOf(0x2c34611au);
    reduced.value = value;
    return reduced;
}

float sinPolynomial(float x, float squared) {
    float s0 = floatOf(0x3c0882dau);
    s0 = s0 - squared * floatOf(0x394c6d33u);
    float s2 = floatOf(0xbe2aaaa0u);
    s2 = s2 + squared * s0;
    return x + x * (s2 * squared);
}

float cosPolynomial(float squared) {
    float s2 = floatOf(0x3d2a9fcau);
    s2 = s2 + squared * floatOf(0xbab23ab9u);
    float s3 = floatOf(0xbeffffddu);
    s3 = s3 + squared * s2;
    return 1.0f + squared * s3;
}
}

float gameSin(float angle) {
    const Reduced r = reduceAngle(angle);
    if (r.large) return std::sin(angle);
    if (r.quadrant < 0) return r.value;
    const float squared = r.value * r.value;
    const float result = (r.quadrant & 1) ? cosPolynomial(squared) : sinPolynomial(r.value, squared);
    return (r.quadrant & 2) ? -result : result;
}

float gameCos(float angle) {
    const Reduced r = reduceAngle(angle);
    if (r.large) return std::cos(angle);
    if (r.quadrant < 0) return 1.0f;
    const float squared = r.value * r.value;
    const float result = (r.quadrant & 1) ? sinPolynomial(r.value, squared) : cosPolynomial(squared);
    const bool negate = (r.quadrant & 1) ? !(r.quadrant & 2) : (r.quadrant & 2) != 0;
    return negate ? -result : result;
}

}
