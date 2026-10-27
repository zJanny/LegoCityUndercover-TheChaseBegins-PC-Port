#include "runtime/platform_wait.hpp"

#include "assets/object_classes.hpp"

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

bool travelling(std::uint16_t state) {
    return state == kPlatformMovingState || state == kPlatformReturningState || state == kPlatformControlledState;
}

}

bool platformReached(const PlatformRecord& record) {
    std::int32_t bits = 0;
    std::memcpy(&bits, &record.value, sizeof bits);
    if (bits == kOneBits || bits >= kOneBits) return true;
    return record.state == kPlatformArrivedState;
}

void PlatformSet::load(const assets::Level& level, const assets::ClassLookup& classOf) {
    order_.clear();
    records_.clear();
    movedUpdates_.clear();
    arrivals_.clear();
    laps_.clear();
    carry_.clear();
    for (const assets::LevelObject& object : level.objects) {
        if (const std::optional<assets::Placement> start = assets::objectStart(level, object.instance))
            carry_.setPlacement(lower(object.instance), {start->position[0], start->position[1], start->position[2]});
        const assets::Level* cls = classOf(object.className);
        if (!cls || assets::objectKind(object, *cls) != kPlatformKind) continue;
        const std::string name = lower(object.instance);
        if (records_.emplace(name, platformRecordOf(object, *cls, level)).second) order_.push_back(name);
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

void PlatformSet::message(const std::string& lowerInstance, std::uint8_t message) {
    if (PlatformRecord* record = find(lowerInstance)) platformMessage(*record, message);
}

void PlatformSet::update(float dt) {
    for (const std::string& name : order_) {
        PlatformRecord& record = records_.at(name);
        const bool wasTravelling = travelling(record.state);
        const std::uint16_t stateBefore = record.state;
        const float valueBefore = record.value;
        if (const std::optional<PlatformVector> node = platformUpdate(record, dt)) {
            if (record.rotation) turnNode(name, record, *node);
            else {
                const PlatformVector carried = carry_.offset(name);
                PlatformVector delta{};
                for (int axis = 0; axis < 3; ++axis) delta[axis] = (*node)[axis] - (record.motion.placement[axis] + carried[axis]);
                carry_.move(name, delta);
            }
            ++movedUpdates_[name];
        }
        const bool wrapped = record.motion.path && record.motion.looped && record.state == stateBefore &&
                             ((stateBefore == kPlatformMovingState && record.value < valueBefore) ||
                              (stateBefore == kPlatformReturningState && record.value > valueBefore));
        if (wasTravelling && !travelling(record.state)) arrivals_.push_back(report(name, record));
        else if (wrapped) laps_.push_back(report(name, record));
    }
}

PlatformArrival PlatformSet::report(const std::string& name, const PlatformRecord& record) {
    PlatformArrival arrival;
    arrival.instance = name;
    const PlatformVector carried = carry_.offset(name);
    for (int axis = 0; axis < 3; ++axis) arrival.node[axis] = record.motion.placement[axis] + carried[axis];
    arrival.state = record.state;
    arrival.movedUpdates = movedUpdates_[name];
    movedUpdates_[name] = 0;
    return arrival;
}

std::vector<PlatformArrival> PlatformSet::takeLaps() {
    std::vector<PlatformArrival> laps;
    laps.swap(laps_);
    return laps;
}

void PlatformSet::turnNode(const std::string& name, const PlatformRecord& record, const PlatformVector& node) {
    const PathRotation now = multiplyPathRotation(record.motion.placementRotation, *record.rotation, true);
    const PlatformVector placed = rotatePathPoint(record.motion.placement, now);
    PlatformVector translation{};
    for (std::size_t axis = 0; axis < 3; ++axis) translation[axis] = node[axis] - placed[axis];
    carry_.place(name, now, translation);
}

void PlatformSet::attachObject(const std::string& lowerParent, const std::string& lowerChild) {
    carry_.setParent(lowerChild, lowerParent);
    if (find(lowerParent)) carry_.attach(lowerParent, lowerChild);
}

std::vector<PlatformArrival> PlatformSet::takeArrivals() {
    std::vector<PlatformArrival> arrivals;
    arrivals.swap(arrivals_);
    return arrivals;
}

}
