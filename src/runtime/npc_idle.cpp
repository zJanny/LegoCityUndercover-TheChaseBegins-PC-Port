#include "runtime/npc_idle.hpp"

namespace chase::runtime {

bool NpcIdle::update(AnimationSlots& slots, std::int64_t now, GameRandom& random, const AnimationClipInfo& idle, const AnimationClipInfo& idle2) {
    if (slots.kind(now) != kAnimationStopped) return false;
    started_ = true;
    variant_ = (random.next() & kIdleRandomBit) != 0 ? NpcIdleVariant::Idle2 : NpcIdleVariant::Idle;
    slots.start(variant_ == NpcIdleVariant::Idle ? idle : idle2, kAnimationOnce, 0, kAnimationFrameDefault, kIdleSpeed, kIdleBlend, now);
    return true;
}

}
