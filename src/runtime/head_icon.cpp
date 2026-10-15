#include "runtime/head_icon.hpp"

#include <algorithm>
#include <cmath>

namespace chase::runtime {

int headIconAlphaByte(float fade, float distance) {
    int alpha = int(fade * 255.0f);
    if (distance < kHeadIconNearDistance) return 0;
    if (distance < kHeadIconFarDistance) {
        const float t = (distance - kHeadIconNearDistance) / (kHeadIconFarDistance - kHeadIconNearDistance);
        alpha = int(t * fade * 255.0f);
    }
    return alpha;
}

std::array<float, 3> headIconPosition(const float node[3], float entityScale) {
    return {node[0], node[1] + kHeadIconLift + entityScale * kHeadIconScaleFactor, node[2]};
}

HeadIconScreen projectHeadIcon(const std::array<float, 3>& position, const float eye[3], float yaw, float pitch, float verticalFov) {
    const float forward[3] = {std::cos(pitch) * std::sin(yaw), std::sin(pitch), std::cos(pitch) * std::cos(yaw)};
    const float right[3] = {std::cos(yaw), 0.0f, -std::sin(yaw)};
    const float up[3] = {forward[1] * right[2] - forward[2] * right[1], forward[2] * right[0] - forward[0] * right[2],
                         forward[0] * right[1] - forward[1] * right[0]};
    float offset[3];
    for (int axis = 0; axis < 3; ++axis) offset[axis] = position[std::size_t(axis)] - eye[axis];
    const float depth = offset[0] * forward[0] + offset[1] * forward[1] + offset[2] * forward[2];
    HeadIconScreen screen;
    if (depth <= 0.0f) return screen;
    const float viewX = offset[0] * right[0] + offset[1] * right[1] + offset[2] * right[2];
    const float viewY = offset[0] * up[0] + offset[1] * up[1] + offset[2] * up[2];
    const float scaleY = 1.0f / std::tan(verticalFov * 0.5f), scaleX = scaleY / (400.0f / 240.0f);
    screen.centreX = (viewX * scaleX / depth + 1.0f) * 200.0f;
    screen.centreY = (1.0f - viewY * scaleY / depth) * 120.0f;
    screen.width = kHeadIconQuadSize * scaleX / depth * 200.0f;
    screen.height = kHeadIconQuadSize * scaleY / depth * 120.0f;
    screen.visible = true;
    return screen;
}

}
