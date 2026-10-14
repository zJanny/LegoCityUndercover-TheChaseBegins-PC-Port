#pragma once
// The gameplay camera as the camera manager sees it (0x23f174): the command chosen by the record selection
// (camera_records.hpp) is either the follow controller 0x2a8bfc (FollowCamera) or the record pose 0x298cd8. While the
// record pose runs the follow controller does not run, so the shoulder buttons do not turn the camera
// (0x217b14 returns 0 and the HUD shows UI_CameraLocked on a press). No SDL dependency.
#include "runtime/camera_records.hpp"
#include "runtime/follow_camera.hpp"
#include <string>

namespace chase::runtime {

class CameraRig {
public:
    CameraRig(const assets::Level& level, const std::string& camText);

    void setCollision(const CollisionWorld* world) { follow_.setCollision(world); }
    void reset(const float target[3], float yaw, float facing);
    void update(const FollowCameraInput& input);

    // True while the record pose is the active camera command: 0x217b14 returns 0.
    bool locked() const { return selector_.command() == CameraCommand::RecordPose; }
    const float* eye() const { return locked() ? pose_.eye : follow_.eye(); }
    const float* lookAt() const { return locked() ? pose_.look : follow_.lookAt(); }
    float yaw() const { return locked() ? poseYaw(pose_) : follow_.yaw(); }
    float pitch() const { return locked() ? posePitch(pose_) : follow_.pitch(); }
    // Vertical field of view in radians for the pose FOV (40 for the follow controller, the record FOV otherwise).
    float verticalFieldOfView() const;

private:
    FollowCamera follow_;
    CameraRecords records_;
    CameraRecordSelector selector_;
    RecordPose pose_;
    bool wasLocked_ = false;
};

}
