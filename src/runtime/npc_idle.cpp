#include "runtime/npc_idle.hpp"

#include <algorithm>

namespace chase::runtime {

namespace {
double positionOf(double elapsedSeconds, const NpcIdleClip& clip) {
    return std::min(elapsedSeconds * clip.framesPerSecond, clip.frames);
}

bool finished(double elapsedSeconds, const NpcIdleClip& clip) {
    return clip.frames <= 0 || elapsedSeconds * clip.framesPerSecond >= clip.frames;
}
}

void NpcIdle::update(double dt, GameRandom& random, const NpcIdleClip& idle, const NpcIdleClip& idle2) {
    if (started_) {
        elapsed_ += dt;
        if (!finished(elapsed_, clipOf(variant_, idle, idle2))) return;
    }
    started_ = true;
    variant_ = (random.next() & kIdleRandomBit) != 0 ? NpcIdleVariant::Idle2 : NpcIdleVariant::Idle;
    elapsed_ = 0;
}

NpcIdlePose NpcIdle::pose(const NpcIdleClip& idle, const NpcIdleClip& idle2) const {
    return {variant_, positionOf(elapsed_, clipOf(variant_, idle, idle2))};
}

}
