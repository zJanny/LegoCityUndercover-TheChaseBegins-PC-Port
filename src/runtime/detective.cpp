#include "runtime/detective.hpp"
#include "runtime/camera_reset_probe.hpp"
#include "runtime/dcam.hpp"
#include "runtime/level_start.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <map>
#include <optional>
#include <utility>

namespace chase::runtime {
namespace {

constexpr int kNoClip = 0x271;
constexpr std::size_t kRolesSheet = 2;
constexpr std::uint32_t kObjectTypeHash = 0x06fef256;
constexpr float kTinyParameter = 1.1920928955078125e-07f;  // 0x22a4c0 converts only fractions whose bits exceed 0x33ffffff
constexpr float kTinyLength = 1.1920928955078125e-07f;     // 0x257ea0 keeps (0, 0, 1) for lengths whose bits are at most 0x34000000

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

std::string referenceName(const assets::LevelObject& object, const assets::Level& cls, const char* name, const std::map<std::uint32_t, std::string>& names) {
    const assets::LevelParam* param = effectiveParam(object, cls, assets::nameHash(name));
    if (!param) return {};
    const auto found = names.find(assets::objectReferenceHash(*param));
    return found == names.end() ? std::string() : found->second;
}

std::optional<float> floatParam(const assets::LevelObject& object, const assets::Level& cls, const char* name) {
    const assets::LevelParam* param = effectiveParam(object, cls, assets::nameHash(name));
    if (!param || param->type != 0) return std::nullopt;
    float value;
    std::memcpy(&value, &param->value, 4);
    return value;
}

int intParam(const assets::LevelObject& object, const assets::Level& cls, const char* name, int fallback) {
    const assets::LevelParam* param = effectiveParam(object, cls, assets::nameHash(name));
    if (!param) return fallback;
    if (param->type == 0) {
        float value;
        std::memcpy(&value, &param->value, 4);
        return int(value);
    }
    if (param->type == 5 || param->type == 7) return int(param->value);
    return fallback;
}

std::string stringParam(const assets::LevelObject& object, const assets::Level& cls, const char* name) {
    const assets::LevelParam* param = effectiveParam(object, cls, assets::nameHash(name));
    if (!param || param->type != 1 || param->blocks.empty()) return {};
    const assets::Bytes& block = param->blocks.front();
    return std::string(block.begin(), std::find(block.begin(), block.end(), std::uint8_t(0)));
}

DetectiveBox classBox(const assets::Level& cls, const char* name) {
    DetectiveBox box;
    for (const assets::LevelBound& bound : cls.bounds)
        if (lower(bound.name) == lower(name)) {
            std::copy_n(bound.centre, 3, box.centre);
            std::copy_n(bound.half, 3, box.half);
            box.present = true;
            break;
        }
    return box;
}

std::uint32_t objectType(const assets::LevelObject& object, const assets::Level& cls) {
    const assets::LevelParam* param = effectiveParam(object, cls, kObjectTypeHash);
    return param ? param->value : 0;
}

int wrapIndex(int index, int count) {
    const int wrapped = index % count;
    return wrapped < 0 ? wrapped + count : wrapped;
}

float distance(const float a[3], const float b[3]) {
    const float dx = a[0] - b[0], dy = a[1] - b[1], dz = a[2] - b[2];
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

float segmentLength(const assets::LevelPath& path, unsigned steps, unsigned from, unsigned to) {
    const unsigned count = unsigned(path.points.size());
    if (to < from) to += count;
    float previous[3], current[3];
    sampleDirectorPath(path, float(from), previous);
    float length = 0.0f;
    for (unsigned index = from; index != to; ++index) {
        const unsigned point = index >= count ? index - count : index;
        for (unsigned step = 0; step < steps; ++step) {
            sampleDirectorPath(path, float(step) / float(steps) + float(point), current);
            length = distance(previous, current) + length;
            std::copy_n(current, 3, previous);
        }
    }
    return length;
}

}

std::vector<float> detectivePathLengths(const assets::LevelPath& path, unsigned steps) {
    std::vector<float> lengths;
    if (path.points.empty()) return lengths;
    const unsigned entries = unsigned(path.points.size()) + (path.closed ? 1u : 0u);
    lengths.push_back(0.0f);
    float total = 0.0f;
    for (unsigned index = 1; index < entries; ++index) {
        total = total + segmentLength(path, steps, index - 1, index);
        lengths.push_back(total);
    }
    return lengths;
}

float detectivePathParameter(const assets::LevelPath& path, const std::vector<float>& lengths, float t) {
    if (path.points.empty() || lengths.empty()) return 0.0f;
    t = std::fabs(t);
    std::uint32_t bits;
    std::memcpy(&bits, &t, 4);
    if (bits == 0x3f7fffffu || bits == 0x3f800000u) return float(path.points.size());
    if (!(t > kTinyParameter)) return 0.0f;
    const float target = std::fmod(t, 1.0f) * lengths.back();
    unsigned low = 0, high = unsigned(path.points.size()) - 1, middle = 0;
    float below = 0.0f, above = 0.0f;
    do {
        middle = (low + high) >> 1;
        below = lengths[middle];
        above = middle + 1 < lengths.size() ? lengths[middle + 1] : lengths.back();
        if (!(target < below) && !(above <= target)) break;
        if (!(target < below)) low = (middle + 1) & 0xffffu;
        else high = (middle - 1) & 0xffffu;
    } while (low <= high && high < lengths.size());
    return float(int(middle)) + (target - below) / (above - below);
}

void detectivePathTangent(const assets::LevelPath& path, float position, float out[3]) {
    const int count = int(path.points.size());
    out[0] = out[1] = 0.0f;
    out[2] = 1.0f;
    if (count == 0) return;
    if (!path.closed) {
        if (!(position >= 0.0f)) position = 0.0f;
        if (!(position <= float(count - 1))) position = float(count - 1);
    }
    const int base = int(std::floor(position));
    int before = base - 1, after = base + 1, beyond = base + 2, at = base;
    if (!path.closed) {
        before = std::max(before, 0);
        after = std::min(after, count - 1);
        beyond = std::min(beyond, count - 1);
    } else {
        before = wrapIndex(before, count);
        at = wrapIndex(at, count);
        after = wrapIndex(after, count);
        beyond = wrapIndex(beyond, count);
    }
    const float t = position - float(base);
    const float square = t * t;
    float derivative[3];
    for (int a = 0; a < 3; ++a) {
        const float p1 = path.points[std::size_t(at)][std::size_t(a)];
        const float p2 = path.points[std::size_t(after)][std::size_t(a)];
        const float p0 = path.points[std::size_t(before)][std::size_t(a)];
        const float p3 = path.points[std::size_t(beyond)][std::size_t(a)];
        derivative[a] = p1 * (square * 6.0f - t * 6.0f) + (square * -6.0f + t * 6.0f) * p2 + (p2 - p0) * ((square * 3.0f - t * 4.0f) + 1.0f) * 0.5f +
                        (p3 - p1) * (square * 3.0f - t * 2.0f) * 0.5f;
    }
    float squared = derivative[0] * derivative[0];
    squared = squared + derivative[1] * derivative[1];
    squared = squared + derivative[2] * derivative[2];
    const float length = std::sqrt(squared);
    if (!(length > kTinyLength)) {
        out[0] = out[1] = 0.0f;
        out[2] = 1.0f;
        return;
    }
    const float inverse = 1.0f / length;
    for (int a = 0; a < 3; ++a) out[a] = derivative[a] * inverse;
}

void detectivePathOrientation(const assets::LevelPath& path, float position, float out[4]) {
    const int count = int(path.points.size());
    if (path.orientations.empty()) {
        out[0] = out[1] = out[2] = 0.0f;
        out[3] = 1.0f;
        return;
    }
    if (!path.closed) {
        if (position < 0.0f) position = 0.0f;
        if (position > float(count - 1)) position = float(count - 1);
    }
    const int floored = int(std::floor(position));
    const float t = position - float(floored);
    int from = floored, to = floored + 1;
    if (path.closed) {
        from = floored % count;
        if (from < 0) from += count;
    } else if (to > count - 1) {
        to = count - 1;
    }
    const std::size_t last = path.orientations.size() - 1;
    const std::array<float, 4>& a = path.orientations[std::min<std::size_t>(std::size_t(std::max(from, 0)), last)];
    const std::array<float, 4>& b = path.orientations[std::min<std::size_t>(std::size_t(std::max(to, 0)), last)];
    float dot = a[0] * b[0];
    dot = dot + a[1] * b[1];
    dot = dot + a[2] * b[2];
    dot = dot + a[3] * b[3];
    float sign = 1.0f;
    if (dot < 0.0f) {
        dot = -dot;
        sign = -1.0f;
    }
    float weightFrom = 1.0f - t, weightTo;
    std::int32_t dotBits;
    std::memcpy(&dotBits, &dot, 4);
    if (dotBits > 0x3f7fff58) {
        weightTo = sign * t;
    } else {
        const float theta = std::acos(dot);
        const float sine = std::sin(theta);
        weightFrom = std::sin((1.0f - t) * theta) / sine;
        weightTo = std::sin(t * theta) / sine * sign;
    }
    for (int k = 0; k < 4; ++k) out[k] = a[std::size_t(k)] * weightFrom + weightTo * b[std::size_t(k)];
}

std::optional<float> footprintGroundHeight(const CollisionWorld& world, const float point[3]) {
    const float from[3] = {point[0], point[1] + 1.0f, point[2]};
    const float to[3] = {point[0], point[1] - 1.0f, point[2]};
    if (const auto hit = CameraResetProbe(world, "").nearestHit(from, to)) return (*hit)[1];
    return std::nullopt;
}

std::array<float, 16> detectiveFootprintMatrix(const std::array<std::int8_t, 4>& rotation, const float translation[3]) {
    const float x = float(rotation[0]) * kFootprintRotationInverse, y = float(rotation[1]) * kFootprintRotationInverse,
                z = float(rotation[2]) * kFootprintRotationInverse, w = float(rotation[3]) * kFootprintRotationInverse;
    const float xx = x * (x + x), wx = w * (x + x), yy = y * (y + y), zz = z * (z + z), yz = y * (z + z), xy = x * (y + y), xz = x * (z + z),
                wz = w * (z + z), wy = w * (y + y);
    std::array<float, 16> m = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    m[0] = 1.0f - (yy + zz);
    m[1] = xy - wz;
    m[2] = xz + wy;
    m[4] = xy + wz;
    m[5] = 1.0f - (xx + zz);
    m[6] = yz - wx;
    m[8] = xz - wy;
    m[9] = yz + wx;
    m[10] = 1.0f - (xx + yy);
    for (int a = 0; a < 3; ++a) m[std::size_t(12 + a)] = translation[a];
    return m;
}

std::vector<DetectiveFootprintRecord> detectiveFootprints(const assets::LevelPath& path, const float spot[3]) {
    std::vector<DetectiveFootprintRecord> footprints;
    const std::vector<float> lengths = detectivePathLengths(path, kPathLengthSteps);
    if (lengths.empty()) return footprints;
    const unsigned count = unsigned(int(std::ceil(lengths.back() * kFootprintsPerUnit)));
    for (unsigned index = 0; index < count; ++index) {
        const float position = detectivePathParameter(path, lengths, float(index) / float(count));
        float point[3], tangent[3], orientation[4];
        sampleDirectorPath(path, position, point);
        detectivePathTangent(path, position, tangent);
        detectivePathOrientation(path, position, orientation);
        const float side = (index & 1u) ? kFootprintSide : -kFootprintSide;
        const float across[3] = {tangent[2], tangent[1], tangent[0]};
        DetectiveFootprintRecord record;
        for (int a = 0; a < 3; ++a) {
            point[a] = point[a] + across[a] * side;
            record.offset[std::size_t(a)] = std::int16_t(int((point[a] - spot[a]) * kFootprintUnitsPerStep));
        }
        for (int a = 0; a < 4; ++a) record.rotation[std::size_t(a)] = std::int8_t(int(orientation[a] * kFootprintRotationScale));
        footprints.push_back(record);
    }
    return footprints;
}

bool DetectiveSet::inBox(const DetectiveBox& box, const float origin[3], const float point[3]) {
    if (!box.present) return false;
    for (int a = 0; a < 3; ++a)
        if (std::fabs(point[a] - origin[a] - box.centre[a]) > box.half[a]) return false;
    return true;
}

void DetectiveSet::load(const assets::Level& level, const assets::ClassLookup& classOf) {
    spots_.clear();
    revealSpots_.clear();
    revealProps_.clear();
    footprintPool_.assign(kFootprintPoolSize, false);
    playerState_ = DetectivePlayerState::Normal;
    playerUpdates_ = 0;
    modeActive_ = false;
    useSpot_ = -1;
    std::map<std::uint32_t, std::string> names;
    for (const assets::LevelObject& object : level.objects) names[assets::nameHash(object.instance)] = lower(object.instance);
    for (std::size_t index = 0; index < level.objects.size(); ++index) {
        const assets::LevelObject& object = level.objects[index];
        const assets::Level* cls = classOf(object.className);
        if (!cls) continue;
        const std::uint32_t type = objectType(object, *cls);
        if (type != kDetectiveSpotType && type != kRevealSpotType && type != kRevealPropType) continue;
        const std::optional<assets::Placement> placed = assets::objectStart(level, object.instance);
        if (!placed) continue;
        if (type == kDetectiveSpotType) {
            DetectiveSpot spot;
            spot.instance = lower(object.instance);
            spot.objectIndex = index;
            std::copy_n(placed->position, 3, spot.position);
            spot.useZone = classBox(*cls, "UseBound");
            spot.questionZone = classBox(*cls, "QuestionBound");
            spot.revealSwitch = referenceName(object, *cls, "RevealSwitch", names);
            spot.triggerObject = referenceName(object, *cls, "TriggerObject", names);
            spot.footprintType = unsigned(std::max(0, intParam(object, *cls, "FootprintType", 0)));
            if (const assets::LevelPath* path = findLevelPath(level, stringParam(object, *cls, "DetectivePath")))
                for (const DetectiveFootprintRecord& record : detectiveFootprints(*path, spot.position))
                    spot.footprints.push_back({record.offset, record.rotation, -1, 0});
            spots_.push_back(std::move(spot));
        } else if (type == kRevealSpotType) {
            RevealSpot spot;
            spot.instance = lower(object.instance);
            spot.objectIndex = index;
            std::copy_n(placed->position, 3, spot.position);
            spot.useZone = classBox(*cls, "UseBound");
            spot.revealTime = floatParam(object, *cls, "RevealTime").value_or(kRevealSpotDefaultTime);
            spot.fadeObject = referenceName(object, *cls, "FadeObject", names);
            spot.revealObject = referenceName(object, *cls, "RevealObject", names);
            spot.triggerObject = referenceName(object, *cls, "TriggerObject", names);
            revealSpots_.push_back(std::move(spot));
        } else {
            RevealProp prop;
            prop.instance = lower(object.instance);
            prop.objectIndex = index;
            prop.revealObject = referenceName(object, *cls, "RevealObject", names);
            prop.triggerObject = referenceName(object, *cls, "TriggerObject", names);
            prop.delayUpdates = intParam(object, *cls, "RevealDelay", kRevealPropDefaultDelay) * kRevealFramesPerSecond;
            prop.flightUpdates = int(floatParam(object, *cls, "RevealTime").value_or(kRevealPropDefaultTime) * float(kRevealFramesPerSecond));
            prop.revealType = intParam(object, *cls, "RevealType", 0);
            prop.soundSpawn = std::uint16_t(intParam(object, *cls, "ATTR_SFXObjectSpawn", 0));
            prop.soundDone = std::uint16_t(intParam(object, *cls, "ATTR_SFXObjectDone", 0));
            prop.bounceUpdates = int(float(kRevealFramesPerSecond) * 0.2f);
            revealProps_.push_back(std::move(prop));
        }
    }
}

void DetectiveSet::triggerMessage(const std::string& lowerInstance) {
    for (RevealProp& prop : revealProps_)
        if (prop.instance == lowerInstance && prop.state == 0) prop.requested = 1;
}

void DetectiveSet::activate(const std::string& target, std::vector<DetectiveAction>& out) {
    if (target.empty()) return;
    DetectiveAction action;
    action.kind = DetectiveActionKind::Activate;
    action.instance = target;
    out.push_back(action);
    triggerMessage(target);
}

int DetectiveSet::playerClip() const {
    switch (playerState_) {
    case DetectivePlayerState::Out: return kAnimationDetectOut;
    case DetectivePlayerState::In: return kAnimationDetectIn;
    case DetectivePlayerState::Use: return kAnimationDetectFind;
    case DetectivePlayerState::Normal: break;
    }
    return 0;
}

void DetectiveSet::enterPlayerState(DetectivePlayerState next, bool fromObject) {
    playerState_ = next;
    playerUpdates_ = 0;
    enteredByObject_ = fromObject;
    if (next == DetectivePlayerState::Out) modeActive_ = true;
    if (next == DetectivePlayerState::In) modeActive_ = false;
}

void DetectiveSet::revealFootprint(DetectiveSpot& spot, std::size_t index) {
    DetectiveFootprint& footprint = spot.footprints[index];
    if (footprint.slot != -1) {
        footprint.alpha = 0xff;
        return;
    }
    const unsigned first = spot.footprintType * kFootprintPoolPerType;
    for (unsigned slot = first; slot < first + kFootprintPoolPerType && slot < footprintPool_.size(); ++slot)
        if (!footprintPool_[slot]) {
            footprintPool_[slot] = true;
            footprint.slot = int(slot);
            footprint.alpha = 0xff;
            return;
        }
}

void DetectiveSet::fadeFootprints(DetectiveSpot& spot) {
    for (DetectiveFootprint& footprint : spot.footprints) {
        if (footprint.slot == -1 || footprint.alpha == 0) continue;
        if (int(footprint.alpha) - int(kFootprintFadeStep) < 2) {
            footprint.alpha = 0;
            footprintPool_[std::size_t(footprint.slot)] = false;
            footprint.slot = -1;
        } else {
            footprint.alpha = std::uint8_t(footprint.alpha - kFootprintFadeStep);
        }
    }
}

bool DetectiveSet::scan(DetectiveSpot& spot, const DetectiveInput& input) {
    if (!modeActive_) return false;
    bool near = false;
    for (std::size_t index = 0; index < spot.footprints.size(); ++index) {
        float d[3];
        for (int a = 0; a < 3; ++a) d[a] = input.player[a] - (spot.position[a] + float(spot.footprints[index].offset[std::size_t(a)]) * 0.25f);
        const float squared = d[0] * d[0] + d[2] * d[2];
        const float ahead = input.forward[0] * d[0] + input.forward[1] * d[1] + input.forward[2] * d[2];
        if (!(0.0f <= ahead) && squared < kFootprintRevealDistanceSquared) revealFootprint(spot, index);
        if (squared < kFootprintNearDistanceSquared) near = true;
    }
    const RevealSpot* target = nullptr;
    for (const RevealSpot& reveal : revealSpots_)
        if (reveal.instance == spot.revealSwitch) target = &reveal;
    if (!spot.revealSwitch.empty() && target) {
        float d[3];
        for (int a = 0; a < 3; ++a) d[a] = input.player[a] - target->position[a];
        const float ahead = input.forward[0] * d[0] + input.forward[1] * d[1] + input.forward[2] * d[2];
        if (!(0.0f <= ahead) && d[0] * d[0] + d[2] * d[2] < kFootprintRevealDistanceSquared) {
            spot.requested = 2;
            return true;
        }
    }
    if (spot.requested != 2 && !near) spot.requested = 0;
    return true;
}

void DetectiveSet::updateSpot(DetectiveSpot& spot, const DetectiveInput& input, std::vector<DetectiveAction>& out) {
    if (spot.requested != spot.state) {
        if (spot.state == 0) {
            spot.questionShown = false;
        } else if (spot.state == 1) {
            if (spot.user) {
                if (playerState_ != DetectivePlayerState::In) enterPlayerState(DetectivePlayerState::In, true);
                spot.user = false;
            }
        } else if (spot.state == 2) {
            activate(spot.triggerObject, out);
        }
        if (spot.requested == 2) {
            DetectiveAction sound;
            sound.kind = DetectiveActionKind::Sound;
            sound.sound = kSoundDetectiveRevealed;
            std::copy_n(spot.position, 3, sound.position);
            out.push_back(sound);
        }
        spot.state = spot.requested;
    }
    if (spot.state == 0) {
        spot.questionShown = inBox(spot.questionZone, spot.position, input.player);
    } else if (spot.state == 1) {
        if (!scan(spot, input)) spot.requested = 0;
    } else if (spot.state == 2) {
        if (!spot.revealSwitch.empty()) {
            DetectiveAction show;
            show.kind = DetectiveActionKind::Show;
            show.instance = spot.revealSwitch;
            out.push_back(show);
        }
        const bool active = std::any_of(spot.footprints.begin(), spot.footprints.end(), [](const DetectiveFootprint& f) { return f.slot != -1 && f.alpha != 0; });
        if (!active) {
            DetectiveAction hide;
            hide.kind = DetectiveActionKind::Hide;
            hide.instance = spot.instance;
            out.push_back(hide);
            spot.usable = false;
        }
    }
    fadeFootprints(spot);
}

void DetectiveSet::updateRevealSpot(RevealSpot& spot, const DetectiveInput& input, std::vector<DetectiveAction>& out) {
    if (spot.state == spot.requested && spot.state == 1) {
        if (spot.timer <= 0.0f) spot.requested = 2;
        else spot.timer = spot.timer - input.dt;
    }
    if (spot.state == spot.requested) return;
    if (spot.requested == 1) {
        if (!spot.fadeObject.empty() && spot.fadeObject != spot.revealObject) {
            DetectiveAction hide;
            hide.kind = DetectiveActionKind::Hide;
            hide.instance = spot.fadeObject;
            out.push_back(hide);
        }
        if (!spot.revealObject.empty()) {
            DetectiveAction show;
            show.kind = DetectiveActionKind::Show;
            show.instance = spot.revealObject;
            out.push_back(show);
        }
        spot.timer = spot.revealTime;
    } else if (spot.requested == 2) {
        DetectiveAction hide;
        hide.kind = DetectiveActionKind::Hide;
        hide.instance = spot.instance;
        out.push_back(hide);
        activate(std::exchange(spot.revealObject, std::string()), out);
        activate(std::exchange(spot.triggerObject, std::string()), out);
    }
    spot.state = spot.requested;
}

void DetectiveSet::updateRevealProp(RevealProp& prop, std::vector<DetectiveAction>& out) {
    if (prop.state == prop.requested) {
        if (prop.state == 1) {
            ++prop.counter;
            if (prop.counter >= prop.delayUpdates) prop.requested = 2;
        }
        if (prop.flightMode == 1 || prop.flightMode == 2) {
            if (prop.flightCounter == 0 || --prop.flightCounter == 0) {
                DetectiveAction sound;
                sound.kind = DetectiveActionKind::Sound;
                sound.sound = prop.soundDone;
                sound.instance = prop.revealObject;
                out.push_back(sound);
                prop.flightMode = prop.flightMode == 1 ? 6 : 3;
                prop.flightCounter = prop.bounceUpdates - 1;
                activate(prop.triggerObject, out);
            }
        } else if (prop.flightMode >= 3 && prop.flightMode <= 5) {
            if (prop.flightCounter != 0) --prop.flightCounter;
            if (prop.flightCounter == 0) {
                prop.flightCounter = prop.bounceUpdates - 1;
                if (++prop.flightMode == 6) activate(prop.triggerObject, out);
            }
        }
    }
    if (prop.state == prop.requested) return;
    if (prop.requested == 1) {
        prop.counter = 0;
    } else if (prop.requested == 2 && !prop.revealObject.empty() && prop.flightMode == 0) {
        prop.flightMode = prop.revealType == 0 ? 2 : (prop.revealType == 1 ? 1 : 0);
        prop.flightCounter = prop.flightUpdates;
        DetectiveAction show;
        show.kind = DetectiveActionKind::Show;
        show.instance = prop.revealObject;
        out.push_back(show);
        DetectiveAction sound;
        sound.kind = DetectiveActionKind::Sound;
        sound.sound = prop.soundSpawn;
        sound.instance = prop.revealObject;
        out.push_back(sound);
    }
    prop.state = prop.requested;
}

void DetectiveSet::updatePlayer(const assets::PlacedObjectState& state, const DetectiveInput& input) {
    if (std::exchange(enteredByObject_, false)) return;
    if (playerState_ != DetectivePlayerState::Normal) {
        ++playerUpdates_;
        if (playerState_ == DetectivePlayerState::Use && playerUpdates_ == kXSpotUseEventUpdate && useSpot_ >= 0)
            revealSpots_[std::size_t(useSpot_)].requested = 1;
        const unsigned end = playerState_ == DetectivePlayerState::Out ? kDetectiveOutUpdates
                             : playerState_ == DetectivePlayerState::In ? kDetectiveInUpdates
                                                                        : kXSpotUseUpdates;
        if (playerUpdates_ >= end) {
            playerState_ = DetectivePlayerState::Normal;
            playerUpdates_ = 0;
            useSpot_ = -1;
        }
        return;
    }
    if (!input.actionPressed || !input.playerFree) return;
    std::vector<std::size_t> order;
    for (std::size_t i = 0; i < spots_.size(); ++i) order.push_back(i);
    for (std::size_t i = 0; i < revealSpots_.size(); ++i) order.push_back(spots_.size() + i);
    std::sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
        const std::size_t ia = a < spots_.size() ? spots_[a].objectIndex : revealSpots_[a - spots_.size()].objectIndex;
        const std::size_t ib = b < spots_.size() ? spots_[b].objectIndex : revealSpots_[b - spots_.size()].objectIndex;
        return ia < ib;
    });
    for (const std::size_t entry : order) {
        if (entry < spots_.size()) {
            DetectiveSpot& spot = spots_[entry];
            if (state.isDisabled(spot.instance) || !spot.usable || spot.state != 0 || !inBox(spot.useZone, spot.position, input.player)) continue;
            spot.requested = 1;
            spot.user = true;
            enterPlayerState(DetectivePlayerState::Out);
            return;
        }
        const std::size_t index = entry - spots_.size();
        RevealSpot& spot = revealSpots_[index];
        if (state.isDisabled(spot.instance) || spot.state != 0 || spot.requested != 0 || !inBox(spot.useZone, spot.position, input.player)) continue;
        enterPlayerState(DetectivePlayerState::Use);
        useSpot_ = int(index);
        return;
    }
    if (modeActive_) enterPlayerState(DetectivePlayerState::In);
}

