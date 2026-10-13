#include "runtime/follow_camera.hpp"
#include <cmath>

namespace chase::runtime {

namespace {
// 0x1ed9ac with the factors of 0x2e0bf0: the tracker moves the fraction (1 - factor) towards the wanted point.
void track(float tracked[3], const float wanted[3], const float factor[3]) {
    for (int i = 0; i < 3; ++i) tracked[i] += (1.0f - factor[i]) * (wanted[i] - tracked[i]);
}
}

float quadraticSpline(float a, float b, float c, float t) {
    return a + ((b * 4.0f - a * 3.0f) - c) * t + ((a - b * 2.0f) + c) * 2.0f * t * t;
}

bool stickTurnsCamera(std::uint16_t entityType) {
    return (entityType >= 1 && entityType <= 5) || entityType == 0x11c;
}

float stickTurnInput(float stickX, std::uint16_t entityType) {
    if (!stickTurnsCamera(entityType)) return 0.0f;
    if (stickX > 0.25f) return (stickX - 0.25f) * 1.3333334f;
    if (stickX < -0.25f) return (stickX + 0.25f) * 1.3333334f;
    return 0.0f;
}

float shoulderTurnInput(float left, float right, bool invert) {
    return invert ? left * 2.0f + right * -2.0f : left * -2.0f + right * 2.0f;
}

FollowCamera::FollowCamera(const FollowCameraPreset& preset, float bodyHeight) : preset_(preset), bodyHeight_(bodyHeight) {}

FollowCamera::Layout FollowCamera::layout() const {
    const float t = zoom_ * 0.5f;
    Layout result;
    result.distance = quadraticSpline(preset_.distance[0], preset_.distance[1], preset_.distance[2], t);
    result.height = quadraticSpline(preset_.height[0], preset_.height[1], preset_.height[2], t);
    result.bodyUp = bodyHeight_ - preset_.groundOffset;
    const float lookUp = quadraticSpline(preset_.lookHeight[0], preset_.lookHeight[1], preset_.lookHeight[2], t) + bodyHeight_;
    result.lookUp = lookUp - result.bodyUp;
    result.lookAhead = quadraticSpline(preset_.lookDepth[0], preset_.lookDepth[1], preset_.lookDepth[2], t);
    return result;
}

void FollowCamera::reset(const float target[3], float yaw, float facing) {
    const Layout l = layout();
    for (int i = 0; i < 3; ++i) b_[i] = target[i];
    b_[1] += l.bodyUp;
    c_[0] = std::sin(facing) * l.lookAhead;
    c_[1] = l.lookUp;
    c_[2] = std::cos(facing) * l.lookAhead;
    a_[0] = b_[0] - std::sin(yaw) * l.distance;
    a_[1] = b_[1] + c_[1] + l.height;
    a_[2] = b_[2] - std::cos(yaw) * l.distance;
    for (int i = 0; i < 3; ++i) {
        aTracked_[i] = a_[i];
        bTracked_[i] = b_[i];
        cTracked_[i] = c_[i];
    }
    refreshPose();
}

void FollowCamera::restore(const float wantedEye[3], const float trackedEye[3], const float trackedLook[3],
                           const float target[3], float facing) {
    const Layout l = layout();
    for (int i = 0; i < 3; ++i) {
        a_[i] = wantedEye[i];
        aTracked_[i] = trackedEye[i];
        b_[i] = target[i];
    }
    b_[1] += l.bodyUp;
    c_[0] = std::sin(facing) * l.lookAhead;
    c_[1] = l.lookUp;
    c_[2] = std::cos(facing) * l.lookAhead;
    for (int i = 0; i < 3; ++i) {
        cTracked_[i] = c_[i];
        bTracked_[i] = trackedLook[i] - c_[i];
    }
    refreshPose();
}

void FollowCamera::update(const FollowCameraInput& input) {
    const Layout l = layout();
    for (int i = 0; i < 3; ++i) b_[i] = input.target[i];
    b_[1] += l.bodyUp;
    c_[0] = std::sin(input.targetFacing) * l.lookAhead;
    c_[1] = l.lookUp;
    c_[2] = std::cos(input.targetFacing) * l.lookAhead;

    float push[3] = {0, 0, 0}, swing[3] = {0, 0, 0};
    push[1] = 0.3f * ((b_[1] + c_[1] + l.height) - a_[1]);

    float toTargetX = input.target[0] - a_[0], toTargetZ = input.target[2] - a_[2];
    const float away = std::sqrt(toTargetX * toTargetX + toTargetZ * toTargetZ);
    if (away > 1.0e-6f) {
        toTargetX /= away;
        toTargetZ /= away;
    } else {
        toTargetX = 0.0f;
        toTargetZ = 1.0f;
    }
    push[0] = toTargetX * (away - l.distance);
    push[2] = toTargetZ * (away - l.distance);

    const float turn = (input.shoulderTurn + input.stickTurn) * input.dt * preset_.turnGain;
    if (turn != 0.0f) {
        const float x = a_[0] - b_[0], z = a_[2] - b_[2];
        swing[0] = x * std::cos(turn) + z * std::sin(turn) - x;
        swing[2] = -x * std::sin(turn) + z * std::cos(turn) - z;
    }
    for (int i = 0; i < 3; ++i) {
        a_[i] += push[i] + swing[i];
        aTracked_[i] += swing[i];
    }

    const float k = input.dt * 30.0f;
    const float positionFactor = std::pow(preset_.positionBase, k), targetFactor = std::pow(preset_.targetBase, k);
    const float positionFactors[3] = {positionFactor, positionFactor, positionFactor};
    const float targetFactors[3] = {targetFactor, targetFactor, targetFactor};
    track(aTracked_, a_, positionFactors);
    track(bTracked_, b_, targetFactors);
    track(cTracked_, c_, targetFactors);
    refreshPose();
}

void FollowCamera::refreshPose() {
    for (int i = 0; i < 3; ++i) {
        position_[i] = aTracked_[i];
        look_[i] = bTracked_[i] + cTracked_[i];
    }
}

float FollowCamera::verticalFieldOfView(float poseDegrees) {
    const float half = poseDegrees * 3.1415927f * 0.0055555557f * 0.5f;
    return 2.0f * std::atan(0.75f * std::tan(half));
}

float FollowCamera::yaw() const {
    const float x = look_[0] - position_[0], z = look_[2] - position_[2];
    return (x == 0.0f && z == 0.0f) ? 0.0f : std::atan2(x, z);
}

float FollowCamera::pitch() const {
    const float x = look_[0] - position_[0], z = look_[2] - position_[2];
    return std::atan2(look_[1] - position_[1], std::sqrt(x * x + z * z));
}

}
