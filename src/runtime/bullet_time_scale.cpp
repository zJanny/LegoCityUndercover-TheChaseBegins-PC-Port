#include "runtime/bullet_time_scale.hpp"

#include <cmath>
#include <limits>

namespace chase::runtime {

BulletTimeModeCall bulletTimeModeCall(std::uint8_t mode) {
    switch (mode) {
    case 0:
    case 4: return BulletTimeModeCall::GameplayRecords;
    case 3: return BulletTimeModeCall::RotFixedPreset;
    default: return BulletTimeModeCall::None;
    }
}

std::uint8_t bulletTimeSlowingMode(bool flag, bool hasOwner, std::optional<std::uint8_t> shotRecordMode) {
    if (flag && hasOwner && shotRecordMode) return *shotRecordMode;
    return kBulletTimeNoMode;
}

bool bulletTimeRestoreRunsNamedObject(bool found, std::uint16_t flags) { return found && (flags & 1) != 0; }

bool applyTimeScaleToCount(ScaledCountRecord& record, float timeScale) {
    if (timeScale == 1.0f) return true;
    const float product = static_cast<float>(record.count) * timeScale;
    if (std::isnan(product)) record.count = 0;
    else if (product >= 2147483648.0f) record.count = std::numeric_limits<std::int32_t>::max();
    else if (product <= -2147483648.0f) record.count = std::numeric_limits<std::int32_t>::min();
    else record.count = static_cast<std::int32_t>(product);
    record.flags = static_cast<std::uint16_t>(record.flags | kScaledCountChangedBit);
    return true;
}

}
