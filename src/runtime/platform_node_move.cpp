#include "runtime/platform_node_move.hpp"

#include "assets/object_classes.hpp"
#include "runtime/level_start.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>

namespace chase::runtime {
namespace {

constexpr std::int32_t kOneBits = 0x3f800000;
constexpr std::uint8_t kRotationStepType = 7;
constexpr std::uint8_t kLiftType = 6;
constexpr std::uint8_t kContinuousType = 0;
constexpr std::uint16_t kLerpShapeCount = 6;
constexpr std::uint8_t kMessageTriggered = 0xff;
constexpr std::uint8_t kMessageUntriggered = 0xfe;

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

const assets::LevelParam* param(const assets::LevelObject& object, const assets::Level& cls, const char* name) {
    return effectiveParam(object, cls, assets::nameHash(name));
}

std::int32_t intParam(const assets::LevelObject& object, const assets::Level& cls, const char* name, std::int32_t fallback) {
    const assets::LevelParam* found = param(object, cls, name);
    if (!found) return fallback;
    if (found->type == 0) {
        float value = 0;
        std::memcpy(&value, &found->value, 4);
        return std::int32_t(value);
    }
    return std::int32_t(found->value);
}

float floatParam(const assets::LevelObject& object, const assets::Level& cls, const char* name, float fallback) {
    const assets::LevelParam* found = param(object, cls, name);
    if (!found || found->type != 0) return fallback;
    float value = 0;
    std::memcpy(&value, &found->value, 4);
    return value;
}

std::string textParam(const assets::LevelObject& object, const assets::Level& cls, const char* name) {
    const assets::LevelParam* found = param(object, cls, name);
    if (!found || found->blocks.empty()) return {};
    const assets::Bytes& block = found->blocks[0];
    const auto end = std::find(block.begin(), block.end(), 0);
    return std::string(block.begin(), end);
}

bool atOrAboveOne(float value) {
    std::int32_t bits = 0;
    std::memcpy(&bits, &value, sizeof bits);
    return bits >= kOneBits;
}

bool movingState(std::uint16_t state) {
    return state == kPlatformMovingState || state == kPlatformReturningState || state == kPlatformControlledState;
}

void endOfTravelForwards(PlatformRecord& record, float unclamped, bool& backwards) {
    PlatformMotion& motion = record.motion;
    record.value = 1.0f;
    switch (motion.type) {
    case kContinuousType:
        if (++motion.continuousCount < motion.continuousWait) return;
        [[fallthrough]];
    case 2:
    case 3:
        motion.continuousCount = 0;
        if (motion.looped) {
            record.value = unclamped - 1.0f;
            return;
        }
        record.value = 2.0f - record.value;
        record.requested = kPlatformReturningState;
        backwards = true;
        return;
    default: record.requested = kPlatformArrivedState; return;
    }
}

void endOfTravelBackwards(PlatformRecord& record, float unclamped) {
    PlatformMotion& motion = record.motion;
    record.value = 0.0f;
    switch (motion.type) {
    case kContinuousType:
        if (++motion.continuousCount < motion.continuousWait) return;
        [[fallthrough]];
    case 3:
        motion.continuousCount = 0;
        if (motion.looped) {
            record.value = unclamped + 1.0f;
            return;
        }
        record.requested = kPlatformMovingState;
        record.value = -record.value;
        return;
    case 2:
    case 4: record.requested = kPlatformArrivedState; return;
    default: record.requested = kPlatformStoppedState; return;
    }
}

}

PlatformRecord platformRecordOf(const assets::LevelObject& object, const assets::Level& cls, const assets::Level& level) {
    PlatformRecord record;
    PlatformMotion& motion = record.motion;
    record.speed = floatParam(object, cls, "speed", kPlatformSpeedDefault);
    motion.type = std::uint8_t(intParam(object, cls, "type", 0));
    motion.orient = std::uint8_t(intParam(object, cls, "orient", 0) & 3);
    motion.rotateOnly = (intParam(object, cls, "rotateonly", 0) & 1) != 0;
    motion.looped = (intParam(object, cls, "Looped", 0) & 1) != 0;
    motion.stopOnUntrigger = (intParam(object, cls, "StopOnUntrigger", 0) & 1) != 0;
    const std::int32_t shape = intParam(object, cls, "LerpShape", 0);
    motion.lerpShape = shape >= 0 && shape < kLerpShapeCount ? std::uint16_t(shape) : std::uint16_t(0);
    motion.lerpSameBackwards = (intParam(object, cls, "LerpSameBackwards", 1) & 1) != 0;
    motion.resetProgressOnTrigger = (intParam(object, cls, "ResetProgressOnTrigger", 1) & 1) != 0;
    if (motion.type == kContinuousType) {
        const std::uint32_t frames = std::uint32_t(std::int32_t(floatParam(object, cls, "continuuswaittime", 0.0f) * kPlatformStepRate));
        motion.continuousWait = frames >= kPlatformWaitLimit ? kPlatformWaitLimit : std::uint16_t(std::max<std::uint32_t>(frames, 1));
        record.state = record.requested = kPlatformMovingState;
    }
    if (intParam(object, cls, "Controlled", 0) != 0) record.state = record.requested = kPlatformControlledState;

    if (const std::optional<assets::Placement> start = assets::objectStart(level, object.instance))
        std::copy_n(start->position, 3, motion.placement.begin());
    motion.origin = motion.target = motion.placement;
    const std::string pathName = textParam(object, cls, "Platform_Path");
    if (!pathName.empty()) {
        const std::uint32_t hash = assets::nameHash(lower(pathName));
        motion.path = std::any_of(level.paths.begin(), level.paths.end(), [&](const assets::LevelPath& path) { return path.nameHash == hash; });
    }
    if (const assets::LevelParam* to = param(object, cls, "ObjectTo"); to && to->type == 4 && !motion.path) {
        const std::uint32_t hash = assets::objectReferenceHash(*to);
        for (const assets::LevelObject& other : level.objects)
            if (hash != 0 && assets::nameHash(lower(other.instance)) == hash)
                if (const std::optional<assets::Placement> start = assets::objectStart(level, other.instance)) {
                    std::copy_n(start->position, 3, motion.target.begin());
                    break;
                }
    }
    return record;
}

void platformRequest(PlatformRecord& record, bool backwards, float progress) {
    const std::uint8_t type = record.motion.type;
    const bool returning = type == 1 || type == 5 || type == kLiftType;
    if (backwards) {
        if (returning && (record.requested == 1 || record.requested == kPlatformMovingState || record.requested == kPlatformArrivedState))
            record.requested = kPlatformReturningState;
        if (type == kRotationStepType) return;
    } else {
        switch (record.requested) {
        case kPlatformStoppedState:
        case 1: record.requested = kPlatformMovingState; break;
        case kPlatformReturningState:
            if (type == kRotationStepType || returning) record.requested = kPlatformMovingState;
            break;
        case kPlatformArrivedState:
            if (type == kRotationStepType) return;
            if (type != 1) record.requested = kPlatformMovingState;
            break;
        default: break;
        }
        if (type == kRotationStepType) return;
    }
    if (record.motion.resetProgressOnTrigger) record.value = progress;
}

void platformMessage(PlatformRecord& record, std::uint8_t message) {
    if (message == kMessageTriggered) {
        if (record.motion.type == kLiftType && record.value > 0.0f) return;
        platformRequest(record, false, record.motion.stopOnUntrigger ? record.value : 0.0f);
    } else if (message == kMessageUntriggered) {
        if (record.motion.stopOnUntrigger) {
            record.requested = kPlatformStoppedState;
            return;
        }
        if (record.motion.type == kLiftType || record.motion.type == kRotationStepType) return;
        platformRequest(record, true, 1.0f);
    }
}

static float shapeStandIn(std::uint16_t shape, float value) {
    (void)shape;
    return value;
}

float platformShape(const PlatformRecord& record, float value, bool backwards) {
    const std::uint16_t shape = record.motion.lerpShape;
    if (shape == 0) return value;
    if (backwards && record.motion.lerpSameBackwards) return 1.0f - shapeStandIn(shape, 1.0f - value);
    return shapeStandIn(shape, value);
}

std::optional<PlatformVector> platformUpdate(PlatformRecord& record, float dt) {
    const float step = dt * kPlatformStepRate * record.speed;
    bool backwards = false;
    std::optional<PlatformVector> node;
    if (movingState(record.state)) {
        if (record.state == kPlatformMovingState) record.value = record.value + step;
        if (record.state == kPlatformReturningState) {
            record.value = record.value - step;
            backwards = true;
        }
    }
    if (movingState(record.state) && record.motion.type != kRotationStepType) {
        if (record.state != kPlatformControlledState) {
            const float unclamped = record.value;
            if (atOrAboveOne(unclamped)) endOfTravelForwards(record, unclamped, backwards);
            else if (!(unclamped > 0.0f)) endOfTravelBackwards(record, unclamped);
        }
        const float shaped = record.requested == kPlatformStoppedState ? record.value : platformShape(record, record.value, backwards);
        const PlatformMotion& motion = record.motion;
        if (!motion.path && !motion.rotateOnly) {
            PlatformVector position{};
            for (int axis = 0; axis < 3; ++axis) position[axis] = motion.origin[axis] + (motion.target[axis] - motion.origin[axis]) * shaped;
            node = position;
        }
    }
    record.previous = record.value;
    record.state = record.requested;
    return node;
}

void NodeCarry::clear() {
    parents_.clear();
    children_.clear();
    attached_.clear();
    offsets_.clear();
    moved_.clear();
}

void NodeCarry::setParent(const std::string& child, const std::string& parent) {
    const auto old = parents_.find(child);
    if (old != parents_.end()) {
        std::vector<std::string>& siblings = children_[old->second];
        siblings.erase(std::remove(siblings.begin(), siblings.end(), child), siblings.end());
        parents_.erase(old);
    }
    if (parent.empty() || parent == child) return;
    parents_[child] = parent;
    children_[parent].push_back(child);
}

void NodeCarry::attach(const std::string& platform, const std::string& child) {
    std::vector<std::string>& list = attached_[platform];
    if (std::find(list.begin(), list.end(), child) != list.end() || list.size() >= kPlatformAttachedMax) return;
    list.push_back(child);
}

void NodeCarry::move(const std::string& name, const PlatformVector& delta) {
    if (delta[0] == 0.0f && delta[1] == 0.0f && delta[2] == 0.0f) return;
    std::vector<std::string> pending{name};
    std::vector<std::string> seen;
    while (!pending.empty()) {
        const std::string current = pending.back();
        pending.pop_back();
        if (std::find(seen.begin(), seen.end(), current) != seen.end()) continue;
        seen.push_back(current);
        PlatformVector& offset = offsets_[current];
        for (int axis = 0; axis < 3; ++axis) offset[axis] += delta[axis];
        if (std::find(moved_.begin(), moved_.end(), current) == moved_.end()) moved_.push_back(current);
        const auto children = children_.find(current);
        if (children != children_.end()) pending.insert(pending.end(), children->second.rbegin(), children->second.rend());
    }
}

PlatformVector NodeCarry::offset(const std::string& name) const {
    const auto found = offsets_.find(name);
    return found == offsets_.end() ? PlatformVector{} : found->second;
}

const std::vector<std::string>& NodeCarry::attached(const std::string& platform) const {
    static const std::vector<std::string> none;
    const auto found = attached_.find(platform);
    return found == attached_.end() ? none : found->second;
}

const std::string* NodeCarry::parent(const std::string& child) const {
    const auto found = parents_.find(child);
    return found == parents_.end() ? nullptr : &found->second;
}

std::vector<std::string> NodeCarry::takeMoved() {
    std::vector<std::string> moved;
    moved.swap(moved_);
    return moved;
}

}
