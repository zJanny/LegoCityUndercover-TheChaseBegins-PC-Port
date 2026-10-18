#include "runtime/brick_grab.hpp"
#include "runtime/level_start.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace chase::runtime {
namespace {

using Vec = std::array<float, 3>;

constexpr float kPi = 3.14159265358979f;

Vec vec(const float v[3]) { return {v[0], v[1], v[2]}; }

Vec sub(const Vec& a, const Vec& b) { return {a[0] - b[0], a[1] - b[1], a[2] - b[2]}; }

float dot(const Vec& a, const Vec& b) {
    float sum = a[0] * b[0];
    sum += a[1] * b[1];
    sum += a[2] * b[2];
    return sum;
}

// 0x257ea0: the vector times 1 / length, unchanged when the length is below 2^-23.
Vec normalized(const Vec& v) {
    const float size = std::sqrt(dot(v, v));
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

int objectType(const assets::Level& cls) {
    const std::uint32_t typeHash = assets::nameHash("ObjectType");
    for (const std::vector<assets::LevelParam>* list : {&cls.classParams, &cls.defaults})
        for (const assets::LevelParam& param : *list)
            if (param.hash == typeHash) return int(param.value);
    return -1;
}

// The heading (ctrl+8 / ctrl+0xa) of a node turned by pi from the brick: 0x1f8384 / 0x24765c of the negated forward row.
std::int16_t hangingHeading(const GrabBrick& brick) {
    return std::int16_t(int(std::atan2(-brick.forward()[0], -brick.forward()[2]) * kBrickHeadingScale));
}

bool jumpState(BrickState state) { return state == BrickState::JumpUp || state == BrickState::JumpLeft || state == BrickState::JumpRight; }

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

const BrickClip* clipFor(const BrickInput& input, BrickState state) {
    const auto found = input.clips.find(brickStateAnimation(state));
    return found == input.clips.end() ? nullptr : &found->second;
}

// 0x202504: separating-axis test of box A (half extents `halfA`, at the origin of its own frame) and box B (centre `c` and half
// extents `halfB` in A's frame, r[i][j] = B axis i . A axis j), transcribed in its order.
bool boxesOverlap(const float halfA[3], const float c[3], const float halfB[3], const float r[3][3]) {
    for (int i = 0; i < 3; ++i)
        if (halfA[i] + std::fabs(r[0][i]) * halfB[0] + halfB[1] * std::fabs(r[1][i]) + halfB[2] * std::fabs(r[2][i]) < std::fabs(c[i]))
            return false;
    for (int j = 0; j < 3; ++j)
        if (std::fabs(r[j][0]) * halfA[0] + halfA[1] * std::fabs(r[j][1]) + halfA[2] * std::fabs(r[j][2]) + halfB[j] <
            std::fabs(c[0] * r[j][0] + c[1] * r[j][1] + c[2] * r[j][2]))
            return false;
    const float a0 = halfA[0], a1 = halfA[1], a2 = halfA[2], b0 = halfB[0], b1 = halfB[1], b2 = halfB[2];
    const float c0 = c[0], c1 = c[1], c2 = c[2];
    const auto m = [&](int i, int j) { return std::fabs(r[i][j]); };
    return std::fabs(c2 * r[0][1] - c1 * r[0][2]) <= m(0, 2) * a1 + a2 * m(0, 1) + m(2, 0) * b1 + b2 * m(1, 0) &&
           std::fabs(c2 * r[1][1] - c1 * r[1][2]) <= m(1, 2) * a1 + a2 * m(1, 1) + m(2, 0) * b0 + b2 * m(0, 0) &&
           std::fabs(c2 * r[2][1] - c1 * r[2][2]) <= m(2, 2) * a1 + a2 * m(2, 1) + m(1, 0) * b0 + b1 * m(0, 0) &&
           std::fabs(c0 * r[0][2] - c2 * r[0][0]) <= m(0, 2) * a0 + a2 * m(0, 0) + m(2, 1) * b1 + b2 * m(1, 1) &&
           std::fabs(c0 * r[1][2] - c2 * r[1][0]) <= m(1, 2) * a0 + a2 * m(1, 0) + m(2, 1) * b0 + b2 * m(0, 1) &&
           std::fabs(c0 * r[2][2] - c2 * r[2][0]) <= m(2, 2) * a0 + a2 * m(2, 0) + m(1, 1) * b0 + b1 * m(0, 1) &&
           std::fabs(c1 * r[0][0] - c0 * r[0][1]) <= m(0, 1) * a0 + a1 * m(0, 0) + m(2, 2) * b1 + b2 * m(1, 2) &&
           std::fabs(c1 * r[1][0] - c0 * r[1][1]) <= m(1, 1) * a0 + a1 * m(1, 0) + m(2, 2) * b0 + b2 * m(0, 2) &&
           std::fabs(c1 * r[2][0] - c0 * r[2][1]) <= m(2, 1) * a0 + a1 * m(2, 0) + m(1, 2) * b0 + b1 * m(0, 2);
}

BrickStep holdingStep(const Vec& position, std::int16_t facing, BrickState state) {
    BrickStep step;
    step.holding = true;
    std::copy(position.begin(), position.end(), step.position);
    step.facing = facing;
    step.animation = brickStateAnimation(state);
    return step;
}

// Every state change sends the released event 5 (0x3233e8: ctrl+0x2b8 = 0, the brick's +0xb8 cleared).
void release(BrickClimber& climber, std::vector<GrabBrick>& bricks) {
    if (climber.brick >= 0 && std::size_t(climber.brick) < bricks.size()) bricks[std::size_t(climber.brick)].held = false;
}

void changeState(BrickClimber& climber, std::vector<GrabBrick>& bricks, BrickState state) {
    release(climber, bricks);
    climber.state = state;
    climber.stateUpdates = 0;
    climber.elapsed = 0;
}

// Enter of the grab states 141..144 (0x32c4b8) and of the idle and reach states 145..148 (0x32c5b8): ctrl+0x130 = ctrl+0x134, the
// brick's +0xb8 = the player, the player node = the brick node 2 lower turned by pi.
BrickStep enterHolding(BrickClimber& climber, std::vector<GrabBrick>& bricks, BrickState state) {
    changeState(climber, bricks, state);
    climber.brick = climber.target;
    GrabBrick& brick = bricks[std::size_t(climber.brick)];
    brick.held = true;
    const Vec position = {brick.position()[0], brick.position()[1] - kBrickHangDrop, brick.position()[2]};
    BrickStep step = holdingStep(position, hangingHeading(brick), state);
    step.entered = state;
    return step;
}

BrickStep leave(BrickClimber& climber, std::vector<GrabBrick>& bricks, const BrickInput& input, BrickExit exit, float fallSpeed) {
    release(climber, bricks);
    climber.state = BrickState::None;
    climber.stateUpdates = 0;
    climber.jumpCount = -1;
    BrickStep step;
    std::copy_n(input.position, 3, step.position);
    step.facing = input.facing;
    step.exit = exit;
    step.fallSpeed = fallSpeed;
    return step;
}

// Enter of the jump states 149..151 (0x32c6fc): the root motion of the clip is scaled per axis so that it ends at the target brick: the
// way between the two hanging points in the current brick's frame (0x229a40) divided by the extent of the root track (0x2147e4,
// 0x1ffa30), x negated for 151 and z always negated (0x211a00). The heading ctrl+0xa becomes the target's.
BrickStep enterJump(BrickClimber& climber, std::vector<GrabBrick>& bricks, BrickState state, int target, const BrickInput& input) {
    changeState(climber, bricks, state);
    climber.target = target;
    const GrabBrick& from = bricks[std::size_t(climber.brick)];
    const GrabBrick& to = bricks[std::size_t(target)];
    const Vec way = sub(vec(to.position()), vec(from.position()));
    const float local[3] = {dot(way, vec(from.right())), dot(way, vec(from.up())), dot(way, vec(from.forward()))};
    const BrickClip* clip = clipFor(input, state);
    for (int a = 0; a < 3; ++a) {
        const float extent = clip && clip->motion ? clip->motion->maximum[std::size_t(a)] - clip->motion->minimum[std::size_t(a)] : 0.0f;
        climber.scale[a] = extent != 0.0f ? local[a] / extent : 0.0f;
    }
    if (state == BrickState::JumpRight) climber.scale[0] = -climber.scale[0];
    climber.scale[2] = -climber.scale[2];
    std::copy_n(input.position, 3, climber.start);
    BrickStep step = holdingStep(vec(input.position), input.facing, state);
    step.entered = state;
    return step;
}

// 0x31a49c, event 6 (B): IsTopBrick starts state 152; else the free adjacent brick that lies best along the stick (or, without the
// stick, along the camera turned towards the wall) starts a jump state; with none the jump state 4 starts.
BrickStep jumpEvent(BrickClimber& climber, std::vector<GrabBrick>& bricks, const BrickInput& input) {
    const GrabBrick& current = bricks[std::size_t(climber.brick)];
    if (current.topBrick) {
        changeState(climber, bricks, BrickState::TopJump);
        std::copy_n(input.position, 3, climber.start);
        climber.lastY = input.position[1];
        BrickStep step = holdingStep(vec(input.position), input.facing, BrickState::TopJump);
        step.entered = BrickState::TopJump;
        return step;
    }
    float yaw;
    if (input.stickHeld) {
        yaw = float(input.stickHeading) * (2.0f * kPi / 65536.0f);
    } else {
        const Vec camera = normalized(vec(input.cameraForward));
        const Vec wall = normalized({-current.forward()[0], -current.forward()[1], -current.forward()[2]});
        yaw = std::acos(std::clamp(dot(camera, wall), -1.0f, 1.0f)) + input.cameraYaw;
    }
    Vec direction = {std::sin(yaw), 0.0f, std::cos(yaw)};
    if (dot(direction, vec(current.forward())) < kBrickUpwardLimit) {
        direction[1] = current.up()[1];
        direction = normalized(direction);
    }
    int best = -1;
    float bestScore = -1.0f;
    Vec bestWay{};
    for (const int adjacent : current.adjacent) {
        if (adjacent < 0) continue;
        const GrabBrick& candidate = bricks[std::size_t(adjacent)];
        if (candidate.held || (input.enabled && !input.enabled(candidate))) continue;
        const Vec way = normalized(sub(vec(candidate.position()), vec(current.position())));
        const float score = dot(way, direction);
        if (score > 0.0f && score > bestScore) {
            bestScore = score;
            best = adjacent;
            bestWay = way;
        }
    }
    if (best < 0) return leave(climber, bricks, input, BrickExit::Jump, 0.0f);
    const GrabBrick& target = bricks[std::size_t(best)];
    BrickState state = BrickState::JumpRight;
    if (dot(bestWay, vec(target.up())) > kBrickJumpUpLimit) state = BrickState::JumpUp;
    else if (dot(bestWay, vec(target.right())) > kBrickJumpUpLimit) state = BrickState::JumpLeft;
    return enterJump(climber, bricks, state, best, input);
}

// 0x31c488, event 9 (stick held): the reach state of the stick direction against the brick's right and forward rows.
BrickState reachState(const GrabBrick& brick, std::int16_t stickHeading) {
    const float yaw = float(stickHeading) * (2.0f * kPi / 65536.0f);
    const Vec direction = {std::sin(yaw), 0.0f, std::cos(yaw)};
    const float side = dot(direction, vec(brick.right()));
    if (side > kBrickReachLimit) return BrickState::ReachLeft;
    if (side < -kBrickReachLimit) return BrickState::ReachRight;
    if (dot(direction, vec(brick.forward())) < -kBrickReachLimit) return BrickState::ReachUp;
    return BrickState::Idle;
}

BrickState grabStateAfterJump(BrickState jump) {
    if (jump == BrickState::JumpUp) return BrickState::GrabUp;
    if (jump == BrickState::JumpLeft) return BrickState::GrabLeft;
    return BrickState::GrabRight;
}

}

int brickStateAnimation(BrickState state) {
    switch (state) {
    case BrickState::Grab: return kAnimationBrickGrab;
    case BrickState::GrabUp: return kAnimationBrickGrabUp;
    case BrickState::GrabLeft: return kAnimationBrickGrabLeft;
    case BrickState::GrabRight: return kAnimationBrickGrabRight;
    case BrickState::ReachUp: return kAnimationBrickReachUp;
    case BrickState::ReachLeft: return kAnimationBrickReachLeft;
    case BrickState::ReachRight: return kAnimationBrickReachRight;
    case BrickState::JumpUp: return kAnimationBrickJumpUp;
    case BrickState::JumpLeft: return kAnimationBrickJumpLeft;
    case BrickState::JumpRight: return kAnimationBrickJumpRight;
    case BrickState::TopJump: return kAnimationBrickTopJump;
    case BrickState::Idle:
    case BrickState::None: break;
    }
    return kAnimationBrickIdle;
}

std::vector<GrabBrick> loadGrabBricks(const assets::Level& level, const assets::ClassLookup& classOf) {
    std::vector<GrabBrick> bricks;
    std::vector<std::array<std::uint32_t, kAdjacentBrickCount>> links;
    std::map<std::uint32_t, std::size_t> byHash;
    for (const assets::LevelObject& object : level.objects) {
        const assets::Level* cls = classOf(object.className);
        if (!cls || objectType(*cls) != kBrickGrabObjectType) continue;
        const std::optional<assets::Placement> placed = assets::objectStart(level, object.instance);
        if (!placed) continue;
        GrabBrick brick;
        brick.instance = lower(object.instance);
        brick.matrix = assets::placementMatrix(*placed);
        brick.adjacent.fill(-1);
        brick.topBrick = intParam(object, *cls, "IsTopBrick", 0) != 0;
        for (const assets::LevelBound& bound : cls->bounds) {
            if (lower(bound.name) != "grabbound") continue;
            brick.hasGrabBound = true;
            std::copy_n(bound.centre, 3, brick.grabCentre);
            std::copy_n(bound.half, 3, brick.grabHalf);
        }
        std::array<std::uint32_t, kAdjacentBrickCount> link{};
        for (std::size_t i = 0; i < kAdjacentBrickCount; ++i) {
            char name[32];
            std::snprintf(name, sizeof name, "AdjacentBrick%02d", int(i + 1));
            const assets::LevelParam* param = effectiveParam(object, *cls, assets::nameHash(name));
            link[i] = param ? assets::objectReferenceHash(*param) : 0;
        }
        byHash[assets::nameHash(object.instance)] = bricks.size();
        links.push_back(link);
        bricks.push_back(brick);
    }
    for (std::size_t b = 0; b < bricks.size(); ++b)
        for (std::size_t i = 0; i < kAdjacentBrickCount; ++i) {
            const auto found = byHash.find(links[b][i]);
            if (links[b][i] != 0 && found != byHash.end()) bricks[b].adjacent[i] = int(found->second);
        }
    return bricks;
}

void updateGrabBricks(std::vector<GrabBrick>& bricks) {
    for (GrabBrick& brick : bricks) {
        if (brick.held) brick.cooldown = kBrickReleaseCooldown;
        else if (brick.cooldown > 0) --brick.cooldown;
    }
}

bool brickJumpTick(BrickClimber& climber, bool jumpStarted, bool jumping) {
    if (jumpStarted) {
        climber.jumpCount = 0;
        return false;
    }
    if (!jumping || climber.jumpCount < 0) {
        climber.jumpCount = -1;
        return false;
    }
    const bool test = climber.jumpCount > kBrickGrabJumpCount;
    ++climber.jumpCount;
    return test;
}

int brickGrabTest(const std::vector<GrabBrick>& bricks, const float position[3], std::int16_t facing,
                  const std::function<bool(const GrabBrick&)>& enabled) {
    const float heading = float(facing) * (2.0f * kPi / 65536.0f);
    const Vec right = {std::cos(heading), 0.0f, -std::sin(heading)};
    const Vec up = {0.0f, 1.0f, 0.0f};
    const Vec forward = {std::sin(heading), 0.0f, std::cos(heading)};
    const Vec rows[3] = {right, up, forward};
    const float half[3] = {kStandInPlayerGrabHalf[0], kStandInPlayerGrabHalf[1] * kBrickGrabBoxHeightScale, kStandInPlayerGrabHalf[2]};
    const Vec centre = {position[0] + kStandInPlayerGrabOffset[0],
                        position[1] + (kStandInPlayerGrabOffset[1] + kStandInPlayerGrabOffset[1] * kBrickGrabBoxRaise),
                        position[2] + kStandInPlayerGrabOffset[2]};
    for (std::size_t i = 0; i < bricks.size(); ++i) {
        const GrabBrick& brick = bricks[i];
        if (brick.held || brick.cooldown != 0 || !brick.hasGrabBound || (enabled && !enabled(brick))) continue;
        if (dot(vec(brick.forward()), forward) > kBrickGrabFacingLimit) continue;
        const Vec boundCentre = {
            brick.position()[0] + brick.grabCentre[0] * brick.right()[0] + brick.grabCentre[1] * brick.up()[0] + brick.grabCentre[2] * brick.forward()[0],
            brick.position()[1] + brick.grabCentre[0] * brick.right()[1] + brick.grabCentre[1] * brick.up()[1] + brick.grabCentre[2] * brick.forward()[1],
            brick.position()[2] + brick.grabCentre[0] * brick.right()[2] + brick.grabCentre[1] * brick.up()[2] + brick.grabCentre[2] * brick.forward()[2]};
        const Vec offset = sub(boundCentre, centre);
        const float local[3] = {dot(offset, right), dot(offset, up), dot(offset, forward)};
        const Vec brickRows[3] = {vec(brick.right()), vec(brick.up()), vec(brick.forward())};
        float r[3][3];
        for (int a = 0; a < 3; ++a)
            for (int b = 0; b < 3; ++b) r[a][b] = dot(brickRows[a], rows[b]);
        if (boxesOverlap(half, local, brick.grabHalf, r)) return int(i);
    }
    return -1;
}

BrickStep brickGrabEnter(BrickClimber& climber, std::vector<GrabBrick>& bricks, int brick) {
    climber.jumpCount = -1;
    climber.target = brick;
    return enterHolding(climber, bricks, BrickState::Grab);
}

BrickStep brickGrabUpdate(BrickClimber& climber, std::vector<GrabBrick>& bricks, const BrickInput& input) {
    ++climber.stateUpdates;
    const BrickState state = climber.state;
    if (jumpState(state)) {
        const GrabBrick& from = bricks[std::size_t(climber.brick)];
        const BrickClip* clip = clipFor(input, state);
        climber.elapsed += input.dt;
        Vec position = vec(climber.start);
        if (clip && clip->motion) {
            const Vec root = rootAt(*clip->motion, climber.elapsed * clip->framesPerSecond);
            for (int a = 0; a < 3; ++a)
                position[std::size_t(a)] += -climber.scale[0] * root[0] * from.right()[a] + climber.scale[1] * root[1] * from.up()[a] -
                                            climber.scale[2] * root[2] * from.forward()[a];
        }
        if (!clip || climber.stateUpdates >= clip->updates) return enterHolding(climber, bricks, grabStateAfterJump(state));
        return holdingStep(position, input.facing, state);
    }
    if (state == BrickState::TopJump) {
        const BrickClip* clip = clipFor(input, state);
        climber.elapsed += input.dt;
        Vec position = vec(climber.start);
        if (clip && clip->motion) {
            const float heading = float(input.facing) * (2.0f * kPi / 65536.0f);
            const Vec root = rootAt(*clip->motion, climber.elapsed * clip->framesPerSecond);
            position[0] += root[0] * std::cos(heading) + root[2] * std::sin(heading);
            position[1] += root[1];
            position[2] += -root[0] * std::sin(heading) + root[2] * std::cos(heading);
        }
        const float before = climber.lastY;
        climber.lastY = position[1];
        if (!clip || climber.stateUpdates >= clip->updates) {
            BrickInput moved = input;
            std::copy(position.begin(), position.end(), moved.position);
            return leave(climber, bricks, moved, BrickExit::Fall, before - position[1]);
        }
        return holdingStep(position, input.facing, state);
    }
    if (input.jumpPressed) return jumpEvent(climber, bricks, input);
    if (input.usePressed) return leave(climber, bricks, input, BrickExit::Fall, 0.0f);
    const GrabBrick& brick = bricks[std::size_t(climber.brick)];
    if (input.enabled && !input.enabled(brick)) return leave(climber, bricks, input, BrickExit::Fall, 0.0f);
    if (int(state) <= int(BrickState::GrabRight)) {
        const BrickClip* clip = clipFor(input, state);
        if (!clip || climber.stateUpdates >= clip->updates) {
            climber.target = climber.brick;
            return enterHolding(climber, bricks, BrickState::Idle);
        }
    } else {
        const BrickState wanted = input.stickHeld ? reachState(brick, input.stickHeading) : BrickState::Idle;
        if (wanted != state) {
            climber.target = climber.brick;
            return enterHolding(climber, bricks, wanted);
        }
    }
    return holdingStep(vec(input.position), input.facing, state);
}

}
