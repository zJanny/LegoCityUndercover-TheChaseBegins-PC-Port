#include "runtime/follow_camera.hpp"
#include "runtime/camera_near_plane.hpp"
#include "runtime/camera_reset_probe.hpp"
#include <algorithm>
#include <cmath>
#include <array>
#include <optional>

namespace chase::runtime {

namespace {
constexpr float kVehicleExtraDistance = 10.0f;  // in+0xfc written by 0x32f334
constexpr float kVehicleExtraHeight = 2.5f;     // in+0x100
constexpr float kSideHoldSeconds = 0.5f;
constexpr float kTopScreenAspect = 400.0f / 240.0f;
constexpr std::uint8_t kVehicleObjectType = 183;
constexpr unsigned kResetHeading = 360;
constexpr unsigned kCandidateDegrees[8] = {180, 0, 90, 270, 45, 135, 225, 315};  // table at 0x359780
constexpr float kPullInMargin = 0.25f;     // in+0x8c, the ground offset of the preset block (+0x18)
constexpr float kNearestHitMinimum = 0.1f;  // literal 0x3dcccccd in 0x2313c8
// 0x257ea0: unit vector, (0, 0, 1) for a vector of length 1.2e-7 or less.
void unitVector(const float v[3], float out[3]) {
    const float length = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (length <= 1.1920929e-7f) {
        out[0] = 0.0f;
        out[1] = 0.0f;
        out[2] = 1.0f;
        return;
    }
    const float scale = 1.0f / length;
    for (int i = 0; i < 3; ++i) out[i] = v[i] * scale;
}

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

float stickTurnInput(float stickX, std::uint16_t entityType, std::uint8_t seatedVehicleType) {
    if (!stickTurnsCamera(entityType) || seatedVehicleType == kVehicleObjectType) return 0.0f;
    if (stickX > 0.25f) return (stickX - 0.25f) * 1.3333334f;
    if (stickX < -0.25f) return (stickX + 0.25f) * 1.3333334f;
    return 0.0f;
}

float shoulderTurnInput(float left, float right, bool invert) {
    return invert ? left * 2.0f + right * -2.0f : left * -2.0f + right * 2.0f;
}

FollowCameraPreset FollowCameraPreset::inVehicle() {
    FollowCameraPreset preset;
    preset.distance[0] = 4.5f, preset.distance[1] = 5.5f, preset.distance[2] = 6.5f;
    preset.height[0] = 0.2f, preset.height[1] = 0.35f, preset.height[2] = 0.5f;
    preset.lookHeight[0] = preset.lookHeight[1] = preset.lookHeight[2] = -0.5f;
    preset.lookDepth[0] = preset.lookDepth[1] = preset.lookDepth[2] = 1.5f;
    preset.pitchHigh = 0.2f;
    preset.autoTurnGain = 1.0f;
    preset.positionBase = 0.7f;
    preset.targetBase = 0.0f;
    preset.vehicleLayout = true;
    return preset;
}

FollowCameraPreset& FollowCameraPreset::withVehicleOverrides(float posY, float posZ, float lookY, float lookZ) {
    height[1] = posY;
    distance[1] = posZ;
    lookHeight[1] = lookY;
    lookDepth[1] = lookZ;
    return *this;
}

void FollowCamera::setSubject(const FollowCameraPreset& preset, float bodyHeight, float modelHalfHeight) {
    preset_ = preset;
    bodyHeight_ = bodyHeight;
    modelHalfHeight_ = modelHalfHeight;
}

FollowCamera::FollowCamera(const FollowCameraPreset& preset, float bodyHeight) : preset_(preset), bodyHeight_(bodyHeight) {
    projectionScales(kFieldOfViewDegrees, xScale_, yScale_);
}

void FollowCamera::projectionScales(float poseDegrees, float& xScale, float& yScale) {
    const float half = poseDegrees * 3.1415927f * 0.0055555557f * 0.5f;
    yScale = 1.0f / std::tan(half) * 1.3333334f;
    xScale = yScale / kTopScreenAspect;
}

FollowCamera::Layout FollowCamera::layout() const {
    const float t = zoom_ * 0.5f;
    Layout result;
    result.baseDistance = quadraticSpline(preset_.distance[0], preset_.distance[1], preset_.distance[2], t);
    result.baseHeight = quadraticSpline(preset_.height[0], preset_.height[1], preset_.height[2], t);
    if (preset_.vehicleLayout) {
        result.baseDistance += kVehicleExtraDistance;
        result.baseHeight += kVehicleExtraHeight;
    }
    result.distance = result.baseDistance * clearance_;
    result.height = result.baseHeight * clearance_;
    result.bodyUp = bodyHeight_ - preset_.groundOffset;
    const float lookHeight = quadraticSpline(preset_.lookHeight[0], preset_.lookHeight[1], preset_.lookHeight[2], t);
    const float lookUp = preset_.vehicleLayout ? lookHeight + modelHalfHeight_ * 2.0f : lookHeight + bodyHeight_;
    result.lookUp = (lookUp - result.bodyUp) * clearance_;
    result.lookAhead = quadraticSpline(preset_.lookDepth[0], preset_.lookDepth[1], preset_.lookDepth[2], t) * clearance_;
    return result;
}

void FollowCamera::reset(const float target[3], float yaw, float facing) {
    storedHeading_ = kResetHeading;
    clearance_ = 1.0f;
    springHold_ = 0.0f;
    sideHold_ = 0.0f;
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

void FollowCamera::placeCandidates(const Layout& l, float facing, bool lineTest) {
    const unsigned heading = lineTest ? kResetHeading : 0;
    std::optional<CameraResetProbe> probe;
    if (world_ && !world_->empty()) probe.emplace(*world_, excludedOwner_);
    const float sine = std::sin(facing), cosine = std::cos(facing);
    for (int index = 0; index < 8; ++index) {
        const float angle = (float(kCandidateDegrees[index]) + float(heading)) * 3.1415927f * 0.0055555557f;
        const float x = std::sin(angle) * l.distance, z = std::cos(angle) * l.distance;
        a_[0] = b_[0] + (x * cosine + z * sine);
        a_[1] = b_[1] + l.height;
        a_[2] = b_[2] + (-x * sine + z * cosine);
        if (!probe) return;
        if (lineTest) {
            if (!probe->segmentBlocked(b_, a_)) return;
            continue;
        }
        const std::optional<std::array<float, 3>> hit = probe->nearestHit(b_, a_);
        if (!hit) return;
        const float toHit[3] = {(*hit)[0] - b_[0], (*hit)[1] - b_[1], (*hit)[2] - b_[2]};
        const float length = std::sqrt(toHit[0] * toHit[0] + toHit[1] * toHit[1] + toHit[2] * toHit[2]);
        if (kNearestHitMinimum > length) continue;
        const float ratio = (length - kPullInMargin) / length;
        for (int i = 0; i < 3; ++i) a_[i] = b_[i] + toHit[i] * ratio;
        return;
    }
}

bool FollowCamera::takeOver(const float target[3], float facing, const float currentEye[3], bool previousIsRecordPose) {
    const Layout l = layout();
    clearance_ = 1.0f;
    for (int i = 0; i < 3; ++i) b_[i] = target[i];
    b_[1] += l.bodyUp;
    c_[0] = std::sin(facing) * l.lookAhead;
    c_[1] = l.lookUp;
    c_[2] = std::cos(facing) * l.lookAhead;
    placeCandidates(l, facing, storedHeading_ == kResetHeading);
    storedHeading_ = kResetHeading;
    const float dx = b_[0] - currentEye[0], dy = b_[1] - currentEye[1], dz = b_[2] - currentEye[2];
    const bool nearCamera = previousIsRecordPose && std::sqrt(dx * dx + dy * dy + dz * dz) < 2.0f * preset_.distance[1];
    if (nearCamera)
        for (int i = 0; i < 3; ++i) a_[i] = currentEye[i];
    for (int i = 0; i < 3; ++i) {
        aTracked_[i] = a_[i];
        bTracked_[i] = b_[i];
        cTracked_[i] = c_[i];
    }
    refreshPose();
    return nearCamera;
}

void FollowCamera::clearStoredHeading() { storedHeading_ = 0; }

void FollowCamera::snap(const float target[3], float facing) {
    const Layout l = layout();
    clearance_ = 1.0f;
    for (int i = 0; i < 3; ++i) b_[i] = target[i];
    b_[1] += l.bodyUp;
    c_[0] = std::sin(facing) * l.lookAhead;
    c_[1] = l.lookUp;
    c_[2] = std::cos(facing) * l.lookAhead;
    placeCandidates(l, facing, false);
    storedHeading_ = kResetHeading;
    for (int i = 0; i < 3; ++i) {
        aTracked_[i] = a_[i];
        bTracked_[i] = b_[i];
        cTracked_[i] = c_[i];
    }
    refreshPose();
}

void FollowCamera::restore(const float wantedEye[3], const float trackedEye[3], const float trackedLook[3],
                           const float target[3], float facing, float clearance, float springHold) {
    clearance_ = clearance;
    springHold_ = springHold;
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

    const float movedX = b_[0] - previousLook_[0], movedY = b_[1] - previousLook_[1], movedZ = b_[2] - previousLook_[2];
    const bool moving = movedX * movedX + movedY * movedY + movedZ * movedZ > 1.0e-4f;
    const float poseScale = clearance_ * zoom_;
    const float autoTurn = automaticTurn(input);
    if (moving) {
        springHold_ = std::max(springHold_ - input.dt, 0.0f);
        sideHold_ = std::max(sideHold_ - input.dt, 0.0f);
    }

    float push[3] = {0, 0, 0}, swing[3] = {0, 0, 0};
    push[1] = std::max(0.5f - springHold_, 0.0f) * 0.3f * 2.0f * ((b_[1] + c_[1] + l.height) - a_[1]);

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

    const float turn = autoTurn + (input.shoulderTurn + input.stickTurn) * input.dt * preset_.turnGain;
    if (turn != 0.0f) {
        const float x = a_[0] - b_[0], z = a_[2] - b_[2];
        swing[0] = x * std::cos(turn) + z * std::sin(turn) - x;
        swing[2] = -x * std::sin(turn) + z * std::cos(turn) - z;
    }
    if (world_ && !world_->empty()) avoidObstacles(input, l, moving, push, swing);
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
    clampPitch(input, poseScale);
    snapToClearView();
    refreshPose();
}

// Tail of 0x2e0bf0: when the wanted position sees further than the tracked one before the first obstruction (0x2e0e28, both looking at the
// tracked look point), the tracker jumps to the wanted position (0x211d98); the larger depth becomes the near plane (0x23f168).
void FollowCamera::snapToClearView() {
    if (!swept_) return;
    const float look[3] = {bTracked_[0] + cTracked_[0], bTracked_[1] + cTracked_[1], bTracked_[2] + cTracked_[2]};
    nearPlane_ = cameraNearDepth(sweep_, aTracked_, look, xScale_, yScale_);
    const float wantedNear = cameraNearDepth(sweep_, a_, look, xScale_, yScale_);
    if (wantedNear > nearPlane_) {
        for (int i = 0; i < 3; ++i) aTracked_[i] = a_[i];
        nearPlane_ = wantedNear;
    }
}

// 0x2e0a50: keeps the look point's height between the target's head and feet, widened by the view depth (at least 10).
void FollowCamera::clampPitch(const FollowCameraInput& input, float poseScale) {
    if (preset_.vehicleLayout || !(poseScale > 0.99f)) return;
    const float lookHeight = bTracked_[1] + cTracked_[1];
    const float toLook[3] = {bTracked_[0] - aTracked_[0], lookHeight - aTracked_[1], bTracked_[2] - aTracked_[2]};
    float forward[3];
    unitVector(toLook, forward);
    const float toHead[3] = {input.target[0] - aTracked_[0], input.target[1] + bodyHeight_ - aTracked_[1], input.target[2] - aTracked_[2]};
    float depth = toHead[0] * forward[0] + toHead[1] * forward[1] + toHead[2] * forward[2];
    if (!(depth > 10.0f)) depth = 10.0f;
    const float upper = input.target[1] + preset_.pitchHigh * depth;
    float lower = input.target[1] + bodyHeight_ - preset_.pitchLow * depth;
    float clamped;
    if (lookHeight > upper) {
        clamped = upper;
    } else {
        if (lookHeight > lower) lower = lookHeight;
        clamped = lower;
    }
    bTracked_[1] = clamped - cTracked_[1];
}

float FollowCamera::automaticTurn(const FollowCameraInput& input) const {
    if (preset_.autoTurnGain == 0.0f || input.shoulderTurn != 0.0f) return 0.0f;
    const float x = a_[0] - input.target[0], z = a_[2] - input.target[2];
    const float sine = std::sin(input.targetFacing), cosine = std::cos(input.targetFacing);
    const float localRight = x * cosine - z * sine, localForward = x * sine + z * cosine;
    const float angle = std::atan2(localRight, -localForward);
    const float wanted = std::fabs(angle * 0.5f) * angle * 0.5f;
    const float limit = input.dt * preset_.autoTurnGain;
    float clamped = limit;
    if (wanted < limit) {
        clamped = wanted;
        if (wanted <= -limit) clamped = -limit;
    }
    return clamped * std::max(0.4f - sideHold_, 0.0f) * 2.0f;
}

void FollowCamera::avoidObstacles(const FollowCameraInput& input, const Layout& l, bool moving, float push[3], const float swing[3]) {
    float wanted[3];
    for (int i = 0; i < 3; ++i) wanted[i] = (a_[i] + swing[i]) + push[i];
    const float fullDistance = std::sqrt(l.baseDistance * l.baseDistance + l.baseHeight * l.baseHeight);
    CameraSweep& sweep = sweep_;
    CameraSweepSettings settings = preset_.sweep;
    settings.previousNear = previousNear_;
    settings.excludedOwner = excludedOwner_;
    sweep.prepare(*world_, wanted, b_, fullDistance, settings);
    swept_ = true;
    previousNear_ = sweep.nearDepth();
    const CameraSweepGap side = sweep.gap(CameraSweep::Side), up = sweep.gap(CameraSweep::Up);
    const bool turning = input.shoulderTurn != 0.0f;
    const bool clear = turning ? side.low == -1.0f && side.high == 1.0f
                               : side.low == -1.0f || side.high == 1.0f || !(0.5f > side.high - side.low);
    if (clear && clearance_ >= 0.99f) {
        clearance_ = std::min(clearance_ + input.dt * 0.5f, 1.0f);
    } else {
        float toWanted[3] = {wanted[0] - b_[0], wanted[1] - b_[1], wanted[2] - b_[2]};
        float direction[3];
        unitVector(toWanted, direction);
        float ratio;
        float probe[3];
        if (!turning) {
            float step[3];
            for (int i = 0; i < 3; ++i) {
                step[i] = direction[i] * fullDistance;
                probe[i] = b_[i] + step[i];
            }
            for (int iteration = 0; iteration < 10; ++iteration) {
                const float dx = probe[0] - b_[0], dy = probe[1] - b_[1], dz = probe[2] - b_[2];
                const CameraSweepGap gap = sweep.sideGapAt(std::sqrt(dx * dx + dy * dy + dz * dz));
                for (int i = 0; i < 3; ++i) step[i] *= 0.5f;
                for (int i = 0; i < 3; ++i) probe[i] = gap.high - gap.low >= 0.5f ? probe[i] + step[i] : probe[i] - step[i];
            }
            const float dx = probe[0] - b_[0], dz = probe[2] - b_[2];
            ratio = std::sqrt(dx * dx + dz * dz) / l.baseDistance;
        } else {
            for (int i = 0; i < 3; ++i) probe[i] = wanted[i];
            const float reach = sweep.farDepth();
            const float fromSide = 1.0f - sweep.deepestObstacle(CameraSweep::Side, 1.0f) / reach;
            const float fromUp = 1.0f - sweep.deepestObstacle(CameraSweep::Up, 1.0f) / reach;
            ratio = std::min(fromSide, fromUp);
        }
        if (ratio > 1.0f) {
            ratio = 1.0f;
        } else if (ratio < 0.01f) {
            ratio = 0.01f;
        }
        clearance_ = clearance_ * 0.5f + ratio * 0.5f;
        float toProbe[3] = {probe[0] - b_[0], probe[1] - b_[1], probe[2] - b_[2]};
        unitVector(toProbe, direction);
        const float length = clearance_ * fullDistance;
        for (int i = 0; i < 3; ++i) {
            const float placed = b_[i] + direction[i] * length;
            push[i] = (push[i] + placed) - wanted[i];
        }
    }
    if (turning) return;

    const auto pushed = [](float middle, const CameraSweepGap& gap) {
        return middle < -0.001f || middle > 0.001f || (gap.low == 0.0f && gap.high == 0.0f);
    };
    const float* sideAxis = sweep.sideAxis();
    const float* upAxis = sweep.upAxis();
    if (pushed(side.middle, side)) {
        const float amount = preset_.sweep.sideHalfWidth * side.middle;
        for (int i = 0; i < 3; ++i) push[i] += sideAxis[i] * amount;
        sideHold_ = kSideHoldSeconds;
    }
    if (pushed(up.middle, up)) {
        if (side.low == 0.0f && side.high == 0.0f && moving) {
            const float sideAmount = preset_.sweep.sideHalfWidth * 0.5f;
            for (int i = 0; i < 3; ++i) push[i] += sideAxis[i] * sideAmount;
            const float upAmount = preset_.sweep.upHalfWidth * (up.middle * 3.0f);
            for (int i = 0; i < 3; ++i) push[i] += upAxis[i] * upAmount;
        } else {
            const float upAmount = preset_.sweep.upHalfWidth * up.middle;
            for (int i = 0; i < 3; ++i) push[i] += upAxis[i] * upAmount;
        }
        springHold_ = 0.5f;
    }
}

void FollowCamera::refreshPose() {
    for (int i = 0; i < 3; ++i) {
        previousLook_[i] = b_[i];
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
