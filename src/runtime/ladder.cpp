#include "runtime/ladder.hpp"
#include "assets/object_classes.hpp"
#include "runtime/level_start.hpp"
#include "runtime/player_intent.hpp"
#include "runtime/player_locomotion.hpp"
#include "runtime/pole.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstring>

namespace chase::runtime {
namespace {

using Vec = std::array<float, 3>;

constexpr float kClamberSnapReach = 0.1f;  // TRAV-S35: floor snap distance of the clamber while it descends (as TRAV-S8 of the pole)
constexpr float kRayReach = 0.1f;          // 0x22f134: the ray ends 0.1 plus its length below the node
constexpr int kJumpOffTurn = 0x3000;       // 0x32cbb0 (Ladder Jump Off Event): ctrl+8 -= 0x3000, ctrl+0xa += 0x8000

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

const assets::LevelParam* param(const assets::LevelObject& object, const assets::Level& cls, const char* name) {
    return effectiveParam(object, cls, assets::nameHash(name));
}

float floatParam(const assets::LevelObject& object, const assets::Level& cls, const char* name, float fallback) {
    const assets::LevelParam* found = param(object, cls, name);
    if (!found) return fallback;
    if (found->type != 0) return float(found->value);
    float value;
    std::memcpy(&value, &found->value, 4);
    return value;
}

int intParam(const assets::LevelObject& object, const assets::Level& cls, const char* name, int fallback) {
    const assets::LevelParam* found = param(object, cls, name);
    if (!found) return fallback;
    if (found->type == 0) {
        float value;
        std::memcpy(&value, &found->value, 4);
        return int(value);
    }
    return int(found->value);
}

std::int16_t headingOf(float x, float z) { return std::int16_t(std::uint16_t(std::int32_t(gameAtan2(x, z) * kLadderHeadingScale))); }

float dot3(const float* a, const Vec& b) {
    float sum = a[0] * b[0];
    sum += a[1] * b[1];
    sum += a[2] * b[2];
    return sum;
}

// 0x2536c0: the player's position in the ladder's frame.
Vec toLocal(const LadderObject& ladder, const float point[3]) {
    const Vec d = {point[0] - ladder.position[0], point[1] - ladder.position[1], point[2] - ladder.position[2]};
    return {dot3(ladder.right, d), dot3(ladder.up, d), dot3(ladder.forward, d)};
}

// 0x221570 switch on FaceTowards: `toObject` is normalized(ladder - player), the forwards are the node rows.
bool faceTestPasses(int mode, const Vec& playerForward, const float objectForward[3], const Vec& toObject) {
    const float facing = playerForward[0] * objectForward[0] + playerForward[1] * objectForward[1] + playerForward[2] * objectForward[2];
    switch (mode) {
    case 1: return playerForward[0] * toObject[0] + playerForward[1] * toObject[1] + playerForward[2] * toObject[2] >= 0.0f;
    case 2: return !(facing > 0.0f) && !(dot3(objectForward, toObject) > 0.0f);
    case 3: return !(facing < 0.0f) && !(dot3(objectForward, toObject) < 0.0f);
    case 4: return !(facing > 0.0f);
    case 5: return !(facing < 0.0f);
    default: return true;
    }
}

Vec rootAt(const assets::RootMotion& motion, float frame) {
    const auto& t = motion.translation;
    if (t.empty()) return {0, 0, 0};
    frame = std::clamp(frame, 0.0f, float(t.size() - 1));
    const std::size_t index = std::size_t(frame);
    if (index + 1 >= t.size()) return {t.back()[0], t.back()[1], t.back()[2]};
    const float f = frame - float(index);
    Vec out;
    for (int a = 0; a < 3; ++a) out[a] = t[index][a] + (t[index + 1][a] - t[index][a]) * f;
    return out;
}

LadderStep riding(const float position[3], std::int16_t facing, int animation) {
    LadderStep step;
    step.riding = true;
    std::copy_n(position, 3, step.position);
    step.facing = facing;
    step.targetHeading = facing;
    step.animation = animation;
    return step;
}

LadderStep leave(LadderClimber& climber, const float position[3], std::int16_t facing, LadderExit exit) {
    LadderStep step;
    std::copy_n(position, 3, step.position);
    step.facing = facing;
    step.targetHeading = facing;
    if (exit == LadderExit::Fall) {
        step.facing = std::int16_t(std::uint16_t(std::int32_t(facing) - kJumpOffTurn));
        step.targetHeading = std::int16_t(std::uint16_t(std::int32_t(facing) + 0x8000));
    }
    step.exit = exit;
    climber = LadderClimber();
    return step;
}

std::uint32_t stickQuadrant(const LadderInput& input) {
    return ((std::uint32_t(std::uint16_t(input.stickHeading)) - std::uint32_t(std::int32_t(input.cameraYaw * kLadderHeadingScale)) + 0x2000u) & 0xffffu) >> 14;
}

// 0x1e43c0: the stick picks the state (quadrant 0 up, 2 down, the blocked idle stays blocked going up); no stick goes idle. Returns the
// step of the state's update: the root motion of the clip, or 0.05 x sign x 30 x dt for a clip without root track (the ladder clips),
// times ctrl+0x29c + 1 (0 in m13-2-ladder).
float ladderInputStep(LadderClimber& climber, const LadderInput& input, float sign) {
    if (!input.stickActive) {
        climber.phase = climber.phase == LadderPhase::Blocked ? LadderPhase::Blocked : LadderPhase::Idle;
        return 0.0f;
    }
    const std::uint32_t quadrant = stickQuadrant(input);
    if (quadrant == 0 && climber.phase != LadderPhase::Blocked) climber.phase = LadderPhase::Up;
    else if (quadrant == 2) climber.phase = LadderPhase::Down;
    return kLadderClimbStep * sign * 30.0f * input.dt;
}

int phaseAnimation(LadderPhase phase) {
    switch (phase) {
    case LadderPhase::Up: return kAnimationLadderUp;
    case LadderPhase::Down: return kAnimationLadderDown;
    case LadderPhase::Clamber: return kAnimationLadderDismount;
    default: return kAnimationLadderIdle;
    }
}

}

std::vector<LadderObject> loadLadders(const assets::Level& level, const assets::ClassLookup& classOf) {
    std::vector<LadderObject> ladders;
    for (const assets::LevelObject& object : level.objects) {
        const assets::Level* cls = classOf(object.className);
        if (!cls || assets::objectKind(object, *cls) != kLadderObjectKind) continue;
        const std::optional<assets::Placement> placed = assets::objectStart(level, object.instance);
        if (!placed) continue;
        LadderObject ladder;
        ladder.instance = lower(object.instance);
        std::copy_n(placed->position, 3, ladder.position);
        const float yaw = placed->orientation[1] * 3.14159265358979f / 180.0f;
        const float s = gameSin(yaw), c = gameCos(yaw);
        ladder.right[0] = c;
        ladder.right[2] = -s;
        ladder.forward[0] = s;
        ladder.forward[2] = c;
        ladder.heading = headingOf(ladder.forward[0], ladder.forward[2]);
        ladder.snapOffset = floatParam(object, *cls, "ATTR_CharacterSnapToOffset", kLadderSnapOffsetDefault);
        ladder.canDismount = (intParam(object, *cls, "CanDismount", 1) & 1) != 0;
        ladder.furled = (intParam(object, *cls, "StartFurled", 0) & 1) != 0;
        ladder.faceTowards = intParam(object, *cls, "FaceTowards", 0) & 7;
        ladder.useStartActive = (intParam(object, *cls, "UseStartActive", 1) & 1) != 0;
        for (const assets::LevelBound& bound : cls->bounds) {
            const std::string name = lower(bound.name);
            if (name == "collisionbound") {
                std::copy_n(bound.centre, 3, ladder.collisionCentre);
                std::copy_n(bound.half, 3, ladder.collisionHalf);
            } else if (name == "usebound") {
                ladder.hasUseBound = true;
                std::copy_n(bound.centre, 3, ladder.useCentre);
                std::copy_n(bound.half, 3, ladder.useHalf);
            }
        }
        ladders.push_back(ladder);
    }
    return ladders;
}

bool ladderZoneHolds(const LadderObject& ladder, const float player[3], std::int16_t facing) {
    if (!ladder.hasUseBound || !ladder.useStartActive) return false;
    if (ladder.faceTowards != 0) {
        const Vec toLadder = {ladder.position[0] - player[0], ladder.position[1] - player[1], ladder.position[2] - player[2]};
        const float size = std::sqrt(toLadder[0] * toLadder[0] + toLadder[1] * toLadder[1] + toLadder[2] * toLadder[2]);
        const Vec direction = size > 1.1920929e-7f ? Vec{toLadder[0] / size, toLadder[1] / size, toLadder[2] / size} : toLadder;
        const float heading = headingRadians(facing);
        if (!faceTestPasses(ladder.faceTowards, {std::sin(heading), 0.0f, std::cos(heading)}, ladder.forward, direction)) return false;
    }
    const Vec local = toLocal(ladder, player);
    for (int a = 0; a < 3; ++a)
        if (!(std::fabs(local[std::size_t(a)] - ladder.useCentre[a]) < ladder.useHalf[a])) return false;
    return true;
}

bool ladderAccepts(const LadderObject& ladder, const float player[3], float clamberRise) {
    if (ladder.furled) return false;
    return !(ladder.top() + ladder.position[1] - clamberRise < player[1]);
}

int ladderWalkInTest(const std::vector<LadderObject>& ladders, const float player[3], std::int16_t facing, float clamberRise,
                     const std::function<bool(const LadderObject&)>& disabled) {
    for (std::size_t i = 0; i < ladders.size(); ++i) {
        const LadderObject& ladder = ladders[i];
        if (!playerBoxTouches(ladder.position, ladder.right, ladder.up, ladder.forward, ladder.collisionCentre, ladder.collisionHalf, player, facing)) continue;
        if (disabled && disabled(ladder)) continue;
        if (ladderZoneHolds(ladder, player, facing) && ladderAccepts(ladder, player, clamberRise)) return int(i);
    }
    return -1;
}

int ladderUseTest(const std::vector<LadderObject>& ladders, const float player[3], std::int16_t facing, float clamberRise,
                  const std::function<bool(const LadderObject&)>& disabled) {
    for (std::size_t i = 0; i < ladders.size(); ++i) {
        if (disabled && disabled(ladders[i])) continue;
        if (ladderZoneHolds(ladders[i], player, facing) && ladderAccepts(ladders[i], player, clamberRise)) return int(i);
    }
    return -1;
}

LadderStep ladderMount(LadderClimber& climber, const std::vector<LadderObject>& ladders, int index, const LadderInput& input, bool byUse) {
    const LadderObject& ladder = ladders[std::size_t(index)];
    climber = LadderClimber();
    climber.phase = LadderPhase::Mount;
    climber.ladder = index;
    std::copy_n(input.position, 3, climber.start);
    climber.target[0] = ladder.position[0] + ladder.snapOffset * ladder.forward[0];
    climber.target[1] = input.position[1];
    climber.target[2] = ladder.position[2] + ladder.snapOffset * ladder.forward[2];
    const std::int16_t facing = byUse ? input.facing : std::int16_t(std::uint16_t(std::int32_t(ladder.heading) + 0x8000));
    LadderInput mounting = input;
    mounting.facing = facing;
    return ladderUpdate(climber, ladders, mounting);
}

LadderStep ladderUpdate(LadderClimber& climber, const std::vector<LadderObject>& ladders, const LadderInput& input) {
    if (!climber.active()) return LadderStep();
    const LadderObject& ladder = ladders[std::size_t(climber.ladder)];
    std::int16_t facing = input.facing;
    float position[3] = {input.position[0], input.position[1], input.position[2]};

    if (climber.phase == LadderPhase::Mount) {
        climber.height += input.dt * 30.0f * kLadderMountStep;
        const float t = climber.height;
        for (int a = 0; a < 3; ++a) position[a] = climber.start[a] + (climber.target[a] - climber.start[a]) * t;
        if (t < 1.0f) return riding(position, facing, kAnimationLadderIdle);
        std::copy_n(climber.target, 3, position);
        climber.phase = LadderPhase::Idle;
        climber.phaseUpdates = 0;
        climber.height = position[1] - ladder.position[1];
        facing = std::int16_t(std::uint16_t(std::int32_t(ladder.heading) + 0x8000));
        return riding(position, facing, kAnimationLadderIdle);
    }

    if (climber.phase == LadderPhase::Clamber) {
        // 0x302dcc / 0x302e94: the first update of state 23 does not move; the update n >= 2 moves by the root motion between frames
        // (n - 2) and (n - 1) of the update, and the clip end (Default Anim Done Event) gives state 1 (m13-2-ladder run lad-up).
        const unsigned update = ++climber.phaseUpdates;
        const float fps = input.clamberFramesPerSecond;
        if (update >= 2 && input.clamber && !input.clamber->translation.empty()) {
            const Vec from = rootAt(*input.clamber, float(update - 2) * input.dt * fps), to = rootAt(*input.clamber, float(update - 1) * input.dt * fps);
            const float heading = headingRadians(facing), s = std::sin(heading), c = std::cos(heading);
            const Vec delta = {to[0] - from[0], to[1] - from[1], to[2] - from[2]};
            position[0] += s * delta[2] + c * delta[0];
            position[1] += delta[1];
            position[2] += c * delta[2] - s * delta[0];
            if (input.floorHeight) {
                if (const std::optional<float> floor = input.floorHeight(position, kClamberSnapReach + std::fabs(delta[1]))) {
                    if (position[1] < *floor) position[1] = *floor;
                    else if (delta[1] < 0.0f && position[1] - *floor <= kClamberSnapReach) position[1] = *floor;
                }
            }
        }
        LadderStep step = riding(position, facing, kAnimationLadderDismount);
        if (float(update - 1) * input.dt * fps >= float(input.clamberFrames) - 1e-3f) {
            step.exit = LadderExit::Ground;
            climber = LadderClimber();
        }
        return step;
    }

    if (input.aPressed) return leave(climber, position, facing, LadderExit::Fall);
    if (input.bPressed) return leave(climber, position, facing, LadderExit::Jump);

    if (climber.phase == LadderPhase::Idle || climber.phase == LadderPhase::Blocked) {
        const LadderPhase before = climber.phase;
        ladderInputStep(climber, input, 0.0f);
        if (climber.phase != before) climber.phaseUpdates = 0;
        return riding(position, facing, kAnimationLadderIdle);
    }

    const bool down = climber.phase == LadderPhase::Down;
    const LadderPhase before = climber.phase;
    const float step = ladderInputStep(climber, input, down ? -1.0f : 1.0f);
    if (climber.phase != before) climber.phaseUpdates = 0;
    else ++climber.phaseUpdates;
    if (!down) {
        // 0x32ca18
        climber.height += step;
        if (ladder.top() <= climber.height + input.clamberRise) {
            if (ladder.canDismount) {
                climber.phase = LadderPhase::Clamber;
                climber.phaseUpdates = 0;
                return riding(position, facing, kAnimationLadderDismount);
            }
            climber.height -= step;
            if (step > 0.0f) {
                climber.phase = LadderPhase::Blocked;
                return riding(position, facing, kAnimationLadderIdle);
            }
        }
        position[1] += step;
        return riding(position, facing, phaseAnimation(climber.phase));
    }
    // 0x32f83c
    if (input.floorBelow && input.floorBelow(position, kRayReach + std::fabs(step))) return leave(climber, position, facing, LadderExit::Fall);
    climber.height += step;
    if (climber.height < -kLadderPlayerHalfHeight) return leave(climber, position, facing, LadderExit::Fall);
    position[1] += step;
    return riding(position, facing, phaseAnimation(climber.phase));
}

}
