#include "runtime/climb_bar.hpp"
#include "runtime/level_start.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <map>

namespace chase::runtime {
namespace {

constexpr float kPi = 3.14159265358979f;
constexpr float kHeadingScale = 10430.378f;  // 0x275138

using Vec = std::array<float, 3>;

Vec vec(const float v[3]) { return {v[0], v[1], v[2]}; }
Vec sub(const Vec& a, const Vec& b) { return {a[0] - b[0], a[1] - b[1], a[2] - b[2]}; }
Vec add(const Vec& a, const Vec& b) { return {a[0] + b[0], a[1] + b[1], a[2] + b[2]}; }
Vec scaled(const Vec& a, float s) { return {a[0] * s, a[1] * s, a[2] * s}; }

float dot(const Vec& a, const Vec& b) {
    float sum = a[0] * b[0];
    sum += a[1] * b[1];
    sum += a[2] * b[2];
    return sum;
}

float length(const Vec& v) { return std::sqrt(dot(v, v)); }

Vec normalized(const Vec& v) {
    const float size = length(v);
    if (!(size > 1.1920929e-7f)) return v;
    const float inverse = 1.0f / size;
    return {v[0] * inverse, v[1] * inverse, v[2] * inverse};
}

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
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

// The node rows of an object turned about y by the placement yaw: x = (cos, 0, -sin), z = (sin, 0, cos).
struct Frame {
    Vec right, up, forward, origin;
    Vec world(const Vec& local) const {
        Vec out = origin;
        out = add(out, scaled(right, local[0]));
        out = add(out, scaled(up, local[1]));
        out = add(out, scaled(forward, local[2]));
        return out;
    }
    Vec toLocal(const Vec& point) const {
        const Vec d = sub(point, origin);
        return {dot(d, right), dot(d, up), dot(d, forward)};
    }
};

Frame frameOf(const float position[3], float yawDegrees) {
    const float yaw = yawDegrees * (kPi / 180.0f);
    const float s = std::sin(yaw), c = std::cos(yaw);
    return {{c, 0, -s}, {0, 1, 0}, {s, 0, c}, vec(position)};
}

std::int16_t facingFromRadians(float heading) {
    return std::int16_t(std::uint16_t(int(std::lround(double(heading) * kHeadingScale))));
}

float radiansFromFacing(std::int16_t facing) { return float(std::uint16_t(facing)) / kHeadingScale; }

Vec facingRow(std::int16_t facing) {
    const float heading = radiansFromFacing(facing);
    return {std::sin(heading), 0.0f, std::cos(heading)};
}

// The facing of the hanging player: 1fc2fc turns the node by pi against the bar's z row; an acrobat pole uses 0x1f80d4.
std::int16_t hangFacing(const ClimbBar& bar, std::int16_t playerFacing) {
    if (bar.acrobatPole) return acrobatPoleFacing(bar, playerFacing);
    return facingFromRadians(bar.yawDegrees * (kPi / 180.0f) + kPi);
}

// 0x1fb8d8: the two ends of the hang line, +x end first (A) and -x end (B), at the hanging height.
void hangLine(const ClimbBar& bar, float offsetY, float offsetZ, Vec& a, Vec& b) {
    const Frame frame = frameOf(bar.position, bar.yawDegrees);
    const Vec centre = frame.world(vec(bar.grabCentre));
    const Vec along = scaled(frame.right, bar.grabHalf[0]);
    Vec offset = scaled(frame.up, offsetY);
    offset = add(offset, scaled(frame.forward, offsetZ));
    a = add(add(centre, along), offset);
    b = add(sub(centre, along), offset);
}

void hangLine(const ClimbBar& bar, Vec& a, Vec& b) {
    if (bar.acrobatPole) hangLine(bar, kAcrobatPoleHangOffsetY, kAcrobatPoleHangOffsetZ, a, b);
    else hangLine(bar, kClimbBarHangOffsetY, kClimbBarHangOffsetZ, a, b);
}

// 0x2bbdc0: the hang point nearest to the player on the line, moved along the bar by `step` and held between the ends.
Vec hangPoint(const ClimbBar& bar, const Vec& position, float step) {
    Vec a, b;
    hangLine(bar, a, b);
    const Vec ab = sub(b, a);
    const Vec direction = normalized(ab);
    const float along = dot(sub(position, a), direction);
    Vec point = add(a, scaled(direction, along));
    const Frame frame = frameOf(bar.position, bar.yawDegrees);
    point = add(point, scaled(frame.right, step));
    const Vec fromA = sub(point, a);
    if (dot(fromA, ab) > 0.0f) {
        if (length(ab) <= length(fromA)) point = b;
    } else {
        point = a;
    }
    return point;
}

ClimbBarStep riding(const Vec& position, std::int16_t facing, int animation) {
    ClimbBarStep step;
    step.riding = true;
    std::copy(position.begin(), position.end(), step.position);
    step.facing = facing;
    step.animation = animation;
    return step;
}

ClimbBarStep release(ClimbBarRider& rider, ClimbBarRelease how, const Vec& position, std::int16_t facing) {
    ClimbBarStep step = riding(position, facing, 0);
    step.riding = false;
    step.release = how;
    climbBarLeave(rider);
    rider.phase = ClimbPhase::None;
    return step;
}

int animationOf(ClimbPhase phase) {
    switch (phase) {
        case ClimbPhase::Left: case ClimbPhase::CornerLeft: return kAnimationLedgeLeft;
        case ClimbPhase::Right: case ClimbPhase::CornerRight: return kAnimationLedgeRight;
        default: return kAnimationLedgeIdle;
    }
}

// 0x2a1c54: the stick against the bar axis: -1 towards +x of the bar (left), +1 towards -x (right), 0 inside +-0.3 or without the stick.
float stickDirection(const ClimbBar& bar, const ClimbBarInput& input) {
    if (!input.stickHeld) return 0.0f;
    const float heading = radiansFromFacing(input.stickHeading);
    const Frame frame = frameOf(bar.position, bar.yawDegrees);
    float d = std::sin(heading) * frame.right[0];
    d += std::cos(heading) * frame.right[2];
    if (d < -kClimbBarDirectionLimit) return 1.0f;
    if (d > kClimbBarDirectionLimit) return -1.0f;
    return 0.0f;
}

float rootX(const assets::RootMotion& motion, float frame) {
    const float last = float(motion.translation.size() - 1);
    frame = std::max(0.0f, std::min(frame, last));
    const std::size_t index = std::size_t(frame);
    if (index + 1 >= motion.translation.size()) return motion.translation.back()[0];
    const float f = frame - float(index);
    return motion.translation[index][0] + (motion.translation[index + 1][0] - motion.translation[index][0]) * f;
}

// 0x2327b4: the root motion x of the update, from the clip time before to the clip time after (a loop passes the end and the start).
// TRAV-S18: the wrap of the loop is taken as continuous; the cached per-update delta of the animation object is not read.
float rootStep(const assets::RootMotion& motion, float before, float after) {
    const float length = float(motion.translation.size() - 1);
    if (after < length) return rootX(motion, after) - rootX(motion, before);
    float delta = rootX(motion, length) - rootX(motion, before);
    delta += rootX(motion, after - length) - rootX(motion, 0.0f);
    return delta;
}

void setPhase(ClimbBarRider& rider, ClimbPhase phase) {
    if (rider.phase != phase) rider.clipFrame = 0.0f;
    rider.phase = phase;
}

// 0x1fc2fc: place the player on the line, moving along it. Returns whether the move was blocked (a direction was asked and the
// horizontal move is nil). TRAV-S18: the move 0x215804 is applied without the world collision.
bool placeOnBar(const ClimbBar& bar, ClimbBarRider& rider, float direction, const ClimbBarInput& input, Vec& position) {
    float step = direction * -kClimbBarFallbackStep;
    const assets::RootMotion* motion = rider.phase == ClimbPhase::Left ? input.leftMotion : rider.phase == ClimbPhase::Right ? input.rightMotion : nullptr;
    if (motion && motion->translation.size() > 1) {
        const float before = rider.clipFrame;
        rider.clipFrame += input.dt * input.clipFramesPerSecond;
        step = -rootStep(*motion, before, rider.clipFrame);
        const float length = float(motion->translation.size() - 1);
        if (rider.clipFrame >= length) rider.clipFrame -= length;
    }
    const Vec point = hangPoint(bar, position, step);
    const Vec delta = sub(point, position);
    const bool blocked = direction != 0.0f && std::fabs(delta[0]) < 1e-5f && std::fabs(delta[2]) < 1e-5f;
    position = point;
    return blocked;
}

ClimbBarStep arriveUpdate(ClimbBarRider& rider, const ClimbBarSet& set, const ClimbBarInput& input) {
    const ClimbBar& bar = set.bars[std::size_t(rider.bar)];
    Vec position = vec(input.position);
    if (input.bHeld && !rider.jumpLatch) {
        rider.jumpLatch = true;
        return release(rider, ClimbBarRelease::Jump, position, input.facing);
    }
    if (input.aPressed) return release(rider, ClimbBarRelease::Fall, position, input.facing);
    const Vec point = hangPoint(bar, position, 0.0f);
    const Vec delta = sub(point, position);
    const float step = input.dt * kClimbBarArrivalSpeed;
    if (step <= length(delta)) {
        position = add(position, scaled(normalized(delta), step));
    } else {
        position = point;
        setPhase(rider, ClimbPhase::Idle);
        rider.enteredThisUpdate = true;
    }
    return riding(position, input.facing, kAnimationLedgeIdle);
}

void startCorner(ClimbBarRider& rider, const ClimbBarSet& set, int corner, bool left, const Vec& position) {
    const CornerRail& rail = set.corners[std::size_t(corner)];
    rider.phase = left ? ClimbPhase::CornerLeft : ClimbPhase::CornerRight;
    rider.corner = corner;
    const Frame frame = frameOf(rail.position, rail.yawDegrees);
    const Vec local = frame.toLocal(position);
    float best = 3.4028235e+38f, found = 0.0f;
    const int count = int(rail.path.size() - 1) * kCornerRailSamplesPerSegment;
    for (int k = 1; k <= count; ++k) {
        const float t = float(k) / float(kCornerRailSamplesPerSegment);
        const std::array<float, 3> p = splinePoint(rail.path, t);
        const Vec d = sub({p[0], p[1], p[2]}, local);
        const float distance = length(d);
        if (best > distance) {
            found = t;
            best = distance;
        }
    }
    rider.t = found;
}

// 0x18fe40 for every corner: the hanging player moving towards a corner whose use box holds him enters the corner state.
bool cornerCheck(ClimbBarRider& rider, const ClimbBarSet& set, const ClimbBarInput& input, ClimbPhase phase) {
    const Vec position = vec(input.position);
    const float heading = radiansFromFacing(input.facing);
    const Vec right = {std::cos(heading), 0.0f, -std::sin(heading)};
    for (std::size_t i = 0; i < set.corners.size(); ++i) {
        const CornerRail& rail = set.corners[i];
        const Vec towards = normalized(sub(vec(rail.position), position));
        const float side = dot(towards, right);
        if (!((side > 0.0f && phase == ClimbPhase::Right) || (side < 0.0f && phase == ClimbPhase::Left))) continue;
        const Frame frame = frameOf(rail.position, rail.yawDegrees);
        const Vec local = frame.toLocal(position);
        bool inside = true;
        for (int a = 0; a < 3; ++a)
            if (!(std::fabs(local[a] - rail.useCentre[a]) < rail.useHalf[a])) inside = false;
        if (!inside) continue;
        startCorner(rider, set, int(i), phase == ClimbPhase::Left, position);
        return true;
    }
    return false;
}

ClimbBarStep cornerUpdate(ClimbBarRider& rider, const ClimbBarSet& set, const ClimbBarInput& input) {
    const CornerRail& rail = set.corners[std::size_t(rider.corner)];
    const Vec position = vec(input.position);
    const bool left = rider.phase == ClimbPhase::CornerLeft;
    if (rider.t < kCornerRailLength && 0.0f <= rider.t) {
        const float old = rider.t;
        const float move = input.dt * kCornerRailRate;
        rider.t = left ? old + move : old - move;
        const Frame frame = frameOf(rail.position, rail.yawDegrees);
        const std::array<float, 3> p = splinePoint(rail.path, old);
        const float turn = old * 0.33333334f;
        const float angle = turn * (kPi * 0.5f);
        const float offsetX = kClimbBarHangOffsetZ * std::sin(angle);
        const float offsetZ = kClimbBarHangOffsetZ * std::cos(angle);
        const Vec local = {p[0] + offsetX, p[1] + kClimbBarHangOffsetY, p[2] + offsetZ};
        const float yaw = rail.yawDegrees * (kPi / 180.0f);
        return riding(frame.world(local), facingFromRadians(yaw + kPi + turn * (kPi * 0.5f)), animationOf(rider.phase));
    }
    const int bar = climbBarGrabTest(set, rider, input.position, input.facing, input.enabled);
    if (bar < 0) return release(rider, ClimbBarRelease::Fall, position, input.facing);
    ClimbBarStep entered = climbBarGrab(rider, set, bar, input);
    ClimbBarInput next = input;
    std::copy(entered.position, entered.position + 3, next.position);
    next.facing = entered.facing;
    ClimbBarStep arrived = arriveUpdate(rider, set, next);
    arrived.sounds = entered.sounds;
    return arrived;
}

}

namespace {
// TRAV-S18: only the yaw is used. Pitch and roll that are both 180 degrees (the 1-2_FlagPole placements, [180, 0, 180]) turn the node about y by
// 180 degrees (Rx(pi) Rz(pi) = Ry(pi)), measured: the grab box of 1-2_FlagPole_01 stands at x 721.362 = 723.112 - 1.75.
float placementYaw(const float orientation[3]) {
    const bool flipped = std::fabs(std::fabs(orientation[0]) - 180.0f) < 0.01f && std::fabs(std::fabs(orientation[2]) - 180.0f) < 0.01f;
    return orientation[1] + (flipped ? 180.0f : 0.0f);
}
}

namespace {
int objectTypeOf(const assets::Level* cls) {
    if (!cls) return -1;
    for (const assets::LevelParam& param : cls->classParams)
        if (param.hash == assets::nameHash("ObjectType")) return int(param.value);
    return -1;
}
}

ClimbBarLink linkParam(const assets::Level& level, const assets::LevelObject& object, const assets::Level& cls, const char* name,
                       const std::map<std::uint32_t, std::string>& names, const assets::ClassLookup& classOf) {
    ClimbBarLink link;
    const assets::LevelParam* param = effectiveParam(object, cls, assets::nameHash(name));
    if (!param) return link;
    const auto found = names.find(assets::objectReferenceHash(*param));
    if (found == names.end()) return link;
    const std::optional<assets::Placement> placed = assets::objectStart(level, found->second);
    if (!placed) return link;
    link.present = true;
    link.instance = lower(found->second);
    std::copy_n(placed->position, 3, link.position);
    for (const assets::LevelObject& target : level.objects)
        if (target.instance == found->second) link.objectType = objectTypeOf(classOf(target.className));
    return link;
}

ClimbBarSet loadClimbBars(const assets::Level& level, const assets::ClassLookup& classOf) {
    ClimbBarSet set;
    const std::uint32_t typeHash = assets::nameHash("ObjectType");
    std::map<std::uint32_t, std::string> names;
    for (const assets::LevelObject& object : level.objects) names[assets::nameHash(object.instance)] = object.instance;
    const char* const linkNames[4] = {"PrimaryNextBar", "PrimaryPrevBar", "SecondaryNextBar", "SecondaryPrevBar"};
    for (std::size_t i = 0; i < level.objects.size(); ++i) {
        const assets::LevelObject& object = level.objects[i];
        const assets::Level* cls = classOf(object.className);
        if (!cls) continue;
        int type = -1;
        for (const assets::LevelParam& param : cls->classParams)
            if (param.hash == typeHash) type = int(param.value);
        if (type != kClimbBarObjectType && type != kCornerRailObjectType) continue;
        const std::optional<assets::Placement> placed = assets::objectStart(level, object.instance);
        if (!placed) continue;
        if (type == kClimbBarObjectType) {
            ClimbBar bar;
            bar.instance = lower(object.instance);
            bar.order = i;
            std::copy_n(placed->position, 3, bar.position);
            bar.yawDegrees = placementYaw(placed->orientation);
            for (const assets::LevelBound& bound : cls->bounds) {
                if (lower(bound.name) != "grabbound") continue;
                bar.hasGrabBound = true;
                std::copy_n(bound.centre, 3, bar.grabCentre);
                std::copy_n(bound.half, 3, bar.grabHalf);
            }
            bar.acrobatPole = intParam(object, *cls, "AcrobatPole", 0) != 0;
            bar.narrowBar = intParam(object, *cls, "NarrowBar", 0) != 0;
            bar.faceJumpDir = intParam(object, *cls, "FaceJumpDir", 1) != 0;
            bar.immediateJump = intParam(object, *cls, "ImmediateJump", 0) != 0;
            bar.ledge = intParam(object, *cls, "Ledge", 0) != 0;
            bar.grabSound = std::uint16_t(intParam(object, *cls, "ATTR_SFX_GRAB", 0));
            bar.swingSound = std::uint16_t(intParam(object, *cls, "ATTR_SFX_SWING", 0));
            for (std::size_t k = 0; k < bar.links.size(); ++k) bar.links[k] = linkParam(level, object, *cls, linkNames[k], names, classOf);
            if (bar.hasGrabBound) set.bars.push_back(bar);
        } else {
            CornerRail rail;
            rail.instance = lower(object.instance);
            rail.order = i;
            std::copy_n(placed->position, 3, rail.position);
            rail.yawDegrees = placed->orientation[1];
            bool hasUse = false;
            for (const assets::LevelBound& bound : cls->bounds) {
                if (lower(bound.name) != "usebound") continue;
                hasUse = true;
                std::copy_n(bound.centre, 3, rail.useCentre);
                std::copy_n(bound.half, 3, rail.useHalf);
            }
            if (!cls->paths.empty()) rail.path = cls->paths.front().points;
            if (hasUse && rail.path.size() >= 2) set.corners.push_back(rail);
        }
    }
    for (ClimbBar& bar : set.bars)
        for (ClimbBarLink& link : bar.links)
            for (std::size_t i = 0; link.present && i < set.bars.size(); ++i)
                if (set.bars[i].instance == link.instance) link.bar = int(i);
    return set;
}

bool obbOverlap(const float a[3], const float t[3], const float b[3], const float r[3][3]) {
    for (int i = 0; i < 3; ++i) {
        float reach = a[i];
        for (int k = 0; k < 3; ++k) reach += b[k] * std::fabs(r[k][i]);
        if (reach < std::fabs(t[i])) return false;
    }
    for (int k = 0; k < 3; ++k) {
        float reach = b[k];
        for (int i = 0; i < 3; ++i) reach += a[i] * std::fabs(r[k][i]);
        float projected = 0.0f;
        for (int i = 0; i < 3; ++i) projected += t[i] * r[k][i];
        if (reach < std::fabs(projected)) return false;
    }
    for (int i = 0; i < 3; ++i) {
        const int i1 = (i + 1) % 3, i2 = (i + 2) % 3;
        for (int k = 0; k < 3; ++k) {
            const int k1 = (k + 1) % 3, k2 = (k + 2) % 3;
            const float ra = a[i1] * std::fabs(r[k][i2]) + a[i2] * std::fabs(r[k][i1]);
            const float rb = b[k1] * std::fabs(r[k2][i]) + b[k2] * std::fabs(r[k1][i]);
            const float distance = t[i2] * r[k][i1] - t[i1] * r[k][i2];
            if (ra + rb < std::fabs(distance)) return false;
        }
    }
    return true;
}

std::array<float, 3> splinePoint(const std::vector<std::array<float, 3>>& points, float t) {
    const int n = int(points.size());
    t = std::max(0.0f, std::min(t, float(n - 1)));
    const int i = int(std::floor(t));
    const float u = t - float(i);
    const int a = std::max(i - 1, 0), b = i, c = std::min(i + 1, n - 1), d = std::min(i + 2, n - 1);
    const float u2 = u * u, u3 = u2 * u;
    std::array<float, 3> out{};
    for (int k = 0; k < 3; ++k) {
        const float pb = points[std::size_t(b)][std::size_t(k)], pc = points[std::size_t(c)][std::size_t(k)];
        const float pa = points[std::size_t(a)][std::size_t(k)], pd = points[std::size_t(d)][std::size_t(k)];
        out[std::size_t(k)] = pb * ((u3 * 2.0f - u2 * 3.0f) + 1.0f) + (u3 * -2.0f + u2 * 3.0f) * pc + (pc - pa) * ((u3 - u2 * 2.0f) + u) * 0.5f +
                              (pd - pb) * (u3 - u2) * 0.5f;
    }
    return out;
}

int climbBarGrabTest(const ClimbBarSet& set, ClimbBarRider& rider, const float position[3], std::int16_t facing,
                     const std::function<bool(const ClimbBar&)>& enabled) {
    const float heading = radiansFromFacing(facing);
    const Vec playerRight = {std::cos(heading), 0, -std::sin(heading)};
    const Vec playerUp = {0, 1, 0};
    const Vec playerForward = {std::sin(heading), 0, std::cos(heading)};
    Vec centre = vec(position);
    centre = add(centre, scaled(playerUp, kClimbBarPlayerBoxCentre[1]));
    centre = add(centre, scaled(playerForward, kClimbBarPlayerBoxCentre[2]));
    centre = add(centre, scaled(playerRight, kClimbBarPlayerBoxCentre[0]));
    for (std::size_t i = 0; i < set.bars.size(); ++i) {
        const ClimbBar& bar = set.bars[i];
        if (enabled && !enabled(bar)) continue;
        const Frame frame = frameOf(bar.position, bar.yawDegrees);
        if (!bar.acrobatPole && dot(frame.forward, playerForward) > kClimbBarFacingLimit) continue;
        const Vec boxCentre = frame.world(vec(bar.grabCentre));
        const Vec d = sub(boxCentre, centre);
        const float t[3] = {dot(d, playerRight), dot(d, playerUp), dot(d, playerForward)};
        const Vec barAxes[3] = {frame.right, frame.up, frame.forward};
        const Vec playerAxes[3] = {playerRight, playerUp, playerForward};
        float r[3][3];
        for (int k = 0; k < 3; ++k)
            for (int j = 0; j < 3; ++j) r[k][j] = dot(barAxes[k], playerAxes[j]);
        const bool hit = obbOverlap(kClimbBarPlayerBoxHalf, t, bar.grabHalf, r);
        if (!hit) {
            if (rider.ignoreBar == int(i)) rider.ignoreBar = -1;
            continue;
        }
        if (rider.ignoreBar == int(i)) continue;
        return int(i);
    }
    return -1;
}

ClimbBarStep climbBarGrab(ClimbBarRider& rider, const ClimbBarSet& set, int bar, const ClimbBarInput& input) {
    rider.phase = ClimbPhase::Arrive;
    rider.bar = bar;
    rider.corner = -1;
    rider.enteredThisUpdate = false;
    ClimbBarStep step = riding(vec(input.position), input.facing, kAnimationLedgeIdle);
    const std::uint16_t sound = set.bars[std::size_t(bar)].grabSound;
    if (sound) step.sounds.push_back({sound});
    return step;
}

ClimbBarStep climbBarUpdate(ClimbBarRider& rider, const ClimbBarSet& set, const ClimbBarInput& input) {
    if (rider.phase == ClimbPhase::Arrive) return arriveUpdate(rider, set, input);
    if (rider.phase == ClimbPhase::CornerLeft || rider.phase == ClimbPhase::CornerRight) return cornerUpdate(rider, set, input);
    const ClimbBar& bar = set.bars[std::size_t(rider.bar)];
    Vec position = vec(input.position);
    if (rider.enteredThisUpdate) {
        rider.enteredThisUpdate = false;
        if (bar.acrobatPole) position = acrobatPoleCentre(bar, kAcrobatPoleHangOffsetY, kAcrobatPoleHangOffsetZ);
        else placeOnBar(bar, rider, 0.0f, input, position);
        return riding(position, hangFacing(bar, input.facing), kAnimationLedgeIdle);
    }
    if ((rider.phase == ClimbPhase::Left || rider.phase == ClimbPhase::Right) && cornerCheck(rider, set, input, rider.phase))
        return riding(position, input.facing, animationOf(rider.phase));
    if (!input.bHeld) rider.jumpLatch = false;
    if (input.bHeld && !rider.jumpLatch) {
        rider.jumpLatch = true;
        return release(rider, ClimbBarRelease::Jump, position, input.facing);
    }
    if (input.aPressed) return release(rider, ClimbBarRelease::Fall, position, input.facing);
    const float direction = stickDirection(bar, input);
    const bool blocked = placeOnBar(bar, rider, direction, input, position);
    if (rider.phase == ClimbPhase::Idle) {
        if (!blocked) {
            if (direction > 0.0f) setPhase(rider, ClimbPhase::Right);
            else if (direction < 0.0f) setPhase(rider, ClimbPhase::Left);
        }
    } else if (blocked || direction == 0.0f) {
        setPhase(rider, ClimbPhase::Idle);
    } else if (rider.phase == ClimbPhase::Left && direction > 0.0f) {
        setPhase(rider, ClimbPhase::Right);
    } else if (rider.phase == ClimbPhase::Right && direction < 0.0f) {
        setPhase(rider, ClimbPhase::Left);
    }
    return riding(position, hangFacing(bar, input.facing), animationOf(rider.phase));
}

std::array<float, 3> acrobatPoleCentre(const ClimbBar& pole, float offsetY, float offsetZ) {
    Vec a, b;
    hangLine(pole, offsetY, offsetZ, a, b);
    return scaled(add(a, b), 0.5f);
}

std::array<float, 3> climbBarGrabCentre(const ClimbBar& bar) {
    return frameOf(bar.position, bar.yawDegrees).world(vec(bar.grabCentre));
}

std::int16_t acrobatPoleFacing(const ClimbBar& pole, std::int16_t playerFacing) {
    const Frame frame = frameOf(pole.position, pole.yawDegrees);
    const float turn = dot(facingRow(playerFacing), frame.forward) > 0.0f ? 0.0f : kPi;
    return facingFromRadians(pole.yawDegrees * (kPi / 180.0f) + turn);
}

AcrobatPoleJump acrobatPoleJump(const ClimbBar& pole, std::int16_t playerFacing, int currentState, bool stickHeld, std::int16_t stickHeading,
                                const std::function<bool(const ClimbBarLink&)>& linkEnabled) {
    const Vec forward = facingRow(playerFacing);
    int main = -1, other = -1;
    for (int i = 0; i < int(pole.links.size()); ++i) {
        const ClimbBarLink& link = pole.links[std::size_t(i)];
        if (!link.present || (linkEnabled && !linkEnabled(link))) continue;
        const Vec towards = normalized(sub(vec(link.position), vec(pole.position)));
        if (!(dot(towards, forward) > 0.0f)) continue;
        if (i < 2 && main < 0) main = i;
        else other = i;
    }
    AcrobatPoleJump jump;
    if (main < 0) main = other;
    if (main < 0) return jump;
    if (other >= 0 && main != other && stickHeld) {
        const bool stickUp = std::uint16_t(std::uint32_t(std::uint16_t(stickHeading)) - 0x4000u) > 0x8000u;
        const bool mainHigher = pole.links[std::size_t(main)].position[1] > pole.links[std::size_t(other)].position[1];
        if (stickUp == mainHigher) main = other;
    }
    jump.link = main;
    if (pole.links[std::size_t(main)].bar < 0) jump.state = kAcrobatPoleJumpToLandState;
    else jump.state = currentState == kAcrobatPoleJumpState ? kAcrobatPoleJumpAgainState : kAcrobatPoleJumpState;
    return jump;
}

bool climbBarJumpTick(ClimbBarRider& rider, bool jumpStarted, bool jumping) {
    if (jumpStarted) {
        rider.jumpCount = 0;
        return false;
    }
    if (!jumping || rider.jumpCount < 0) {
        rider.jumpCount = -1;
        return false;
    }
    const bool test = rider.jumpCount > kClimbBarGrabJumpCount;
    ++rider.jumpCount;
    return test;
}

void climbBarLeave(ClimbBarRider& rider) {
    rider.ignoreBar = rider.bar;
}

}
