#include "runtime/zipline.hpp"
#include "runtime/level_start.hpp"
#include "runtime/player_intent.hpp"
#include "runtime/player_locomotion.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <map>

namespace chase::runtime {
namespace {

using Vec = std::array<float, 3>;

Vec vec(const float v[3]) { return {v[0], v[1], v[2]}; }

Vec sub(const Vec& a, const Vec& b) { return {a[0] - b[0], a[1] - b[1], a[2] - b[2]}; }

float dot(const Vec& a, const Vec& b) {
    float sum = a[0] * b[0];
    sum += a[1] * b[1];
    sum += a[2] * b[2];
    return sum;
}

float length(const Vec& v) { return std::sqrt(dot(v, v)); }

// 0x257ea0: the vector times 1 / length, unchanged when the length is below 2^-23.
Vec normalized(const Vec& v) {
    const float size = length(v);
    if (!(size > 1.1920929e-7f)) return v;
    const float inverse = 1.0f / size;
    return {v[0] * inverse, v[1] * inverse, v[2] * inverse};
}

Vec cross(const Vec& a, const Vec& b) {
    return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}

// 0x2536c0: the point in the frame of a node (rows right, up, forward and the translation).
Vec toLocal(const ZiplineAnchor& anchor, const Vec& point) {
    const Vec d = sub(point, vec(anchor.position));
    return {dot(d, vec(anchor.right)), dot(d, vec(anchor.up)), dot(d, vec(anchor.forward))};
}

// 0x254250 for the local point (0, 0, z).
Vec alongForward(const ZiplineAnchor& anchor, float z) {
    return {anchor.position[0] + z * anchor.forward[0], anchor.position[1] + z * anchor.forward[1], anchor.position[2] + z * anchor.forward[2]};
}

// 0x2397a0: centre - half <= point <= centre + half on every axis.
bool insideBox(const Vec& point, const float centre[3], const float half[3]) {
    for (int a = 0; a < 3; ++a) {
        const float d = point[a] - centre[a];
        if (d > half[a] || d < -half[a]) return false;
    }
    return true;
}

// 0x22f650 as used by the carry and detective zones: |point - centre| < half on every axis.
bool insideUseBound(const Vec& point, const float centre[3], const float half[3]) {
    for (int a = 0; a < 3; ++a)
        if (!(std::fabs(point[a] - centre[a]) < half[a])) return false;
    return true;
}

// 0x221e5c: a + (b - a) t.
Vec lerp(const Vec& a, const Vec& b, float t) {
    return {a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t, a[2] + (b[2] - a[2]) * t};
}

// 0x23df04: the facing moves towards the target by at most `step`.
std::int16_t turnTowards(std::int16_t facing, std::int16_t target, int step) {
    if (facing == target) return facing;
    int d = int(target) - int(facing);
    if (d < -32768) d += 65536;
    if (d > 32768) d -= 65536;
    d = std::clamp(d, -step, step);
    return std::int16_t(std::uint16_t(int(facing) + d));
}

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

float floatParam(const assets::LevelObject& object, const assets::Level& cls, const char* name, float fallback) {
    const assets::LevelParam* param = effectiveParam(object, cls, assets::nameHash(name));
    if (!param) return fallback;
    if (param->type != 0) return float(param->value);
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
    return int(param->value);
}

bool anchorEnabled(const std::vector<ZiplineAnchor>& anchors, const ZiplineAnchor& anchor, const std::function<bool(const ZiplineAnchor&)>& enabled) {
    if (!enabled) return true;
    return enabled(anchor) && (anchor.target < 0 || enabled(anchors[std::size_t(anchor.target)]));
}

ZiplineStep riding(const Vec& position, std::int16_t facing, int animation) {
    ZiplineStep step;
    step.riding = true;
    std::copy(position.begin(), position.end(), step.position);
    step.facing = facing;
    step.animation = animation;
    return step;
}

// 0x2c4114: t moves by rate * dt; past either end the end test (2D dot of the way to the target with the facing) can raise event 6;
// otherwise the player moves 0.2 of the way to the point at t on the line between the two anchors and turns to the line heading.
void slideStep(ZiplineRider& rider, const std::vector<ZiplineAnchor>& anchors, const ZiplineInput& input, Vec& position, std::int16_t& facing) {
    const ZiplineAnchor& line = anchors[std::size_t(rider.line)];
    const ZiplineAnchor& target = anchors[std::size_t(line.target)];
    rider.t += rider.rate * input.dt;
    bool move = true;
    if (rider.t > 1.0f || rider.t < 0.0f) {
        const Vec toTarget = sub(vec(target.position), position);
        const float heading = headingRadians(facing);
        const float forwardX = std::sin(heading), forwardZ = std::cos(heading);
        float along = toTarget[0] * forwardX;
        along += toTarget[2] * forwardZ;
        if (along < 0.0f) {
            rider.endReached = true;
            move = false;
        }
    }
    if (move) {
        const Vec point = lerp(vec(line.position), vec(target.position), rider.t);
        const float before = position[1];
        position = lerp(position, point, kZiplineFollowFactor);
        rider.fallSpeed = before - position[1];
    }
    facing = turnTowards(facing, rider.heading, kZiplineTurnStep);
}

void startDismount(ZiplineRider& rider, const Vec& position, ZiplineStep& step) {
    if (rider.phase == ZiplinePhase::GetOn || rider.phase == ZiplinePhase::Slide) step.sounds.push_back({ZiplineSoundAction::Stop, kZiplineLoopSound});
    rider.phase = ZiplinePhase::Dismount;
    rider.phaseUpdates = 0;
    rider.endReached = false;
    std::copy(position.begin(), position.end(), rider.base);
    rider.fallSpeed = 1.0f;
    rider.fallDistance = 0.0f;
}

Vec rootAt(const assets::RootMotion& motion, float frame) {
    if (motion.translation.empty()) return {0, 0, 0};
    const float last = float(motion.translation.size() - 1);
    frame = std::clamp(frame, 0.0f, last);
    const std::size_t index = std::size_t(frame);
    if (index + 1 >= motion.translation.size()) return motion.translation.back();
    const float f = frame - float(index);
    const auto& a = motion.translation[index];
    const auto& b = motion.translation[index + 1];
    return {a[0] + (b[0] - a[0]) * f, a[1] + (b[1] - a[1]) * f, a[2] + (b[2] - a[2]) * f};
}

}

void linkZiplineAnchors(std::vector<ZiplineAnchor>& anchors) {
    for (ZiplineAnchor& anchor : anchors) {
        anchor.endOffset = anchor.classWalkCentre[2] - anchor.classWalkHalf[2];
        std::copy_n(anchor.classWalkCentre, 3, anchor.walkCentre);
        std::copy_n(anchor.classWalkHalf, 3, anchor.walkHalf);
        anchor.listed = false;
        if (anchor.target < 0) continue;
        const ZiplineAnchor& target = anchors[std::size_t(anchor.target)];
        const Vec own = vec(anchor.position), other = vec(target.position);
        float dx = own[0] - other[0], dz = own[2] - other[2];
        float squared = dx * dx;
        squared += dz * dz;
        const float horizontal = std::sqrt(squared);
        anchor.walkCentre[2] = horizontal * 0.5f;
        anchor.walkHalf[2] = std::fabs(anchor.endOffset) + horizontal * 0.5f;
        const Vec up = {0, 1, 0};
        const Vec forward = normalized(sub(other, own));
        const Vec right = normalized(cross(up, forward));
        const Vec level = cross(right, up);
        std::copy(right.begin(), right.end(), anchor.right);
        std::copy(up.begin(), up.end(), anchor.up);
        std::copy(level.begin(), level.end(), anchor.forward);
    }
    for (ZiplineAnchor& anchor : anchors)
        anchor.listed = anchor.target >= 0 && anchors[std::size_t(anchor.target)].order <= anchor.order;
}

std::vector<ZiplineAnchor> loadZiplineAnchors(const assets::Level& level, const assets::ClassLookup& classOf) {
    std::vector<ZiplineAnchor> anchors;
    std::vector<std::uint32_t> targets;
    std::map<std::uint32_t, std::size_t> byHash;
    for (std::size_t i = 0; i < level.objects.size(); ++i) {
        const assets::LevelObject& object = level.objects[i];
        const assets::Level* cls = classOf(object.className);
        if (!cls) continue;
        const std::uint32_t typeHash = assets::nameHash("ObjectType");
        int type = -1;
        for (const assets::LevelParam& param : cls->classParams)
            if (param.hash == typeHash) type = int(param.value);
        if (type != kGrappleLineObjectType || intParam(object, *cls, "GrappleLineType", 0) != kGrappleLineZipline) continue;
        const std::optional<assets::Placement> placed = assets::objectStart(level, object.instance);
        if (!placed) continue;
        ZiplineAnchor anchor;
        anchor.instance = lower(object.instance);
        anchor.order = i;
        std::copy_n(placed->position, 3, anchor.position);
        anchor.speed = floatParam(object, *cls, "ZipLineSpeed", kZipLineSpeedDefault);
        anchor.lineType = kGrappleLineZipline;
        anchor.faceTowards = intParam(object, *cls, "FaceTowards", 0) & 7;
        for (const assets::LevelBound& bound : cls->bounds) {
            const std::string name = lower(bound.name);
            if (name == "walkbound") {
                std::copy_n(bound.centre, 3, anchor.classWalkCentre);
                std::copy_n(bound.half, 3, anchor.classWalkHalf);
            } else if (name == "usebound") {
                anchor.hasUseBound = true;
                std::copy_n(bound.centre, 3, anchor.useCentre);
                std::copy_n(bound.half, 3, anchor.useHalf);
            }
        }
        const assets::LevelParam* target = effectiveParam(object, *cls, assets::nameHash("TargetTightropeAnchor"));
        targets.push_back(target ? assets::objectReferenceHash(*target) : 0);
        byHash[assets::nameHash(object.instance)] = anchors.size();
        anchors.push_back(anchor);
    }
    for (std::size_t i = 0; i < anchors.size(); ++i) {
        const auto found = byHash.find(targets[i]);
        if (targets[i] != 0 && found != byHash.end() && found->second != i) anchors[i].target = int(found->second);
    }
    linkZiplineAnchors(anchors);
    return anchors;
}

int ziplineGrabTest(const std::vector<ZiplineAnchor>& anchors, const float point[3], const std::function<bool(const ZiplineAnchor&)>& enabled) {
    const Vec p = vec(point);
    for (std::size_t i = 0; i < anchors.size(); ++i) {
        const ZiplineAnchor& anchor = anchors[i];
        if (!anchor.listed || !anchorEnabled(anchors, anchor, enabled)) continue;
        Vec local = toLocal(anchor, p);
        const Vec target = toLocal(anchor, vec(anchors[std::size_t(anchor.target)].position));
        const float along = local[2] / target[2];
        local[1] = local[1] - target[1] * along;
        if (insideBox(local, anchor.walkCentre, anchor.walkHalf)) return int(i);
    }
    return -1;
}

int ziplineUseTest(const std::vector<ZiplineAnchor>& anchors, const float player[3], std::int16_t facing,
                   const std::function<bool(const ZiplineAnchor&)>& enabled) {
    const Vec p = vec(player);
    const float heading = headingRadians(facing);
    const Vec forward = {std::sin(heading), 0.0f, std::cos(heading)};
    for (std::size_t i = 0; i < anchors.size(); ++i) {
        const ZiplineAnchor& anchor = anchors[i];
        if (!anchor.hasUseBound || anchor.target < 0 || !anchorEnabled(anchors, anchor, enabled)) continue;
        if (anchor.faceTowards == 1 && dot(forward, normalized(sub(vec(anchor.position), p))) < 0.0f) continue;
        if (!insideUseBound(toLocal(anchor, p), anchor.useCentre, anchor.useHalf)) continue;
        if (anchors[std::size_t(anchor.target)].position[1] >= anchor.position[1]) continue;
        return int(i);
    }
    return -1;
}

ZiplineStep ziplineEnter(ZiplineRider& rider, const std::vector<ZiplineAnchor>& anchors, int listed, const ZiplineInput& input) {
    int line = listed;
    const int other = anchors[std::size_t(listed)].target;
    if (anchors[std::size_t(listed)].position[1] < anchors[std::size_t(other)].position[1]) line = other;
    const ZiplineAnchor& high = anchors[std::size_t(line)];
    const ZiplineAnchor& low = anchors[std::size_t(high.target)];
    const Vec start = alongForward(high, high.endOffset);
    const Vec end = alongForward(low, high.endOffset);
    const Vec way = sub(end, start);
    const float headingAngle = gameAtan2(way[0], way[2]) * kZiplineHeadingScale;
    const float span = length(way);
    const float perUpdate = input.dt * high.speed;
    const float updates = span / perUpdate;
    Vec position = vec(input.position);
    rider = ZiplineRider();
    rider.phase = ZiplinePhase::GetOn;
    rider.line = line;
    rider.heading = std::int16_t(std::uint16_t(std::int32_t(headingAngle)));
    rider.rate = float(kZiplineUpdateRate) / updates;
    rider.t = length(sub(position, start)) / span;
    std::int16_t facing = input.facing;
    slideStep(rider, anchors, input, position, facing);
    ZiplineStep step = riding(position, facing, kAnimationZiplineGrab);
    step.sounds.insert(step.sounds.begin(), ZiplineSound{ZiplineSoundAction::Play, kZiplineLoopSound});
    rider.phaseUpdates = 1;
    return step;
}

ZiplineStep ziplineUpdate(ZiplineRider& rider, const std::vector<ZiplineAnchor>& anchors, const ZiplineInput& input) {
    Vec position = vec(input.position);
    std::int16_t facing = input.facing;
    ZiplineStep step;
    if (rider.phase == ZiplinePhase::None) return step;
    if ((rider.phase == ZiplinePhase::GetOn || rider.phase == ZiplinePhase::Slide) && (rider.endReached || input.jumpPressed))
        startDismount(rider, position, step);
    if (rider.phase == ZiplinePhase::GetOn && rider.phaseUpdates >= input.grabUpdates) {
        rider.phase = ZiplinePhase::Slide;
        rider.phaseUpdates = 0;
    }
    if (rider.phase == ZiplinePhase::GetOn || rider.phase == ZiplinePhase::Slide) {
        if (input.enabled && !anchorEnabled(anchors, anchors[std::size_t(rider.line)], input.enabled)) {
            step.sounds.push_back({ZiplineSoundAction::Stop, kZiplineLoopSound});
            step.falling = true;
            step.fallSpeed = rider.fallSpeed;
            step.fallDistance = rider.fallDistance;
            std::copy(position.begin(), position.end(), step.position);
            step.facing = facing;
            rider = ZiplineRider();
            return step;
        }
        slideStep(rider, anchors, input, position, facing);
        ZiplineStep moved = riding(position, facing, rider.phase == ZiplinePhase::GetOn ? kAnimationZiplineGrab : kAnimationZiplineSlide);
        moved.sounds = step.sounds;
        ++rider.phaseUpdates;
        return moved;
    }
    if (rider.phaseUpdates >= input.dismountUpdates) {
        step.falling = true;
        step.fallSpeed = rider.fallSpeed;
        step.fallDistance = rider.fallDistance;
        std::copy(position.begin(), position.end(), step.position);
        step.facing = facing;
        rider = ZiplineRider();
        return step;
    }
    if (input.dismountMotion) {
        const float frame = float(rider.phaseUpdates) * input.dt * input.dismountFramesPerSecond;
        const Vec root = rootAt(*input.dismountMotion, frame);
        const float heading = headingRadians(facing), s = std::sin(heading), c = std::cos(heading);
        const float before = position[1];
        position[0] = rider.base[0] + s * root[2] + c * root[0];
        position[1] = rider.base[1] + root[1];
        position[2] = rider.base[2] + c * root[2] - s * root[0];
        const float drop = before - position[1];
        rider.fallSpeed = drop;
        rider.fallDistance = drop;
        if (drop > 0.0f && !input.dismountMotion->translation.empty()) {
            const std::size_t last = input.dismountMotion->translation.size() - 1;
            const std::size_t key = std::min(last - 1, std::size_t(std::max(0.0f, std::ceil(frame) - 1.0f)));
            rider.fallDistance = input.dismountMotion->translation[key][1] - input.dismountMotion->translation[key + 1][1];
        }
    }
    ZiplineStep moved = riding(position, facing, kAnimationZiplineDismount);
    moved.sounds = step.sounds;
    ++rider.phaseUpdates;
    return moved;
}

bool ziplineJumpTick(ZiplineRider& rider, bool jumpStarted, bool jumping) {
    if (jumpStarted) {
        rider.jumpCount = 0;
        return false;
    }
    if (!jumping || rider.jumpCount < 0) {
        rider.jumpCount = -1;
        return false;
    }
    const bool test = rider.jumpCount > kZiplineGrabJumpCount;
    ++rider.jumpCount;
    return test;
}

}
