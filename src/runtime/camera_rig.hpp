#pragma once
// The gameplay camera as the camera manager sees it (0x23f174, 0x1cf0b8): the command chosen by the record selection
// (camera_records.hpp) is either the follow controller 0x2a8bfc (FollowCamera) or the record pose 0x298cd8. A new command
// is pushed on a stack (0x224080); the stack entries fade into each other with a smoothstep over the fade time (0x2ae250,
// curve 5 at 0x2b5950) and their weighted pose goes through the per-update low-pass of 0x2a00dc (filter factors from the
// record's Filters bytes, 0x19b8c8 / 0x21a538). While the record pose is the top command the follow controller does
// not take the shoulder buttons, so they do not turn the camera (0x217b14 returns 0 and the HUD shows UI_CameraLocked
// on a press). No SDL dependency.
#include "runtime/camera_records.hpp"
#include "runtime/follow_camera.hpp"
#include <string>
#include <vector>

namespace chase::runtime {

// Filter byte (record +0x4c / +0x4d) to the per-update factor: 0 gives 0.3 (0x287a90), otherwise 1 - ease4(byte * 0.02)
// with the curve 1 - (1 - x)^2 of 0x1dd648 (0x19b8c8).
float poseFilterFactor(std::uint8_t filterByte);
// Curve 5 of the fade between commands (0x2b5950): x * x * (3 - 2x).
float fadeCurve(float progress);

class CameraRig {
public:
    CameraRig(const assets::Level& level, const std::string& camText);

    void setCollision(const CollisionWorld* world) { follow_.setCollision(world); }
    void reset(const float target[3], float yaw, float facing);
    void update(const FollowCameraInput& input);

    // True while the record pose is the top camera command: 0x217b14 returns 0.
    bool locked() const { return !stack_.empty() && stack_.front().kind == Entry::Kind::Record; }
    const float* eye() const { return output_.eye; }
    const float* lookAt() const { return output_.look; }
    float yaw() const { return poseYaw(output_); }
    float pitch() const { return posePitch(output_); }
    // Vertical field of view in radians for the pose FOV (40 for the follow controller, the record FOV otherwise).
    float verticalFieldOfView() const;
    float fieldOfViewDegrees() const { return output_.fovDegrees; }
    std::size_t commandCount() const { return stack_.size(); }
    float weightOfTop() const { return stack_.empty() ? 0.0f : stack_.front().weight; }

private:
    struct Entry {
        enum class Kind { Record, Follow } kind = Kind::Follow;
        CameraRecordSet set;
        float progress = 1.0f;
        float progressPerSecond = 0.0f;
        float weight = 1.0f;
        RecordPose pose;      // last evaluated pose (0x2cb5dc reads it back when the oldest two commands are merged)
        bool frozen = false;  // merged command: no command routine any more, the stored pose stays
    };

    void push(Entry entry, bool clearStack);
    RecordPose entryPose(const Entry& entry, const float target[3]) const;
    static RecordPose weightedPose(const std::vector<Entry>& entries);
    void mergeOldestTwo();

    FollowCamera follow_;
    CameraRecords records_;
    CameraRecordSelector selector_;
    std::vector<Entry> stack_;  // front = newest command
    RecordPose output_;
    float eyeFactor_ = 1.0f;
    float lookFactor_ = 1.0f;
};

}
