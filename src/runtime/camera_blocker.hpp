#pragma once
// Blocker records: the record pose 0x298cd8 returns the pose block 0x58d42c when the first record of its set has the
// Blocker byte +0x4b, and the default preset's post-update 0x181fe0 copies the filtered camera pose C+0x4c into that
// block at the end of every camera update. No SDL dependency.
#include "runtime/camera_records.hpp"

namespace chase::runtime {

bool blocksCamera(const CameraRecords& records, const CameraRecordSet& set);

class CameraBlockerPose {
public:
    void capture(const RecordPose& output) { pose_ = output; }
    const RecordPose& pose() const { return pose_; }

private:
    RecordPose pose_;
};

}
