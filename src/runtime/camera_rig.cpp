#include "runtime/camera_rig.hpp"

namespace chase::runtime {

CameraRig::CameraRig(const assets::Level& level, const std::string& camText) : records_(level, camText), selector_(records_) {}

void CameraRig::reset(const float target[3], float yaw, float facing) {
    follow_.reset(target, yaw, facing);
    selector_ = CameraRecordSelector(records_);
    selector_.update(target);
    wasLocked_ = locked();
    if (wasLocked_) pose_ = evaluateRecordPose(records_, selector_.blend(), target);
}

void CameraRig::update(const FollowCameraInput& input) {
    selector_.update(input.target);
    if (locked()) {
        pose_ = evaluateRecordPose(records_, selector_.blend(), input.target);
    } else {
        if (wasLocked_) follow_.reset(input.target, poseYaw(pose_), input.targetFacing);
        follow_.update(input);
    }
    wasLocked_ = locked();
}

float CameraRig::verticalFieldOfView() const {
    return FollowCamera::verticalFieldOfView(locked() ? pose_.fovDegrees : FollowCamera::kFieldOfViewDegrees);
}

}
