#include "runtime/node_fade.hpp"

#include <algorithm>

namespace chase::runtime {

void nodeFadeStart(NodeFade& fade) {
    fade.alpha = 1;
    fade.step = -1;
}

void nodeFadeStep(NodeFade& fade, const NodeFadeFrame& frame, const DistanceFadeTable& table) {
    if (frame.tier < 0 || frame.tier > 2) return;
    if (frame.forceFadeIn) {
        fade.step = kNodeFadeStepIn;
    } else {
        const float radiusSquared = frame.radius * frame.radius;
        const float beyond = frame.distanceSquared - radiusSquared;
        const std::size_t tier = static_cast<std::size_t>(frame.tier);
        if (fade.step < 0) {
            const float reach = table.fadeIn[tier] * table.fadeIn[tier];
            if (reach >= beyond) fade.step = kNodeFadeStepIn;
        } else {
            const float reach = table.fadeOut[tier] * table.fadeOut[tier];
            if (reach <= beyond) fade.step = kNodeFadeStepOut;
        }
    }
    if (frame.jumpToTarget) {
        fade.alpha = fade.step < 0 ? 0 : kNodeFadeOpaque;
        return;
    }
    const int moved = static_cast<int>(fade.alpha) + static_cast<int>(fade.step);
    fade.alpha = static_cast<std::uint8_t>(std::clamp(moved, 0, 255));
}

float nodeFadeOpacity(const NodeFade& fade) { return static_cast<float>(fade.alpha) / 255.0f; }

}
