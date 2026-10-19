#include "runtime/fall_apart.hpp"

#include <cmath>
#include <cstring>

namespace chase::runtime {
namespace {

// 0x1d3f54 / 0x1d3f58: added to and then subtracted from the matrix's translation y.
constexpr float kHeadDrop = -0.6f;
constexpr float kPartLift = -1.4f;
// 0x1d3fa4..0x1d3fb0 (s17, s20, s21, s16 of 0x1d3b50).
constexpr float kDrawOffset = 0.5f;
constexpr float kStartSpread = 2.0f;
constexpr float kStartRise = 2.0f;
constexpr float kStartScale = 0.1f;
constexpr float kStartSpinDivisor = 4.0f;
// 0x1d2a70 / 0x1d2a74 and the immediate 0x3e000000 (0.125 compared as an integer) of 0x1d27f4.
constexpr float kRestSpeed = 2.0f;
constexpr std::int32_t kRestDriftBits = 0x3e000000;
constexpr float kBounceSpinDivisor = 2.0f;
constexpr float kReflect = -2.0f;

float dot(const DebrisVec3& a, const DebrisVec3& b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }

float length(const DebrisVec3& v) { return std::sqrt(dot(v, v)); }

// 0x23b998: the unit vector, (0, 0, 1) for a length of 2^-23 or less.
DebrisVec3 unitOf(const DebrisVec3& v) {
    const float l = length(v);
    if (l <= 1.0f / 8388608.0f) return {0, 0, 1};
    const float inverse = 1.0f / l;
    return {v[0] * inverse, v[1] * inverse, v[2] * inverse};
}

std::int32_t bitsOf(float value) {
    std::int32_t bits = 0;
    std::memcpy(&bits, &value, 4);
    return bits;
}

// 0x250da8: the point through the matrix.
DebrisVec3 transformed(const DebrisVec3& p, const FallApartMatrix& m) {
    DebrisVec3 out{};
    for (unsigned i = 0; i < 3; ++i) out[i] = p[0] * m[i] + p[1] * m[4 + i] + p[2] * m[8 + i] + m[12 + i];
    return out;
}

void translate(FallApartMatrix& m, const DebrisVec3& by) {
    for (unsigned i = 0; i < 3; ++i) m[12 + i] += by[i];
}

FallApartMatrix multiply(const FallApartMatrix& a, const FallApartMatrix& b) {
    FallApartMatrix out{};
    for (unsigned row = 0; row < 4; ++row)
        for (unsigned column = 0; column < 4; ++column) {
            float sum = 0;
            for (unsigned k = 0; k < 4; ++k) sum += a[row * 4 + k] * b[k * 4 + column];
            out[row * 4 + column] = sum;
        }
    return out;
}

// 0x1d2a08..0x1d2a64: T(-centre) * rotation(spin) * T(centre) * matrix (0x24d434, 0x227790, 0x237c48, 0x2588f4, 0x258c9c, 0x258a10).
FallApartMatrix turned(const FallApartMatrix& m, const DebrisVec3& centre, const DebrisVec3& spin) {
    FallApartMatrix toCentre{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, -centre[0], -centre[1], -centre[2], 1};
    const std::array<float, 9> r = debrisRotation(spin[0], spin[1], spin[2]);
    const FallApartMatrix rotation{r[0], r[1], r[2], 0, r[3], r[4], r[5], 0, r[6], r[7], r[8], 0, 0, 0, 0, 1};
    const FallApartMatrix back{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, centre[0], centre[1], centre[2], 1};
    return multiply(multiply(multiply(toCentre, rotation), back), m);
}

float spinDraw(GameRandom& random, float divisor) { return (gameRandomUnit(random) - kDrawOffset) / divisor; }

}

bool fallApartStep(FallApartBody& body, const FallApartBounds& bounds, float bounce, float gravity, const DebrisLineTest& test, GameRandom& random) {
    FallApartMatrix& m = body.matrix;
    DebrisVec3& v = body.velocity;
    const float radius = bounds.half[0];
    const DebrisVec3 centre = transformed(bounds.centre, m);
    const float speed = length(v);
    const DebrisVec3 unit = unitOf(v);
    const DebrisVec3 lead{centre[0] + unit[0] * radius, centre[1] + unit[1] * radius, centre[2] + unit[2] * radius};
    DebrisVec3 to{};
    for (unsigned i = 0; i < 3; ++i) to[i] = (unit[i] * radius + v[i]) + centre[i];
    const std::optional<DebrisLineHit> hit = test(centre, to);
    if (!hit) {
        translate(m, v);
    } else {
        const DebrisVec3 toContact{hit->point[0] - lead[0], hit->point[1] - lead[1], hit->point[2] - lead[2]};
        float travel = speed - length(toContact);
        translate(m, toContact);
        const float along = dot(hit->normal, v) * kReflect;
        for (unsigned i = 0; i < 3; ++i) v[i] = (v[i] + hit->normal[i] * along) * bounce;
        travel *= bounce;
        if (speed < kRestSpeed && bitsOf(std::fabs(v[0])) < kRestDriftBits && bitsOf(std::fabs(v[2])) < kRestDriftBits) return true;
        DebrisVec3 onward = unitOf(v);
        for (float& c : onward) c *= travel;
        translate(m, onward);
        const float first = spinDraw(random, kBounceSpinDivisor);
        const float second = spinDraw(random, kBounceSpinDivisor);
        const float third = spinDraw(random, kBounceSpinDivisor);
        body.spin = {third, second, first};
    }
    v[1] -= gravity;
    m = turned(m, bounds.centre, body.spin);
    return false;
}

FallApartMatrix fallApartCarried(const FallApartMatrix& childWorld, const FallApartMatrix& oldParent, const FallApartMatrix& newParent) {
    FallApartMatrix inverse{};
    for (unsigned row = 0; row < 3; ++row)
        for (unsigned column = 0; column < 3; ++column) inverse[row * 4 + column] = oldParent[column * 4 + row];
    for (unsigned column = 0; column < 3; ++column)
        inverse[12 + column] = -(oldParent[12] * inverse[column] + oldParent[13] * inverse[4 + column] + oldParent[14] * inverse[8 + column]);
    inverse[15] = 1;
    return multiply(multiply(childWorld, inverse), newParent);
}

std::optional<unsigned> FallApartPool::start(const std::string& owner, const FallApartMatrix& world, const std::optional<FallApartBounds>& headBounds) {
    unsigned index = 0;
    while (index < kFallApartSlots && slots_[index].inUse) ++index;
    if (index == kFallApartSlots) return std::nullopt;
    FallApartSlot& slot = slots_[index];
    slot.lowerPart = FallApartPart(3 + random_.next() % 2);
    slot.headBounds = headBounds.value_or(partBounds_[unsigned(FallApartPart::Head)]);
    slot.inUse = true;
    slot.owner = owner;
    FallApartMatrix m = world;
    m[13] += kHeadDrop;
    slot.parts[unsigned(FallApartPart::Head)].matrix = m;
    m[13] -= kPartLift;
    slot.parts[unsigned(FallApartPart::Body)].matrix = m;
    slot.parts[unsigned(FallApartPart::Arm)].matrix = m;
    slot.parts[unsigned(slot.lowerPart)].matrix = m;
    slot.phase = FallApartPhase::Flying;
    slot.counter = kFallApartPhaseUpdates;
    for (unsigned part = 0; part < kFallApartParts; ++part) {
        if (!partUsed(slot, part)) continue;
        FallApartBody& body = slot.parts[part];
        body.enabled = true;
        body.velocity[0] = (gameRandomUnit(random_) - kDrawOffset) * kStartSpread;
        body.velocity[1] = kStartRise;
        body.velocity[0] = (gameRandomUnit(random_) - kDrawOffset) * kStartSpread;
        for (float& c : body.velocity) c *= kStartScale;
        const float first = spinDraw(random_, kStartSpinDivisor);
        const float second = spinDraw(random_, kStartSpinDivisor);
        const float third = spinDraw(random_, kStartSpinDivisor);
        body.spin = {third, second, first};
    }
    return index;
}

void FallApartPool::update(const DebrisLineTest& test, std::uint32_t clock) {
    for (FallApartSlot& slot : slots_) {
        if (!slot.inUse) continue;
        if (slot.phase == FallApartPhase::Flying) {
            for (unsigned part = 0; part < kFallApartParts; ++part) {
                if (!partUsed(slot, part)) continue;
                const FallApartBounds& bounds = part == unsigned(FallApartPart::Head) ? slot.headBounds : partBounds_[part];
                fallApartStep(slot.parts[part], bounds, kFallApartBounce, kFallApartGravity, test, random_);
            }
            if (--slot.counter == 0) {
                slot.phase = FallApartPhase::Blinking;
                slot.counter = kFallApartPhaseUpdates;
            }
            continue;
        }
        if (--slot.counter == 0) {
            release(slot);
            continue;
        }
        if ((clock & 1) != 0) continue;
        for (unsigned part = 0; part < kFallApartParts; ++part)
            if (partUsed(slot, part)) slot.parts[part].enabled = !slot.parts[part].enabled;
    }
}

void FallApartPool::release(FallApartSlot& slot) {
    slot.inUse = false;
    for (unsigned part = 0; part < kFallApartParts; ++part)
        if (partUsed(slot, part)) slot.parts[part].enabled = false;
}

}
