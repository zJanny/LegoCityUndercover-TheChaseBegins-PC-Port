#include "audio/spatial.hpp"
#include <algorithm>
#include <cmath>

namespace chase::audio {
namespace {
float dot(const std::array<float, 3>& a, const std::array<float, 3>& b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
std::array<float, 3> cross(const std::array<float, 3>& a, const std::array<float, 3>& b) {
    return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}
std::array<float, 3> normalized(const std::array<float, 3>& v, const std::array<float, 3>& fallback) {
    const float length = std::sqrt(dot(v, v));
    if (!(length > 1e-9f)) return fallback;
    return {v[0] / length, v[1] / length, v[2] / length};
}
} // namespace

Listener listenerFromForward(const std::array<float, 3>& position, const std::array<float, 3>& forward) {
    Listener listener;
    listener.position = position;
    listener.forward = normalized(forward, {0, 0, 1});
    listener.right = normalized({listener.forward[2], 0, -listener.forward[0]}, {1, 0, 0});
    listener.up = cross(listener.forward, listener.right);
    return listener;
}

float distanceVolume(float distance, float minimumDistance, float maximumDistance) {
    if (maximumDistance <= distance) return 0.0f;
    if (distance <= minimumDistance) return 1.0f;
    return (maximumDistance - distance) / (maximumDistance - minimumDistance);
}

float panOf(const Listener& listener, const std::array<float, 3>& emitter) {
    const std::array<float, 3> offset = {emitter[0] - listener.position[0], emitter[1] - listener.position[1],
                                         emitter[2] - listener.position[2]};
    const float x = dot(offset, listener.right), z = dot(offset, listener.forward);
    const float length = std::sqrt(x * x + z * z);
    if (!(length > 1.1920929e-7f)) return 0.0f;
    return std::clamp(x / length, -1.0f, 1.0f);
}

StereoGains positionalGains(const EmitterShape& shape, const Listener& listener, const std::array<float, 3>& emitter,
                            float masterVolume) {
    const std::array<float, 3> offset = {emitter[0] - listener.position[0], emitter[1] - listener.position[1],
                                         emitter[2] - listener.position[2]};
    const float volume = shape.volume * distanceVolume(std::sqrt(dot(offset, offset)), shape.minimumDistance, shape.maximumDistance);
    if (!(volume > 0.0f)) return {};
    const float pan = panOf(listener, emitter);
    const float scaled = volume * masterVolume;
    return {pan > 0 ? scaled * (1.0f - pan) : scaled, pan < 0 ? scaled * (1.0f + pan) : scaled};
}

StereoGains flatGains(const EmitterShape& shape, float masterVolume) {
    const float scaled = shape.volume * masterVolume;
    return {scaled, scaled};
}

} // namespace chase::audio
