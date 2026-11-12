#include "runtime/animation_slots.hpp"

#include <cmath>
#include <cstring>

namespace chase::runtime {
namespace {

constexpr float kTicksPerSecond = float(kAnimationClockTicksPerSecond);

std::uint32_t truncateUnsigned(float value) {
    if (!(value > 0.0f)) return 0;
    if (value >= 4294967296.0f) return 0xffffffffu;
    return std::uint32_t(value);
}

std::int32_t truncateSigned(float value) {
    if (std::isnan(value)) return 0;
    if (value >= 2147483648.0f) return 0x7fffffff;
    if (value < -2147483648.0f) return std::int32_t(0x80000000u);
    return std::int32_t(value);
}

std::int32_t bitsOf(float value) {
    std::int32_t bits;
    std::memcpy(&bits, &value, sizeof bits);
    return bits;
}

}

const AnimationSlot& AnimationSlots::start(const AnimationClipInfo& clip, unsigned mode, std::uint16_t first, std::uint16_t end,
                                           float speed, float blend, std::int64_t now) {
    if (count_ < kCapacity) ++count_;
    for (std::size_t index = count_ - 1; index > 0; --index) slots_[index] = slots_[index - 1];
    AnimationSlot& slot = slots_[0];
    slot = AnimationSlot{};
    starts_ = (starts_ + 1) & 0x7fffffffu;
    slot.clip = clip.clip;
    slot.length = clip.length;
    slot.startId = starts_;
    slot.mode = mode & 7u;
    slot.first = first == kAnimationFrameDefault ? clip.length : first;
    slot.end = end == kAnimationFrameDefault ? clip.length : end;
    slot.rate = clip.rate;
    slot.rateSpeed = float(clip.rate) * speed;
    slot.wrap = clip.loops;
    if (!slot.wrap && (mode == kAnimationLoop || mode == kAnimationReverseLoop)) {
        slot.wrap = true;
        slot.end = std::uint16_t(slot.end + 1);
    }
    slot.start = now;
    slot.stop = -1;
    slot.started = now;
    slot.blend = blend;
    return slot;
}

AnimationPosition AnimationSlots::position(const AnimationSlot& slot, std::int64_t now, std::int32_t offset) {
    AnimationPosition result;
    result.finished = slot.mode == kAnimationStopped;
    const std::int64_t clock = slot.stop == -1 ? now : slot.stop;
    const float elapsed = float(std::uint32_t(clock - slot.start));
    float p = (elapsed * slot.rateSpeed) / kTicksPerSecond + float(offset);
    const std::int32_t span = std::int32_t(slot.end) - std::int32_t(slot.first);
    if (span == (slot.wrap ? 1 : 0)) {
        result.finished = true;
        result.loops = 0;
        result.frame = float(slot.first);
        return result;
    }
    const float spanFrames = float(std::uint32_t(span));
    switch (slot.mode) {
    case kAnimationOnce:
        if (!(0.0f <= p)) p = 0.0f;
        if (!(p < spanFrames)) {
            result.finished = true;
            result.loops = 0;
            p = spanFrames;
        }
        break;
    case kAnimationLoop:
    case kAnimationStopped: {
        std::uint32_t loops = truncateUnsigned(p / spanFrames);
        p -= float(std::uint32_t(span) * loops);
        if (slot.wrap && loops != 0 && bitsOf(p) < 0x3f800000) {
            --loops;
            p += spanFrames;
        }
        result.loops = loops & 0x7fffffffu;
        break;
    }
    case kAnimationReverse:
        p = spanFrames - p;
        if (!(0.0f <= p)) {
            result.finished = true;
            result.loops = 0;
            p = 0.0f;
        }
        break;
    case kAnimationPingPong: {
        const std::uint32_t period = std::uint32_t(span) * 2u;
        const std::int32_t loops = truncateSigned(p / float(period));
        result.loops = std::uint32_t(loops) & 0x7fffffffu;
        p -= float(period * std::uint32_t(loops));
        if (p > spanFrames) p = float(period) - p;
        break;
    }
    case kAnimationReverseLoop: {
        const std::uint32_t loops = truncateUnsigned(p / spanFrames);
        p -= float(std::uint32_t(span) * loops);
        if (p != 0.0f) p = spanFrames - p;
        result.loops = loops & 0x7fffffffu;
        break;
    }
    default: break;
    }
    result.frame = float(slot.first) + p;
    return result;
}

unsigned AnimationSlots::kind(std::int64_t now) const {
    if (!count_) return kAnimationStopped;
    const AnimationSlot& slot = slots_[0];
    if (slot.mode != kAnimationOnce && slot.mode != kAnimationReverse) return slot.mode;
    return position(slot, now).finished ? unsigned(kAnimationStopped) : slot.mode;
}

void AnimationSlots::stop(std::int64_t now) {
    if (!count_) return;
    AnimationSlot& slot = slots_[0];
    if (slot.mode == kAnimationStopped) return;
    if (slot.stop == -1) slot.stop = now;
    slot.mode = kAnimationStopped;
}

void AnimationSlots::pause(bool paused, std::int64_t now) {
    if (!count_) return;
    AnimationSlot& slot = slots_[0];
    if (slot.mode == kAnimationStopped) return;
    if (paused) {
        if (slot.stop == -1) slot.stop = now;
        return;
    }
    if (slot.stop == -1) return;
    slot.start += now - slot.stop;
    slot.stop = -1;
}

void AnimationSlots::setMode(unsigned mode) {
    if (count_) slots_[0].mode = mode & 7u;
}

void AnimationSlots::setOffset(float frame, std::int64_t now) {
    if (!count_) return;
    AnimationSlot& slot = slots_[0];
    const float rate = float(slot.rate);
    const float relative = frame - float(slot.first);
    if (slot.mode == kAnimationStopped) {
        slot.stop = std::int64_t(truncateSigned((kTicksPerSecond * relative * rate) / (rate * slot.rateSpeed))) + slot.start;
        return;
    }
    float distance = 0.0f;
    switch (slot.mode) {
    case kAnimationOnce:
    case kAnimationLoop:
    case kAnimationPingPong: distance = relative; break;
    case kAnimationReverse:
    case kAnimationReverseLoop:
        distance = float(std::int32_t(slot.end) - std::int32_t(slot.first));
        if (slot.wrap) distance += 1.0f;
        distance -= relative;
        break;
    default: break;
    }
    slot.start = now - std::int64_t(truncateSigned((kTicksPerSecond * distance * rate) / (rate * slot.rateSpeed)));
}

bool AnimationSlots::currentNearEnd(std::int64_t now) const {
    const unsigned current = kind(now);
    if (current == kAnimationStopped) return true;
    if (current != kAnimationLoop && current != kAnimationReverseLoop) return false;
    const float distance = std::fabs(float(slots_[0].length) - position(slots_[0], now).frame);
    return bitsOf(distance) < 0x3f800001;
}

}
