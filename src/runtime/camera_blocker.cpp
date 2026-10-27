#include "runtime/camera_blocker.hpp"

namespace chase::runtime {

bool blocksCamera(const CameraRecords& records, const CameraRecordSet& set) {
    if (set.records.empty() || set.records[0] == kRoomRecord) return false;
    return records.records()[std::size_t(set.records[0])].blocker;
}

}
