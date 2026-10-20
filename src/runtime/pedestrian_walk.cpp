#include "runtime/pedestrian_walk.hpp"

#include "runtime/dcam.hpp"
#include "runtime/detective.hpp"

#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>

namespace chase::runtime {

namespace {

using Node = std::array<PedestrianVec3, 4>;

constexpr std::int32_t kNormalizeMinimumBits = 0x34000000;
constexpr std::int32_t kAnglesMinimumBits = 0x36000000;
constexpr float kLookAtUpLimit = 0.98f;
constexpr float kPi = 3.14159274f;
constexpr float kHalfPi = 1.57079637f;
constexpr float kHeadingUnitsPerRadian = 10430.378f;
constexpr float kRadiansPerHeadingUnit = 9.58738e-05f;
constexpr int kModeOnce = 0;
constexpr int kModeLoop = 1;
constexpr std::uint32_t kNodeTypeMask = 0x1f;
constexpr std::uint8_t kPathTypeSpline = 0;
constexpr std::uint8_t kPathTypeLinear = 1;
const PedestrianVec3 kUp{0.0f, 1.0f, 0.0f};
const PedestrianVec3 kZeroVector{0.0f, 0.0f, 0.0f};  // 0x56d400, (0, 0, 0) in the oracle (docs/re/debris.md)

std::int32_t armFloatToInt(float value) {
    if (std::isnan(value)) return 0;
    if (value >= 2147483648.0f) return std::numeric_limits<std::int32_t>::max();
    if (value < -2147483648.0f) return std::numeric_limits<std::int32_t>::min();
    return std::int32_t(value);
}

std::int32_t floatBits(float value) {
    std::int32_t bits = 0;
    std::memcpy(&bits, &value, sizeof bits);
    return bits;
}

float length(const PedestrianVec3& v) {
    float squared = v[0] * v[0];
    squared = squared + v[1] * v[1];
    squared = squared + v[2] * v[2];
    return std::sqrt(squared);
}

PedestrianVec3 unit(const PedestrianVec3& v) {
    const float size = length(v);
    if (floatBits(size) <= kNormalizeMinimumBits) return {0.0f, 0.0f, 1.0f};
    const float inverse = 1.0f / size;
    return {v[0] * inverse, v[1] * inverse, v[2] * inverse};
}

PedestrianVec3 cross(const PedestrianVec3& a, const PedestrianVec3& b) {
    return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}

PedestrianVec3 subtract(const PedestrianVec3& a, const PedestrianVec3& b) { return {a[0] - b[0], a[1] - b[1], a[2] - b[2]}; }

PedestrianVec3 lerp(const PedestrianVec3& a, const PedestrianVec3& b, float t) {
    return {a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t, a[2] + (b[2] - a[2]) * t};
}

float horizontalLength(const PedestrianVec3& v) {
    float squared = v[0] * v[0];
    squared = squared + v[2] * v[2];
    return std::sqrt(squared);
}

float horizontalDot(const PedestrianVec3& a, const PedestrianVec3& b) {
    float sum = a[0] * b[0];
    sum = sum + a[2] * b[2];
    return sum;
}

float horizontalDistanceSquared(const PedestrianVec3& a, const PedestrianVec3& b) {
    const float dx = a[0] - b[0];
    const float dz = a[2] - b[2];
    float sum = dx * dx;
    sum = sum + dz * dz;
    return sum;
}

PedestrianClip dodgeClip(std::uint8_t index) { return PedestrianClip(std::uint8_t(PedestrianClip::DodgeRight01) + index); }
PedestrianClip bumpClip(std::uint8_t index) { return PedestrianClip(std::uint8_t(PedestrianClip::Bump01) + index); }

bool clipFinished(const PedestrianWalkWorld& world, PedestrianClip clip) { return world.clipFinished && world.clipFinished(clip); }

void request(PedestrianWalkUpdate& out, PedestrianClip clip, int mode, bool rootMotion) {
    PedestrianAnimationRequest animation;
    animation.clip = clip;
    animation.mode = mode;
    animation.rootMotion = rootMotion;
    out.animations.push_back(animation);
}

void sampleTarget(PedestrianBody& body, const PedestrianPath& path, float parameter) {
    PedestrianVec3 tangent{0.0f, 0.0f, 1.0f};
    path.sample(parameter, body.target, tangent);
    const float side = -tangent[2];
    body.target[0] = body.target[0] + side * body.lateral;
    body.target[2] = body.target[2] + tangent[0] * body.lateral;
}

float wrapParameter(float parameter, std::uint16_t pointCount) {
    const float count = float(int(pointCount));
    if (parameter < 0.0f) return count + parameter;
    if (count > parameter) return parameter;
    return parameter - count;
}

void moveAlongPath(PedestrianBody& body, const PedestrianWalkWorld& world, float step) {
    if (!world.path || !world.path->sample) return;
    body.pathParameter = wrapParameter(body.pathParameter + step, world.path->pointCount);
    sampleTarget(body, *world.path, body.pathParameter);
}

void sidestep(PedestrianBody& body, float side) {
    const Node& node = body.node;
    for (int a = 0; a < 3; ++a) body.target[a] = node[3][a] + node[2][a] * kPedestrianSidestep;
    const float amount = kPedestrianSidestep * side;
    for (int a = 0; a < 3; ++a) body.target[a] = body.target[a] + node[0][a] * amount;
    body.avoidDelay = kPedestrianAvoidUpdates;
}

bool playerPushes(const PedestrianWalkWorld& world) { return world.playerState == 3 || world.playerState == 2; }

void probeGround(PedestrianBody& body, const PedestrianWalkWorld& world, PedestrianWalkUpdate& out) {
    out.groundProbed = true;
    if (world.groundProbe) world.groundProbe(body.node[3]);
}

bool tilted(const Node& node) {
    float angle = std::fabs(pedestrianTiltAngle(node));
    if (floatBits(angle) > floatBits(kHalfPi)) angle = std::fabs(angle - kPi);
    return angle > kPedestrianTiltLimit;
}

void facePlayer(PedestrianBody& body, const PedestrianWalkWorld& world) {
    const PedestrianVec3 d = subtract(world.playerPosition, body.node[3]);
    const float heading = std::atan2(d[0], d[2]) * kHeadingUnitsPerRadian;
    const std::uint16_t units = std::uint16_t(std::int32_t(heading));
    const float angle = float(std::int32_t(units)) * kRadiansPerHeadingUnit;
    const float s = std::sin(angle);
    const float c = std::cos(angle);
    body.node[0] = {c, 0.0f, -s};
    body.node[1] = {0.0f, 1.0f, 0.0f};
    body.node[2] = {s, 0.0f, c};
}

}

const char* pedestrianClipName(PedestrianClip clip) {
    switch (clip) {
    case PedestrianClip::Walk: return "walk_minifig";
    case PedestrianClip::DodgeRight01: return "dodge_right01_minifig";
    case PedestrianClip::DodgeLeft01: return "dodge_left01_minifig";
    case PedestrianClip::DodgeRight02: return "dodge_right02_minifig";
    case PedestrianClip::DodgeLeft02: return "dodge_left02_minifig";
    case PedestrianClip::Bump01: return "bump01_minifig";
    case PedestrianClip::Bump02: return "bump02_minifig";
    case PedestrianClip::Splat: return "splat_minifig";
    case PedestrianClip::SplatUp: return "splat_up_minifig";
    }
    return "";
}

PedestrianBody pedestrianCreate(bool isMale, const std::array<PedestrianVec3, 4>& node) {
    PedestrianBody body;
    body.voice = isMale ? 4 : 8;
    body.node = node;
    return body;
}

PedestrianPath pedestrianLevelPath(const assets::LevelPath& path) {
    PedestrianPath result;
    result.pointCount = std::uint16_t(path.points.size());
    const assets::LevelPath* source = &path;
    if (path.type == kPathTypeSpline) {
        result.sample = [source](float parameter, PedestrianVec3& position, PedestrianVec3& tangent) {
            sampleDirectorPath(*source, parameter, position.data());
            detectivePathTangent(*source, parameter, tangent.data());
        };
    } else if (path.type == kPathTypeLinear) {
        result.sample = [source](float parameter, PedestrianVec3& position, PedestrianVec3& tangent) {
            pedestrianLinearPathSample(*source, parameter, position, tangent);
        };
    } else {
        throw std::invalid_argument("pedestrian path type " + std::to_string(int(path.type)) + " has no sampler");
    }
    return result;
}

void pedestrianLinearPathSample(const assets::LevelPath& path, float parameter, PedestrianVec3& position, PedestrianVec3& tangent) {
    const std::int32_t count = std::int32_t(path.points.size());
    if (count == 0) throw std::invalid_argument("pedestrian path without points");
    float p = parameter;
    if (!path.closed) {
        if (p < 0.0f) p = 0.0f;
        const float last = float(count - 1);
        if (last < p) p = last;
    }
    std::int32_t index = armFloatToInt(std::floor(p));
    std::int32_t next = index + 1;
    const float fraction = p - float(index);
    if (path.closed) {
        index = index % count;
        if (index < 0) index = index + count;
        next = next % count;
        if (next < 0) next = count + next;
    }
    const float at = float(index) + fraction;
    const PedestrianVec3& from = path.points[std::size_t(index)];
    PedestrianVec3 d{0.0f, 0.0f, 0.0f};
    if (next < count) d = subtract(path.points[std::size_t(next)], from);
    const float scale = at - float(index);
    for (float& value : d) value = value * scale;
    tangent = unit(d);
    for (int a = 0; a < 3; ++a) position[a] = d[a] + from[a];
}

PedestrianEntityBox pedestrianEntityBox(const PedestrianVec3& boundsCentre, const PedestrianVec3& boundsHalf) {
    PedestrianEntityBox box;
    for (int a = 0; a < 3; ++a) {
        const float minimum = boundsCentre[a] - boundsHalf[a];
        const float maximum = boundsCentre[a] + boundsHalf[a];
        box.centre[a] = (minimum + maximum) * kPedestrianBoxScale;
        box.half[a] = (maximum - minimum) * kPedestrianBoxScale;
    }
    box.radius = length(box.half);
    return box;
}

void pedestrianGroundProbe(PedestrianVec3& position,
                           const std::function<std::optional<float>(const PedestrianVec3& from, const PedestrianVec3& to)>& ray) {
    if (!ray) return;
    const PedestrianVec3 from{position[0], position[1] + kPedestrianProbeAbove, position[2]};
    const PedestrianVec3 to{position[0], position[1] - kPedestrianProbeBelow, position[2]};
    if (const std::optional<float> height = ray(from, to)) position[1] = *height;
}

float pedestrianDistanceSquared(const PedestrianVec3& a, const PedestrianVec3& b) {
    const float dx = a[0] - b[0];
    const float dy = a[1] - b[1];
    const float dz = a[2] - b[2];
    float squared = dx * dx;
    squared = squared + dy * dy;
    squared = squared + dz * dz;
    return squared;
}

bool pedestrianNodeDrawn(const PedestrianNode& node, std::uint32_t meshNodeType, const DistanceFadeTable& fade, float distanceSquared) {
    if ((node.word & kNodeTypeMask) != meshNodeType) return true;
    const int tier = pedestrianFadeTier(node, fade);
    if (tier < 0) return true;
    const float reach = fade.fadeIn[std::size_t(tier)];
    const float reachSquared = reach * reach;
    return reachSquared > distanceSquared;
}

bool pedestrianUnderVehicle(const PedestrianVehicleBox& vehicle, const PedestrianVec3& position) {
    const PedestrianVec3 d = subtract(position, vehicle.node[3]);
    auto along = [&](int row) {
        float sum = d[0] * vehicle.node[std::size_t(row)][0];
        sum = sum + d[1] * vehicle.node[std::size_t(row)][1];
        sum = sum + d[2] * vehicle.node[std::size_t(row)][2];
        return sum;
    };
    if (!(vehicle.halfX > std::fabs(along(0)))) return false;
    return std::fabs(along(2)) < vehicle.halfZ;
}

void pedestrianLookAt(std::array<PedestrianVec3, 4>& node, const PedestrianVec3& from, const PedestrianVec3& to) {
    node[2] = unit(subtract(to, from));
    if (floatBits(std::fabs(node[2][1])) > floatBits(kLookAtUpLimit))
        node[0] = {0.0f, -node[2][2], node[2][1]};
    else
        node[0] = {node[2][2], 0.0f, -node[2][0]};
    node[0] = unit(node[0]);
    node[1] = cross(node[2], node[0]);
}

float pedestrianTiltAngle(const std::array<PedestrianVec3, 4>& node) {
    float squared = node[0][0] * node[0][0];
    squared = squared + node[1][0] * node[1][0];
    const float planar = std::sqrt(squared);
    PedestrianVec3 first;
    if (floatBits(planar) > kAnglesMinimumBits) {
        first = {std::atan2(-node[2][1], node[2][2]), std::atan2(node[2][0], planar), std::atan2(-node[1][0], node[0][0])};
    } else {
        first = {std::atan2(node[1][2], node[1][1]), std::atan2(node[2][0], planar), 0.0f};
    }
    const PedestrianVec3 second{first[0] + kPi, kPi - first[1], first[2] + kPi};
    return length(first) < length(second) ? first[0] : second[0];
}

int PedestrianWalk::avoidance(const PedestrianBody& body, const PedestrianWalkWorld& world) {
    const PedestrianVec3& position = body.node[3];
    auto side = [&](const PedestrianVec3& other) -> int {
        const PedestrianVec3 d = subtract(other, position);
        if (!(horizontalLength(d) < kPedestrianAvoidDistance)) return 0;
        const float lateral = horizontalDot(d, body.node[0]);
        if (!(std::fabs(lateral) < kPedestrianAvoidLateral)) return 0;
        return lateral > 0.0f ? 2 : 1;
    };
    if (const int result = side(world.playerPosition)) return result;
    for (const PedestrianVec3& other : world.otherPedestrians)
        if (const int result = side(other)) return result;
    return 0;
}

void PedestrianWalk::wallTest(PedestrianBody& body, const PedestrianWalkWorld& world, PedestrianVec3& move) {
    const float nearRadius = world.arrivalRadius * kPedestrianWallNearScale;
    PedestrianVec3 slid{move[0], 0.0f, move[2]};
    if (world.wallSlide) slid = world.wallSlide(body.node[3], slid, nearRadius);
    body.node[3][0] = body.node[3][0] + slid[0];
    body.node[3][2] = body.node[3][2] + slid[2];
    move[0] = slid[0];
    move[1] = move[1] + slid[1];
    move[2] = slid[2];
}

bool PedestrianWalk::contact(PedestrianBody& body, const PedestrianWalkWorld& world, PedestrianVec3& velocity, bool wall, bool probe,
                             PedestrianWalkUpdate& out) {
    if (world.playerInVehicle && !probe) return false;
    bool touching = false;
    const float squared = horizontalDistanceSquared(body.node[3], world.playerPosition);
    if (floatBits(squared) < floatBits(kPedestrianTouchDistanceSquared)) {
        touching = true;
        if (playerPushes(world)) {
            const PedestrianVec3 away = unit(subtract(body.node[3], world.playerPosition));
            velocity[0] = velocity[0] + away[0] * kPedestrianPushStep;
            velocity[2] = velocity[2] + away[2] * kPedestrianPushStep;
        }
        if (wall) wallTest(body, world, velocity);
    }
    if (probe) probeGround(body, world, out);
    return touching;
}

void PedestrianWalk::rootMotion(PedestrianBody& body, const PedestrianWalkWorld& world, PedestrianWalkUpdate& out) {
    if (!world.rootTranslation) return;
    const PedestrianVec3& root = *world.rootTranslation;
    const Node& node = body.node;
    PedestrianVec3 move = kZeroVector;
    for (int c = 0; c < 3; ++c) {
        float moved = root[0] * node[0][c];
        moved = moved + root[1] * node[1][c];
        moved = moved + root[2] * node[2][c];
        moved = moved + node[3][c];
        move[c] = move[c] + (-node[3][c] + moved);
    }
    wallTest(body, world, move);
    probeGround(body, world, out);
}

void PedestrianWalk::bumped(PedestrianBody& body, const PedestrianWalkWorld& world, PedestrianWalkUpdate& out) {
    PedestrianVec3 push = kZeroVector;
    if (!contact(body, world, push, true, true, out)) return;
    for (int a = 0; a < 3; ++a) body.node[3][a] = body.node[3][a] + push[a];
}

void PedestrianWalk::walk(PedestrianBody& body, const PedestrianWalkWorld& world, PedestrianWalkUpdate& out) {
    out.walked = true;
    Node& node = body.node;
    const float dt = world.dt;
    const float turn = dt * kPedestrianTurnRate;
    Node look = node;
    pedestrianLookAt(look, node[3], body.target);
    node[2][1] = look[2][1];
    node[0][1] = look[0][1];
    node[0] = lerp(node[0], look[0], turn);
    node[2] = lerp(node[2], look[2], turn);
    const float distance = body.speed * dt;
    PedestrianVec3 velocity{node[2][0] * distance, node[2][1] * distance, node[2][2] * distance};

    bool localToken = false;
    bool& token = world.groundProbeTaken ? *world.groundProbeTaken : localToken;
    bool probe = false;
    if (!token) {
        probe = true;
        token = true;
    } else if (body.avoidDelay != 0 && tilted(node)) {
        probe = true;
    }

    const bool touching = contact(body, world, velocity, false, probe, out);
    if (touching && playerPushes(world)) {
        body.requested = 5;
        if (world.groundRay) {
            const PedestrianVec3 to{node[3][0], kPedestrianBumpRayEndY, node[3][2]};
            if (const std::optional<float> height = world.groundRay(node[3], to)) node[3][1] = *height;
        }
    }

    int result = world.blocked ? world.blocked(node[3], velocity, dt) : 0;
    for (int a = 0; a < 3; ++a) node[3][a] = node[3][a] + velocity[a];
    const PedestrianVec3 right = unit(cross(kUp, node[2]));
    node[2] = unit(cross(right, kUp));
    node[1] = kUp;
    node[0] = right;

    bool checkArrival = false;
    if (result == 0) {
        if (body.avoidDelay != 0) checkArrival = true;
        else result = avoidance(body, world);
    }
    if (!checkArrival && result == 4) {
        body.forward = !body.forward;
        moveAlongPath(body, world, body.forward ? kPedestrianReverseStep : -kPedestrianReverseStep);
    } else if (checkArrival || result == 0 || result == 3) {
        if (length(subtract(node[3], body.target)) < world.arrivalRadius) {
            if (result == 0) moveAlongPath(body, world, body.forward ? kPedestrianPathStep : -kPedestrianPathStep);
            else sidestep(body, -1.0f);
        }
    } else {
        sidestep(body, result == 1 ? 1.0f : -1.0f);
    }

    if (body.requested == 3 || body.requested == 4) pedestrianLookAt(node, node[3], world.playerPosition);
}

void PedestrianWalk::enter(PedestrianBody& body, const PedestrianWalkWorld& world, GameRandom& random, PedestrianWalkUpdate& out) {
    switch (body.requested) {
    case 1:
        if (world.path && world.path->sample)
            sampleTarget(body, *world.path, body.pathParameter + (body.forward ? kPedestrianEntryLookAhead : -kPedestrianEntryLookAhead));
        body.hold = random.next() % kPedestrianHoldRange + kPedestrianHoldBase;
        body.walkStarted = true;
        request(out, PedestrianClip::Walk, kModeLoop, false);
        break;
    case 3:
    case 4:
        if (body.requested == 4) body.dodgeClip = body.dodgeSide == 1 ? 2 : 0;
        else body.dodgeClip = body.dodgeSide == 1 ? 3 : 1;
        request(out, dodgeClip(body.dodgeClip), kModeOnce, true);
        break;
    case 5:
        body.bumpClip = std::uint8_t(random.next() % 2);
        request(out, bumpClip(body.bumpClip), kModeOnce, false);
        facePlayer(body, world);
        break;
    case 6:
        request(out, PedestrianClip::Splat, kModeLoop, false);
        body.splatTimer = kPedestrianSplatSeconds;
        break;
    case 7:
        request(out, PedestrianClip::SplatUp, kModeOnce, true);
        break;
    default:
        break;
    }
    body.state = body.requested;
}

void PedestrianWalk::step(PedestrianBody& body, const PedestrianWalkWorld& world) {
    switch (body.state) {
    case 1:
        body.requested = 2;
        break;
    case 3:
    case 4:
        if (clipFinished(world, dodgeClip(body.dodgeClip))) body.requested = 1;
        break;
    case 5:
        if (clipFinished(world, bumpClip(body.bumpClip))) body.requested = 1;
        break;
    case 6:
        if (world.playerVehicleBox ? pedestrianUnderVehicle(*world.playerVehicleBox, body.node[3]) : world.splatRefresh)
            body.splatTimer = kPedestrianSplatSeconds;
        if (body.splatTimer > 0.0f) {
            body.splatTimer = body.splatTimer - world.dt;
            if (body.splatTimer <= 0.0f) body.requested = 7;
        }
        break;
    case 7:
        if (clipFinished(world, PedestrianClip::SplatUp)) body.requested = 1;
        break;
    default:
        break;
    }
    bool visible = world.visible;
    bool hidden = world.hidden;
    if (world.drawTest) {
        const PedestrianDrawTest& test = *world.drawTest;
        visible = pedestrianNodeDrawn(test.node, test.meshNodeType, test.fade, pedestrianDistanceSquared(body.node[3], test.fadePoint));
        hidden = pedestrianNodeHidden(test.node);
    }
    const bool keep = visible && !hidden && (body.requested != 0 || body.state != 0);
    if (keep) {
        if (body.hold <= kPedestrianHoldVisible) body.hold = kPedestrianHoldVisible;
    } else if (body.hold != 0) {
        --body.hold;
    }
    if (body.avoidDelay != 0) --body.avoidDelay;
}

PedestrianWalkUpdate PedestrianWalk::update(PedestrianBody& body, const PedestrianWalkWorld& world, GameRandom& random) {
    PedestrianWalkUpdate out;
    if (body.state == body.requested) {
        switch (body.state) {
        case 2: walk(body, world, out); break;
        case 3:
        case 4:
        case 7: rootMotion(body, world, out); break;
        case 5: bumped(body, world, out); break;
        default: break;
        }
    }
    if (body.state != body.requested) enter(body, world, random, out);
    step(body, world);
    return out;
}

}