std::vector<DetectiveAction> DetectiveSet::update(const assets::PlacedObjectState& state, const DetectiveInput& input) {
    std::vector<DetectiveAction> out;
    std::size_t last = 0;
    for (const DetectiveSpot& spot : spots_) last = std::max(last, spot.objectIndex + 1);
    for (const RevealSpot& spot : revealSpots_) last = std::max(last, spot.objectIndex + 1);
    for (const RevealProp& prop : revealProps_) last = std::max(last, prop.objectIndex + 1);
    for (std::size_t index = last; index-- > 0;) {
        for (DetectiveSpot& spot : spots_)
            if (spot.objectIndex == index && !state.isDisabled(spot.instance)) updateSpot(spot, input, out);
        for (RevealSpot& spot : revealSpots_)
            if (spot.objectIndex == index && !state.isDisabled(spot.instance)) updateRevealSpot(spot, input, out);
        for (RevealProp& prop : revealProps_)
            if (prop.objectIndex == index) updateRevealProp(prop, out);
    }
    updatePlayer(state, input);
    return out;
}

DetectiveRoles::DetectiveRoles(const assets::Table& animations) {
    if (animations.sheets.size() <= kRolesSheet) return;
    const assets::TableSheet& roles = animations.sheets[kRolesSheet];
    for (unsigned row = 0; row < roles.rows; ++row) {
        std::array<int, 7> record{};
        for (std::size_t column = 0; column < 7; ++column) record[column] = int(std::int16_t(assets::tableValue(roles, row, column + 1).bits));
        roles_.push_back(record);
    }
}

int DetectiveRoles::clip(int role) const {
    for (const std::array<int, 7>& record : roles_)
        if (record[0] == role) return record[kDetectiveRoleVariant] == kNoClip ? role : record[kDetectiveRoleVariant];
    return role;
}

std::vector<int> DetectiveRoles::clips() const {
    std::vector<int> clips;
    for (const std::array<int, 7>& record : roles_)
        if (record[kDetectiveRoleVariant] != kNoClip) clips.push_back(record[kDetectiveRoleVariant]);
    return clips;
}

}
