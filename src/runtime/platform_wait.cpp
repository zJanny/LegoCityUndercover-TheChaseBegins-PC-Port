#include "runtime/platform_wait.hpp"

#include "assets/object_classes.hpp"
#include "runtime/level_start.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>

namespace chase::runtime {
namespace {

constexpr std::int32_t kOneBits = 0x3f800000;  // the immediate #1065353216 of 0x263b98 and 0x263bac

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

}

bool platformReached(const PlatformRecord& record) {
    std::int32_t bits = 0;
    std::memcpy(&bits, &record.value, sizeof bits);
    if (bits == kOneBits || bits >= kOneBits) return true;
    return record.state == kPlatformArrivedState;
}

void PlatformSet::load(const assets::Level& level, const assets::ClassLookup& classOf) {
    records_.clear();
    for (const assets::LevelObject& object : level.objects) {
        const assets::Level* cls = classOf(object.className);
        if (!cls || assets::objectKind(object, *cls) != kPlatformKind) continue;
        PlatformRecord record;
        if (const assets::LevelParam* speed = effectiveParam(object, *cls, assets::nameHash("speed")); speed && speed->type == 0)
            std::memcpy(&record.speed, &speed->value, sizeof record.speed);
        records_[lower(object.instance)] = record;
    }
}

PlatformRecord* PlatformSet::find(const std::string& lowerInstance) {
    const auto found = records_.find(lowerInstance);
    return found == records_.end() ? nullptr : &found->second;
}

const PlatformRecord* PlatformSet::find(const std::string& lowerInstance) const {
    const auto found = records_.find(lowerInstance);
    return found == records_.end() ? nullptr : &found->second;
}

void PlatformSet::triggerStandIn(const std::string& lowerInstance) {
    if (PlatformRecord* record = find(lowerInstance)) record->requested = kPlatformMovingState;
}

void PlatformSet::update(float dt) {
    for (auto& [name, record] : records_) {
        if (record.state == kPlatformMovingState) {
            record.value = record.value + dt * kPlatformStepRate * record.speed;
            if (record.value >= 1.0f) {
                record.value = 1.0f;
                record.requested = kPlatformArrivedState;
            }
        }
        record.state = record.requested;
    }
}

}
