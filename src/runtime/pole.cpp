#include "runtime/pole.hpp"
#include "runtime/level_start.hpp"
#include "runtime/player_intent.hpp"
#include "runtime/player_locomotion.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>

namespace chase::runtime {
namespace {

using Vec = std::array<float, 3>;

constexpr float kPi = 3.14159265358979f;
constexpr float kDismountSnapReach = 0.1f;  // TRAV-S8: floor snap distance of the dismount while it descends (observed 0.0524 at update 108)

Vec vec(const float v[3]) { return {v[0], v[1], v[2]}; }

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

bool boolParam(const assets::LevelObject& object, const assets::Level& cls, const char* name) {
    const assets::LevelParam* param = effectiveParam(object, cls, assets::nameHash(name));
    if (!param) return false;
    if (param->type == 0) {
        float value;
        std::memcpy(&value, &param->value, 4);
        return int(value) != 0;
    }
    return (param->value & 1) != 0;
}

std::int16_t headingOf(float x, float z) { return std::int16_t(std::uint16_t(std::int32_t(gameAtan2(x, z) * kPoleHeadingScale))); }

Vec toLocal(const PoleObject& pole, const Vec& point) {
    const Vec d = {point[0] - pole.position[0], point[1] - pole.position[1], point[2] - pole.position[2]};
    auto dot = [](const float* a, const Vec& b) {
        float sum = a[0] * b[0];
        sum += a[1] * b[1];
        sum += a[2] * b[2];
        return sum;
    };
    return {dot(pole.right, d), dot(pole.up, d), dot(pole.forward, d)};
}

// 0x254250: a point of the pole's frame in world space.
Vec toWorld(const PoleObject& pole, const Vec& local) {
    Vec out;
    for (int a = 0; a < 3; ++a) out[a] = pole.position[a] + local[0] * pole.right[a] + local[1] * pole.up[a] + local[2] * pole.forward[a];
    return out;
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

Vec rootAt(const assets::RootMotion& motion, float frame, unsigned loopFrames) {
    const auto& t = motion.translation;
    if (t.empty()) return {0, 0, 0};
    const float last = float(t.size() - 1);
    Vec offset = {0, 0, 0};
    if (loopFrames > 0 && frame >= float(loopFrames)) {
        const float turns = std::floor(frame / float(loopFrames));
        for (int a = 0; a < 3; ++a) offset[a] = turns * (t[loopFrames][a] - t[0][a]);
        frame -= turns * float(loopFrames);
    }
    frame = std::clamp(frame, 0.0f, last);
    const std::size_t index = std::size_t(frame);
    Vec out;
    if (index + 1 >= t.size()) {
        out = {t.back()[0], t.back()[1], t.back()[2]};
    } else {
        const float f = frame - float(index);
        for (int a = 0; a < 3; ++a) out[a] = t[index][a] + (t[index + 1][a] - t[index][a]) * f;
    }
    for (int a = 0; a < 3; ++a) out[a] += offset[a];
    return out;
}

// The step of 0x1d579c / 0x283f18 for the clip playing in the state: root motion of the update, else the global.
float climbStep(const PoleInput& input, const assets::RootMotion* motion, unsigned frames, float fps, unsigned update, bool down) {
    if (motion && !motion->translation.empty()) return poleClimbStep(*motion, frames, update, input.dt, fps);
    return down ? -kPoleClimbStepDefault : kPoleClimbStepDefault;
}

PoleStep riding(const float position[3], std::int16_t facing, int animation) {
    PoleStep step;
    step.riding = true;
    std::copy_n(position, 3, step.position);
    step.facing = facing;
    step.animation = animation;
    return step;
}

// TRAV-S6: the standing point after leaving: pole axis + forward x (CollisionBound half depth + player box front + margin).
void exitPlacement(const PoleObject& pole, float height, float out[3]) {
    const float distance = pole.collisionHalf[2] + 0.6f + kPoleExitMargin;
    out[0] = pole.position[0] + pole.forward[0] * distance;
    out[1] = height;
    out[2] = pole.position[2] + pole.forward[2] * distance;
}

PoleStep leave(PoleClimber& climber, const PoleObject& pole, const PoleInput& input, PoleExit exit, std::int16_t facing) {
    PoleStep step;
    exitPlacement(pole, input.position[1], step.position);
    step.facing = facing;
    step.exit = exit;
    step.animation = exit == PoleExit::Jump ? 0 : kAnimationPoleIdle;
    climber = PoleClimber();
    return step;
}

// 0x206510 / 0x2011b0 gate and 0x28d908 end: the pole idle state at the snap point.
void settle(PoleClimber& climber, const PoleObject& pole, float y) {
    climber.phase = PolePhase::Idle;
    climber.height = y - pole.position[1];
    climber.phaseUpdates = 0;
}

bool bottomReached(const PoleClimber& climber, const PoleObject& pole) { return climber.height < pole.bottom(); }

}

std::vector<PoleObject> loadPoles(const assets::Level& level, const assets::ClassLookup& classOf) {
    std::vector<PoleObject> poles;
    const std::uint32_t typeHash = assets::nameHash("ObjectType");
    for (const assets::LevelObject& object : level.objects) {
        const assets::Level* cls = classOf(object.className);
        if (!cls) continue;
        int type = -1;
        for (const assets::LevelParam& param : cls->classParams)
            if (param.hash == typeHash) type = int(param.value);
        if (type != kPoleObjectType) continue;
        const std::optional<assets::Placement> placed = assets::objectStart(level, object.instance);
        if (!placed) continue;
        PoleObject pole;
        pole.instance = lower(object.instance);
        std::copy_n(placed->position, 3, pole.position);
        const float yaw = placed->orientation[1] * kPi / 180.0f;
        const float s = gameSin(yaw), c = gameCos(yaw);
        pole.right[0] = c;
        pole.right[2] = -s;
        pole.forward[0] = s;
        pole.forward[2] = c;
        pole.heading = headingOf(pole.forward[0], pole.forward[2]);
        pole.snapOffset = floatParam(object, *cls, "ATTR_CharacterSnapToOffset", 0.0f);
        pole.canDismount = boolParam(object, *cls, "CanDismount");
        for (const assets::LevelBound& bound : cls->bounds) {
            const std::string name = lower(bound.name);
            if (name == "collisionbound") {
                std::copy_n(bound.centre, 3, pole.collisionCentre);
                std::copy_n(bound.half, 3, pole.collisionHalf);
            } else if (name == "usebound") {
                std::copy_n(bound.centre, 3, pole.useCentre);
                std::copy_n(bound.half, 3, pole.useHalf);
            }
        }
        poles.push_back(pole);
    }
    return poles;
}

bool poleZoneHolds(const PoleObject& pole, const float player[3], std::int16_t facing) {
    const Vec p = vec(player);
    const float heading = headingRadians(facing);
    const Vec toPole = {pole.position[0] - p[0], pole.position[1] - p[1], pole.position[2] - p[2]};
    const float size = std::sqrt(toPole[0] * toPole[0] + toPole[1] * toPole[1] + toPole[2] * toPole[2]);
    if (size > 1.1920929e-7f) {
        const float inverse = 1.0f / size;
        const float along = std::sin(heading) * toPole[0] * inverse + std::cos(heading) * toPole[2] * inverse;
        if (along < 0.0f) return false;
    }
    const Vec local = toLocal(pole, p);
    for (int a = 0; a < 3; ++a)
        if (!(std::fabs(local[a] - pole.useCentre[a]) < pole.useHalf[a])) return false;
    return true;
}

bool poleTouched(const PoleObject& pole, const float player[3], std::int16_t facing) {
    const float heading = headingRadians(facing);
    const float ps = std::sin(heading), pc = std::cos(heading);
    const float boxCentre[3] = {0.0f, 1.15f, 0.08f}, boxHalf[3] = {0.6f, 0.85f, 0.52f};
    const Vec playerRight = {pc, 0, -ps}, playerForward = {ps, 0, pc};
    Vec centreA, centreB;
    for (int a = 0; a < 3; ++a) {
        centreA[a] = player[a] + boxCentre[0] * playerRight[a] + boxCentre[2] * playerForward[a];
        centreB[a] = pole.position[a] + pole.collisionCentre[0] * pole.right[a] + pole.collisionCentre[1] * pole.up[a] +
                     pole.collisionCentre[2] * pole.forward[a];
    }
    centreA[1] = player[1] + boxCentre[1];
    if (std::fabs(centreA[1] - centreB[1]) > boxHalf[1] + pole.collisionHalf[1]) return false;
    const Vec poleRight = {pole.right[0], 0, pole.right[2]}, poleForward = {pole.forward[0], 0, pole.forward[2]};
    const Vec axes[4] = {playerRight, playerForward, poleRight, poleForward};
    const float d[2] = {centreB[0] - centreA[0], centreB[2] - centreA[2]};
    for (const Vec& axis : axes) {
        const float distance = std::fabs(d[0] * axis[0] + d[1] * axis[2]);
        auto reach = [&](const Vec& right, const Vec& forward, float halfRight, float halfForward) {
            return std::fabs(right[0] * axis[0] + right[2] * axis[2]) * halfRight + std::fabs(forward[0] * axis[0] + forward[2] * axis[2]) * halfForward;
        };
        const float sum = reach(playerRight, playerForward, boxHalf[0], boxHalf[2]) +
                          reach(poleRight, poleForward, pole.collisionHalf[0], pole.collisionHalf[2]);
        if (distance > sum) return false;
    }
    return true;
}

int poleWalkInTest(const std::vector<PoleObject>& poles, const float player[3], std::int16_t facing, bool stickActive, std::int16_t stickHeading,
                   const std::function<bool(const PoleObject&)>& disabled) {
    int touched = -1;
    for (std::size_t i = 0; i < poles.size(); ++i)
        if (poleTouched(poles[i], player, facing)) touched = int(i);
    if (touched < 0) return -1;
    const PoleObject& pole = poles[std::size_t(touched)];
    if (disabled && disabled(pole)) return -1;
    if (stickActive) {
        const std::int32_t toPole = std::int32_t(gameAtan2(pole.position[0] - player[0], pole.position[2] - player[2]) * kPoleHeadingScale) & 0xffff;
        std::int32_t d = std::int32_t(std::uint16_t(stickHeading)) - toPole;
        if (d < -0x8000) d += 0x10000;
        if (0x8000 < d) d -= 0x10000;
        if (d < 0) d = -d;
        if (d > kPoleStickCone) return -1;
    }
    return poleZoneHolds(pole, player, facing) ? touched : -1;
}

int poleUseTest(const std::vector<PoleObject>& poles, const float player[3], std::int16_t facing,
                const std::function<bool(const PoleObject&)>& disabled) {
    for (std::size_t i = 0; i < poles.size(); ++i) {
        if (disabled && disabled(poles[i])) continue;
        if (poleZoneHolds(poles[i], player, facing)) return int(i);
    }
    return -1;
}

float poleClimbStep(const assets::RootMotion& motion, unsigned frames, unsigned update, float dt, float framesPerSecond) {
    const float from = float(update) * dt * framesPerSecond;
    const float to = float(update + 1) * dt * framesPerSecond;
    return rootAt(motion, to, frames)[1] - rootAt(motion, from, frames)[1];
}

PoleStep poleMount(PoleClimber& climber, const std::vector<PoleObject>& poles, int index, const PoleInput& input, bool byUse) {
    const PoleObject& pole = poles[std::size_t(index)];
    climber = PoleClimber();
    climber.phase = PolePhase::Mount;
    climber.pole = index;
    std::copy_n(input.position, 3, climber.start);
    climber.target[0] = pole.position[0] + pole.snapOffset * pole.forward[0];
    climber.target[1] = input.position[1];
    climber.target[2] = pole.position[2] + pole.snapOffset * pole.forward[2];
    // 0x191ac0: ctrl+0xa = atan2(pole forward) - 0x8000 (facing the pole); 0x2011b0 then overwrites it with the direction player -> pole.
    climber.targetHeading = std::int16_t(std::uint16_t(std::int32_t(pole.heading) - 0x8000));
    std::int16_t facing = input.facing;
    if (byUse) climber.targetHeading = headingOf(pole.position[0] - input.position[0], pole.position[2] - input.position[2]);
    else facing = climber.targetHeading;  // 0x206510: ctrl+8 = ctrl+0xa
    return riding(input.position, facing, kAnimationPoleIdle);
}

PoleStep poleUpdate(PoleClimber& climber, const std::vector<PoleObject>& poles, const PoleInput& input) {
    if (!climber.active()) return PoleStep();
    const PoleObject& pole = poles[std::size_t(climber.pole)];
    std::int16_t facing = input.facing;
    float position[3] = {input.position[0], input.position[1], input.position[2]};

    if (climber.phase == PolePhase::Mount) {
        // 0x28d908
        const Vec point = toWorld(pole, {pole.collisionCentre[0], pole.collisionCentre[1], pole.collisionCentre[2] + kPoleMountPointDepth});
        climber.targetHeading = headingOf(point[0] - position[0], point[2] - position[2]);
        facing = turnTowards(facing, climber.targetHeading, kPoleTurnStep);
        climber.height = climber.height + input.dt * kPoleMountRate;
        if (climber.height >= 1.0f) climber.height = 1.0f;
        for (int a = 0; a < 3; ++a) position[a] = climber.start[a] + (climber.target[a] - climber.start[a]) * climber.height;
        if (climber.height < 1.0f) return riding(position, facing, kAnimationPoleIdle);
        position[0] = climber.target[0];
        position[1] = climber.target[1];
        position[2] = climber.target[2];
        settle(climber, pole, position[1]);
        facing = std::int16_t(std::uint16_t(std::int32_t(pole.heading) + 0x8000));
        return riding(position, facing, kAnimationPoleIdle);
    }

    if (climber.phase == PolePhase::Idle) {
        // 0x283f18
        if (input.stickActive) {
            const std::uint32_t quadrant =
                ((std::uint32_t(std::uint16_t(input.stickHeading)) - std::uint32_t(std::int32_t(input.cameraYaw * kPoleHeadingScale)) + 0x2000u) & 0xffffu) >> 14;
            if (quadrant == 0) {
                // The step of an idle update: the idle clip has a root track of zeros, so its step is 0 (TRAV-S9: the return value of 0x2327b4 for it is not read).
                if (climber.height + kPoleReach + 0.0f < pole.top()) {
                    climber.phase = PolePhase::Up;
                    climber.phaseUpdates = 0;
                }
            } else if (quadrant == 2) {
                climber.phase = PolePhase::Down;
                climber.phaseUpdates = 0;
            }
        }
        if (input.aPressed) return leave(climber, pole, input, PoleExit::Fall, facing);
        if (input.bPressed) return leave(climber, pole, input, PoleExit::Jump, facing);
        const int clip = climber.phase == PolePhase::Up ? kAnimationPoleUp : climber.phase == PolePhase::Down ? kAnimationPoleDown : kAnimationPoleIdle;
        return riding(position, facing, clip);
    }

    if (climber.phase == PolePhase::Dismount) {
        // 0x2ac32c: the root-motion move 0x215804 (0x20cd30 with the stick cleared), then the end of the clip -> state 1.
        const unsigned update = climber.phaseUpdates;
        const assets::RootMotion* motion = input.clips.dismount;
        if (motion && !motion->translation.empty()) {
            const float fps = input.clips.dismountFramesPerSecond;
            const Vec from = rootAt(*motion, float(update) * input.dt * fps, 0), to = rootAt(*motion, float(update + 1) * input.dt * fps, 0);
            const float heading = headingRadians(facing), s = std::sin(heading), c = std::cos(heading);
            const Vec delta = {to[0] - from[0], to[1] - from[1], to[2] - from[2]};
            position[0] += s * delta[2] + c * delta[0];
            position[1] += delta[1];
            position[2] += c * delta[2] - s * delta[0];
            if (input.floorHeight) {
                if (const std::optional<float> floor = input.floorHeight(position, kDismountSnapReach + std::fabs(delta[1]))) {
                    if (position[1] < *floor) position[1] = *floor;
                    else if (delta[1] < 0.0f && position[1] - *floor <= kDismountSnapReach) position[1] = *floor;
                }
            }
        }
        ++climber.phaseUpdates;
        PoleStep step = riding(position, facing, kAnimationPoleDismount);
        if (float(climber.phaseUpdates) * input.dt * input.clips.dismountFramesPerSecond >= float(input.clips.dismountFrames) - 1e-3f) {
            step.exit = PoleExit::Ground;
            climber = PoleClimber();
        }
        return step;
    }

    // POLE CLIMB UP / DOWN: 0x1d579c
    const bool down = climber.phase == PolePhase::Down;
    if (!input.stickActive) {
        climber.phase = PolePhase::Idle;
        return riding(position, facing, kAnimationPoleIdle);
    }
    const float step = climbStep(input, down ? input.clips.down : input.clips.up, down ? input.clips.downFrames : input.clips.upFrames,
                                 down ? input.clips.downFramesPerSecond : input.clips.upFramesPerSecond, climber.phaseUpdates, down);
    climber.height += step;
    if (bottomReached(climber, pole)) return leave(climber, pole, input, PoleExit::Fall, facing);
    if (pole.top() <= climber.height + kPoleReach) {
        if (pole.canDismount) {
            climber.phase = PolePhase::Dismount;
            climber.phaseUpdates = 0;
            return riding(position, facing, kAnimationPoleDismount);
        }
        climber.height -= step;
        if (step > 0.0f) {
            climber.phase = PolePhase::Idle;
            return riding(position, facing, kAnimationPoleIdle);
        }
    }
    if (down && input.floorBelow && input.floorBelow(position, std::fabs(step)))
        return leave(climber, pole, input, PoleExit::Fall, std::int16_t(std::uint16_t(std::int32_t(facing) - 0x3000)));
    position[1] += step;
    ++climber.phaseUpdates;
    PoleStep result = riding(position, facing, down ? kAnimationPoleDown : kAnimationPoleUp);
    if (!input.aPressed || climber.height <= kPoleFallHeight) {
        const std::uint32_t quadrant =
            ((std::uint32_t(std::uint16_t(input.stickHeading)) - std::uint32_t(std::int32_t(input.cameraYaw * kPoleHeadingScale)) + 0x2000u) & 0xffffu) >> 14;
        if (quadrant == 0 && climber.phase != PolePhase::Up) {
            climber.phase = PolePhase::Up;
            climber.phaseUpdates = 0;
        } else if (quadrant == 2 && climber.phase != PolePhase::Down) {
            climber.phase = PolePhase::Down;
            climber.phaseUpdates = 0;
        }
    } else {
        PoleStep fall = leave(climber, pole, input, PoleExit::Fall, facing);
        fall.position[1] = position[1];
        return fall;
    }
    if (input.bPressed) {
        PoleStep jump = leave(climber, pole, input, PoleExit::Jump, facing);
        jump.position[1] = position[1];
        return jump;
    }
    return result;
}

}
