#include "runtime/semi_trans_near_camera.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace chase::runtime {

namespace {

float dot(const Vector3& a, const Vector3& b) {
    float sum = a[0] * b[0];
    sum += a[1] * b[1];
    sum += a[2] * b[2];
    return sum;
}

}

Vector3 nearCameraDirection(const NearCameraView& view) {
    Vector3 direction{view.target[0] - view.eye[0], 0.0f, view.target[2] - view.eye[2]};
    const float length = std::sqrt(dot(direction, direction));
    std::uint32_t bits = 0;
    std::memcpy(&bits, &length, sizeof bits);
    if (std::int32_t(bits) <= 0x34000000) {
        direction = {0.0f, 0.0f, 0.0f};
    } else {
        const float scale = 1.0f / length;
        for (float& c : direction) c *= scale;
    }
    if (std::int32_t(bits) < 0x3727c5ac) direction = view.forward;
    return direction;
}

NearCameraTarget nearCameraTarget(const NearCameraView& view, const Vector3& direction, const Vector3& centre, const Vector3& half) {
    const Vector3 offset{centre[0] - view.eye[0], centre[1] - view.eye[1], centre[2] - view.eye[2]};
    float reach = dot(offset, direction);
    const float width = half[0] + half[2];
    reach -= width * 0.5f;
    NearCameraTarget target;
    if (!(view.distance > reach)) return target;
    target.seeThrough = true;
    target.floor = kNearCameraFloor;
    if (view.distance * 0.5f > reach) {
        target.alpha = 0;
    } else {
        const float share = (view.distance - reach) / view.distance;
        float alpha = 0.5f - share;
        alpha *= 2.0f;
        alpha *= 255.0f;
        target.alpha = int(alpha);
    }
    if (target.alpha == 0) target.alpha = 1;
    return target;
}

std::int16_t semiTransStep(std::int16_t alpha, const NearCameraTarget& target) {
    if (!target.seeThrough) {
        const std::int16_t raised = std::int16_t(alpha + kSemiTransStep);
        return raised <= target.alpha ? raised : std::int16_t(target.alpha);
    }
    const std::int16_t lowered = std::int16_t(alpha - kSemiTransStep);
    if (lowered >= target.alpha) return std::int16_t(target.alpha);
    if (target.floor >= lowered) return std::int16_t(target.floor);
    return lowered;
}

std::uint8_t semiTransNodeAlpha(std::int16_t alpha) { return std::uint8_t(std::clamp<int>(alpha, 0, 255)); }

}
