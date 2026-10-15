#pragma once
// Node tween of the original (registered by 0x21b7cc, evaluated by 0x1593c8): a value that moves linearly from `from` to `to` over `duration`
// seconds and is clamped to 0..1. With the flags 3 the value is the node's alpha (int(value x 255)) and colour. docs/re/objective-hud.md.
// No SDL dependency.

namespace chase::runtime {

struct NodeTween {
    float from = 1.0f;
    float to = 1.0f;
    float duration = 0.0f;
    float elapsed = 0.0f;

    static NodeTween start(float from, float to, float duration) {
        NodeTween tween;
        tween.from = from;
        tween.to = to;
        tween.duration = duration;
        return tween;
    }
    void step(float dt) { elapsed += dt; }
    bool finished() const { return elapsed >= duration; }
    // 0x159420..0x159458: from + (to - from) elapsed / duration, 1.0 from 1.0 up and 0 at or below 0. A duration of 0 is not evaluated by the original
    // (0x159414); the value stays at `to` here.
    float value() const {
        if (duration <= 0.0f) return to;
        const float v = from + (to - from) * ((elapsed < duration ? elapsed : duration) / duration);
        if (v >= 1.0f) return 1.0f;
        return v > 0.0f ? v : 0.0f;
    }
};

}
