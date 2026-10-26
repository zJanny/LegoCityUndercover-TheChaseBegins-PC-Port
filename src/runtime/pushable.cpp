#include "runtime/pushable.hpp"
#include "assets/object_classes.hpp"
#include "assets/object_collision.hpp"
#include "runtime/lever_use.hpp"
#include "runtime/level_start.hpp"
#include "runtime/player_intent.hpp"
#include "runtime/player_locomotion.hpp"
#include "runtime/pole.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <map>

namespace chase::runtime {
namespace {

constexpr float kHeadingScale = 10430.378f;
constexpr const char* kHandleSlotParams[4] = {"ActiveHandles_Front", "ActiveHandles_Back", "ActiveHandles_Right", "ActiveHandles_Left"};

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

int intParam(const assets::LevelObject& object, const assets::Level& cls, const char* name) {
    const assets::LevelParam* param = effectiveParam(object, cls, assets::nameHash(name));
    if (!param) return 0;
    if (param->type == 0) {
        float value;
        std::memcpy(&value, &param->value, 4);
        return int(value);
    }
    return int(param->value);
}

std::string referenceName(const assets::LevelObject& object, const assets::Level& cls, const char* name, const std::map<std::uint32_t, std::string>& names) {
    const assets::LevelParam* param = effectiveParam(object, cls, assets::nameHash(name));
    if (!param) return {};
    const auto found = names.find(assets::objectReferenceHash(*param));
    return found == names.end() ? std::string() : found->second;
}

const float* row(const PushMatrix& m, int r) { return m.data() + 4 * r; }

float dot3(const float* a, const float* b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }

void transformPoint(const float point[3], const PushMatrix& m, float out[3]) {
    for (int a = 0; a < 3; ++a) out[a] = point[0] * m[a] + point[1] * m[4 + a] + point[2] * m[8 + a] + m[12 + a];
}

float distance3(const float a[3], const float b[3]) {
    const float d[3] = {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
    return std::sqrt(dot3(d, d));
}

}

PushMatrix pushHandleLocal(int slot, const float centre[3], const float half[3]) {
    PushMatrix m{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    const auto setRows = [&](const float r0[3], const float r2[3]) {
        std::copy_n(r0, 3, m.begin());
        std::copy_n(r2, 3, m.begin() + 8);
    };
    const float x[3] = {1, 0, 0}, z[3] = {0, 0, 1}, nx[3] = {-1, 0, 0}, nz[3] = {0, 0, -1};
    switch (slot) {
    case 0:
        m[14] = centre[2] + half[2];
        setRows(x, z);
        break;
    case 1:
        m[14] = centre[2] - half[2];
        setRows(nx, nz);
        break;
    case 2:
        m[12] = centre[0] + half[0];
        setRows(nz, x);
        break;
    default:
        m[12] = centre[0] - half[0];
        setRows(z, nx);
        break;
    }
    return m;
}

PushMatrix pushHandleWorld(const PushableObject& object, const PushHandle& handle) { return assets::multiply(handle.local, object.node); }

void PushableSet::load(assets::FuseArchive& archive, const assets::Level& level, const assets::ClassLookup& classOf) {
    objects_.clear();
    std::map<std::uint32_t, std::string> names;
    for (const assets::LevelObject& object : level.objects) names[assets::nameHash(object.instance)] = lower(object.instance);
    for (const assets::LevelObject& object : level.objects) {
        const assets::Level* cls = classOf(object.className);
        if (!cls || assets::objectKind(object, *cls) != kPushableKind) continue;
        const std::optional<assets::Placement> placed = assets::objectStart(level, object.instance);
        if (!placed) continue;
        PushableObject pushable;
        pushable.instance = lower(object.instance);
        pushable.destination = referenceName(object, *cls, "Destination", names);
        pushable.triggerObject = referenceName(object, *cls, "TriggerObject", names);
        pushable.triggerDistance = floatParam(object, *cls, "TriggerDistance", kPushableTriggerDistanceDefault);
        pushable.pushSpeed = floatParam(object, *cls, "PushSpeed", kPushableSpeedDefault);
        pushable.sinkHeight = floatParam(object, *cls, "SinkHeight", kPushableSinkHeightDefault);
        pushable.strong = intParam(object, *cls, "Strong") != 0;
        pushable.autoPush = intParam(object, *cls, "AutoPush") != 0;
        pushable.breakOnFall = intParam(object, *cls, "BreakOnFall") != 0;
        pushable.noGravity = intParam(object, *cls, "NoGravity") != 0;
        pushable.soundPush = std::uint16_t(intParam(object, *cls, "ATTR_SFX_PUSH"));
        pushable.soundLand = std::uint16_t(intParam(object, *cls, "ATTR_SFX_LAND"));
        pushable.soundDestroyed = std::uint16_t(intParam(object, *cls, "ATTR_SFX_DESTROYED"));
        const int moveType = intParam(object, *cls, "MoveType");
        pushable.canPush = moveType == 0 || moveType == 2;
        pushable.canPull = moveType == 1 || moveType == 2;
        pushable.node = assets::placementMatrix(*placed);
        if (!placed->mesh.empty()) assets::modelBounds(archive, assets::meshModelPath(placed->mesh), pushable.boundsCentre, pushable.boundsHalf);
        int slot = 0;
        for (const assets::LevelObject& child : cls->objects) {
            const assets::Level* childClass = classOf(child.className);
            if (!childClass || assets::objectKind(child, *childClass) != kPushHandleKind || slot > 3) continue;
            PushHandle handle;
            handle.instance = pushable.instance + "." + lower(child.instance);
            handle.local = pushHandleLocal(slot, pushable.boundsCentre, pushable.boundsHalf);
            handle.active = intParam(object, *cls, kHandleSlotParams[slot]) != 0;
            for (const assets::LevelBound& bound : childClass->bounds) {
                if (lower(bound.name) != "usebound" || !bound.enabled) continue;
                std::copy_n(bound.centre, 3, handle.useCentre);
                std::copy_n(bound.half, 3, handle.useHalf);
            }
            pushable.handles.push_back(handle);
            ++slot;
        }
        objects_.push_back(std::move(pushable));
    }
}

void PushableSet::push(std::size_t index, const float direction[3]) {
    PushableObject& object = objects_[index];
    std::copy_n(direction, 3, object.direction);
    object.pushed = true;
}

std::vector<PushableEvent> PushableSet::update(const std::function<bool(const std::string&)>& disabled,
                                               const std::function<bool(const std::string&, float[3])>& destination) {
    std::vector<PushableEvent> events;
    for (std::size_t i = 0; i < objects_.size(); ++i) {
        PushableObject& object = objects_[i];
        if (disabled(object.instance)) continue;
        const auto soundOff = [&] {
            if (!object.pushSound) return;
            object.pushSound = false;
            events.push_back({PushableEventKind::SoundStop, i});
        };
        float* position = object.node.data() + 12;
        if (object.state == object.target) {
            if (object.state == 0) {
                if (!object.pushed) {
                    soundOff();
                } else {
                    float move[3];
                    for (int a = 0; a < 3; ++a) move[a] = object.direction[a] * object.pushSpeed;
                    for (int a = 0; a < 3; ++a) position[a] += move[a];
                    events.push_back({PushableEventKind::Moved, i});
                    if (std::sqrt(dot3(move, move)) > object.pushSpeed * 0.25f && !object.pushSound) {
                        object.pushSound = true;
                        events.push_back({PushableEventKind::SoundStart, i});
                    }
                    object.pushed = false;
                }
                float target[3];
                if (!object.destination.empty() && !disabled(object.destination) && destination(object.destination, target) &&
                    distance3(position, target) < object.triggerDistance)
                    object.target = 1;
            } else if (object.state == 1) {
                float target[3];
                if (!object.destination.empty() && destination(object.destination, target)) {
                    position[0] = position[0] * 0.9f + target[0] * 0.1f;
                    position[2] = position[2] * 0.9f + target[2] * 0.1f;
                    position[1] = target[1] - object.boundsHalf[1] * object.sinkHeight * float(object.sinkUpdates) * 2.0f * 0.016666668f;
                }
                ++object.sinkUpdates;
                if (object.sinkUpdates == kPushableSinkUpdates || object.sinkHeight == 0.0f) object.target = 2;
            } else if (object.state == 2) {
                soundOff();
            }
        }
        if (object.target == object.state) continue;
        object.state = object.target;
        if (object.state == 1) events.push_back({PushableEventKind::Arrived, i});
        if (object.state != 2) continue;
        for (PushHandle& handle : object.handles) handle.usable = false;
        events.push_back({PushableEventKind::Done, i});
    }
    return events;
}

bool pushHandleZoneHolds(const PushableObject& object, const PushHandle& handle, const float player[3], std::int16_t facing) {
    if (!handle.active || !handle.usable) return false;
    const PushMatrix world = pushHandleWorld(object, handle);
    const float heading = headingRadians(facing);
    const float forward[3] = {gameSin(heading), 0.0f, gameCos(heading)};
    if (dot3(forward, row(world, 2)) > 0.0f) return false;
    const float offset[3] = {player[0] - world[12], player[1] - world[13], player[2] - world[14]};
    for (int a = 0; a < 3; ++a)
        if (!(std::fabs(dot3(offset, row(world, a)) - handle.useCentre[a]) < handle.useHalf[a])) return false;
    return true;
}

std::optional<std::size_t> pushGrabTest(const PushableObject& object, const float player[3], std::int16_t facing) {
    if (object.handles.empty() || !object.autoPush) return std::nullopt;
    if (!playerBoxTouches(row(object.node, 3), row(object.node, 0), row(object.node, 1), row(object.node, 2), object.boundsCentre, object.boundsHalf,
                          player, facing))
        return std::nullopt;
    std::size_t nearest = 0;
    float best = 100.0f;
    for (std::size_t h = 0; h < object.handles.size(); ++h) {
        const PushMatrix world = pushHandleWorld(object, object.handles[h]);
        const float d = distance3(world.data() + 12, player);
        if (h == 0 || d < best) {
            nearest = h;
            best = d;
        }
    }
    if (!pushHandleZoneHolds(object, object.handles[nearest], player, facing)) return std::nullopt;
    return nearest;
}

void pushGripStart(PushGrip& grip, std::size_t pushable, std::size_t handle, const float position[3]) {
    grip = PushGrip{};
    grip.active = true;
    grip.pushable = pushable;
    grip.handle = handle;
    std::copy_n(position, 3, grip.start);
}

PushGripStep pushGripUpdate(PushGrip& grip, PushableSet& set, const PushGripInput& input) {
    PushGripStep step;
    std::copy_n(input.position, 3, step.position);
    step.facing = input.facing;
    PushableObject& object = set.objects()[grip.pushable];
    const PushHandle& handle = object.handles[grip.handle];
    const PushMatrix world = pushHandleWorld(object, handle);
    const float* handleZ = row(world, 2);
    const std::int16_t targetHeading = std::int16_t(std::int32_t(gameAtan2(-handleZ[0], -handleZ[2]) * kHeadingScale));
    float along = 0.0f;
    if (input.stickActive) {
        const float angle = float(std::uint16_t(input.stickHeading)) * 9.58738e-05f;
        const float stick[3] = {gameSin(angle), 0.0f, gameCos(angle)};
        along = dot3(stick, handleZ);
    }
    bool holds = object.autoPush && !object.canPull ? along < 0.0f : input.actionHeld;
    if (holds) holds = pushHandleZoneHolds(object, handle, input.position, input.facing);
    if (!holds) {
        grip.active = false;
        step.exited = true;
        step.animation = 0;
        return step;
    }
    std::uint16_t state = grip.state;
    if (!input.stickActive) {
        state = kPlayerStateHandleIdle;
    } else if (along > 0.0f && object.canPull) {
        if (grip.blend == 1.0f) set.push(grip.pushable, handleZ);
        state = kPlayerStateHandlePull;
    } else if (along < 0.0f && object.canPush) {
        if (grip.blend == 1.0f) {
            const float direction[3] = {-handleZ[0], -handleZ[1], -handleZ[2]};
            set.push(grip.pushable, direction);
        }
        state = kPlayerStateHandlePush;
    }
    if (state != grip.state) step.enteredState = state;
    grip.state = state;
    float usePoint[3];
    transformPoint(handle.useCentre, world, usePoint);
    step.position[0] = grip.start[0] + (usePoint[0] - grip.start[0]) * grip.blend;
    step.position[2] = grip.start[2] + (usePoint[2] - grip.start[2]) * grip.blend;
    if (grip.blend < 1.0f) grip.blend = std::min(1.0f, grip.blend + input.dt * kPushGripBlendRate);
    step.facing = turnTowards(input.facing, targetHeading, int(kPushGripTurnPerSecond / input.frameRate));
    step.animation = state == kPlayerStateHandlePush ? kAnimationPush : state == kPlayerStateHandlePull ? kAnimationPushPull : kAnimationPushIdle;
    return step;
}

}
