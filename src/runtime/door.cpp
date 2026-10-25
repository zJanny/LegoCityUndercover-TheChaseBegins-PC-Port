#include "runtime/door.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>

namespace chase::runtime {
namespace {
constexpr float kPi = 3.1415927f;
constexpr float kHalfPi = 1.5707964f;
constexpr float kSecondsToFrames = 30.0f;
constexpr float kZoneExtraY = 0.25f;
constexpr float kZoneExtraZ = 4.0f;

enum Phase : std::uint16_t { Closed = 1, Opening = 2, Open = 3, Closing = 4 };

float lerp(float t, float a, float b) { return a + (b - a) * t; }

std::array<float, 16> identity() {
    std::array<float, 16> m{};
    m[0] = m[5] = m[10] = m[15] = 1.0f;
    return m;
}

std::array<float, 16> turnAbout(float angle, const float pivot[3]) {
    const float c = std::cos(angle), s = std::sin(angle);
    std::array<float, 16> m = identity();
    m[0] = c;
    m[2] = -s;
    m[8] = s;
    m[10] = c;
    m[12] = pivot[0] - (pivot[0] * m[0] + pivot[1] * m[4] + pivot[2] * m[8]);
    m[13] = pivot[1] - (pivot[0] * m[1] + pivot[1] * m[5] + pivot[2] * m[9]);
    m[14] = pivot[2] - (pivot[0] * m[2] + pivot[1] * m[6] + pivot[2] * m[10]);
    return m;
}

void startState(DoorState& door, const DoorParams& params, std::uint16_t newState, std::vector<DoorSoundEvent>& sounds) {
    using Action = DoorSoundEvent::Action;
    switch (door.target) {
    case Closed:
        sounds.push_back({Action::Stop, params.sounds.closing});
        sounds.push_back({Action::Play, params.sounds.shut});
        door.openFlag = false;
        break;
    case Opening:
        door.timer = door.state == Closing ? std::uint8_t(params.openFrames - door.timer) : std::uint8_t(0);
        door.bounceLeft = params.bounce;
        door.openFlag = true;
        break;
    case Open:
        door.timer = 0;
        sounds.push_back({Action::Stop, params.sounds.opening});
        sounds.push_back({Action::Play, params.sounds.open});
        break;
    case Closing:
        sounds.push_back({Action::Play, params.sounds.closing});
        door.timer = door.state == Opening ? std::uint8_t(params.openFrames - door.timer) : std::uint8_t(0);
        door.bounceLeft = params.bounce;
        break;
    default: break;
    }
    door.state = newState;
}

void movingLogic(DoorState& door, const DoorParams& params, bool playerInZone) {
    if (!door.partial) {
        if (door.bounceTimer == 0) {
            const std::uint8_t bounceBefore = door.bounceLeft;
            door.timer = std::uint8_t(door.timer + 1);
            if (bounceBefore != 0 && int(door.timer) >= int(params.openFrames) - 1) {
                door.bounceLeft = std::uint8_t(bounceBefore >> 1);
                door.bounceTimer = std::uint8_t((unsigned(bounceBefore) * params.openFrames) >> 9);
            }
        } else {
            door.bounceTimer = std::uint8_t(door.bounceTimer - 1);
            door.timer = std::uint8_t(door.timer - 1);
        }
    }
    if (door.state == Opening) {
        if (params.openFrames < door.timer) {
            door.target = Open;
            door.timer = params.openFrames;
            door.partial = false;
        }
    } else {
        if (params.openFrames < door.timer) {
            door.target = Closed;
            door.timer = params.openFrames;
            door.partial = false;
        }
        if (params.autoOpen && playerInZone) door.target = Opening;
    }
}
}

std::uint8_t doorFrames(float seconds) { return std::uint8_t(int(seconds * kSecondsToFrames)); }

float doorEase(float t, int shape) {
    switch (shape) {
    case 1: return (std::cos((t - 1.0f) * -kPi) + 1.0f) * 0.5f;
    case 2: return 1.0f - std::cos(t * kHalfPi);
    case 3: return std::sin(t * kHalfPi);
    case 5: return t * t * (3.0f - 2.0f * t);
    default: return t;
    }
}

bool doorZoneContains(const float m[16], const float player[3], const DoorParams& params) {
    const float dx = player[0] - m[12], dy = player[1] - m[13], dz = player[2] - m[14];
    const float local[3] = {dx * m[0] + dy * m[1] + dz * m[2], dx * m[4] + dy * m[5] + dz * m[6], dx * m[8] + dy * m[9] + dz * m[10]};
    const float half[3] = {params.zoneHalf[0], params.zoneHalf[1] + kZoneExtraY, params.zoneHalf[2] + kZoneExtraZ};
    for (int a = 0; a < 3; ++a) {
        const float d = local[a] - params.zoneCentre[a];
        if (d > half[a] || d < -half[a]) return false;
    }
    return true;
}

void doorUpdate(DoorState& door, const DoorParams& params, bool playerInZone, std::vector<DoorSoundEvent>& sounds) {
    if (door.state == door.target) {
        switch (door.state) {
        case Closed:
            if (params.autoOpen && playerInZone) door.target = Opening;
            break;
        case Opening:
        case Closing:
            movingLogic(door, params, playerInZone);
            break;
        case Open:
            if (params.autoCloseFrames != 0) {
                if (playerInZone) door.timer = 0;
                door.timer = std::uint8_t(door.timer + 1);
                if (params.autoCloseFrames <= door.timer) door.target = Closing;
            }
            break;
        default: break;
        }
    }
    if (door.state != door.target) startState(door, params, door.target, sounds);
    if (door.state == Opening && door.timer == params.soundOpenDelay) sounds.push_back({DoorSoundEvent::Action::Play, params.sounds.opening});
}

std::optional<DoorMessage> doorMessageFor(std::uint8_t message) {
    switch (message) {
    case 0xff: return DoorMessage::Open;
    case 0xfe: return DoorMessage::Close;
    case 8: return DoorMessage::Reset;
    case 0x36: return DoorMessage::OpenAtOnce;
    default: return std::nullopt;
    }
}

void doorMessage(DoorState& door, const DoorParams& params, DoorMessage message, float value) {
    switch (message) {
    case DoorMessage::Reset:
        door.state = door.target = Closed;
        break;
    case DoorMessage::Open:
        if (door.state == Closed || door.state == Closing || door.state == 0) door.target = Opening;
        else if (door.state == Open) door.timer = 0;
        break;
    case DoorMessage::Close:
        if (door.state == Open || door.state == Opening) door.target = Closing;
        break;
    case DoorMessage::OpenAtOnce:
        door.openFlag = true;
        door.timer = params.openFrames;
        door.state = door.target = Open;
        break;
    case DoorMessage::SetOpening:
        if (value == 0.0f && door.partial) {
            door.target = Closing;
            door.partial = false;
            door.timer = std::uint8_t(int(float(params.openFrames) * door.partialValue));
        } else if (value == 1.0f && door.partial) {
            door.target = Opening;
            door.partial = false;
            door.timer = std::uint8_t(int(float(params.openFrames) * door.partialValue));
        } else {
            door.partial = true;
            door.target = Opening;
            door.timer = std::uint8_t(int(float(params.openFrames) * value));
            door.partialValue = value;
        }
        break;
    }
}

DoorPose doorPose(const DoorState& door, const DoorParams& params, const float pivot[2][3]) {
    DoorPose pose;
    pose.node[0] = identity();
    pose.node[1] = identity();
    if (door.state == Closed) return pose;
    const float t = door.state == Open ? 1.0f : float(door.timer) / float(params.openFrames);
    const float f = doorEase(t, params.lerpShape);
    const bool opening = door.state == Opening || door.state == Open;
    const bool hinged = params.doorType == 0 || params.doorType == 1;
    const bool sliding = (params.doorType >= 5 && params.doorType <= 8) || params.doorType == 0x11;
    if (hinged) {
        const float angle = opening ? lerp(f, 0.0f, params.restAngle) : lerp(f, params.restAngle, 0.0f);
        pose.node[0] = turnAbout(angle, pivot[0]);
        if (params.twoLeaves) pose.node[1] = turnAbout(-angle, pivot[1]);
    } else if (sliding) {
        for (int a = 0; a < 3; ++a) {
            const float v = opening ? lerp(f, 0.0f, params.slide[a]) : lerp(f, params.slide[a], 0.0f);
            pose.node[0][12 + a] = v;
            pose.node[1][12 + a] = -v;
        }
        if (!params.twoLeaves) pose.node[1] = identity();
    }
    return pose;
}

namespace {
using assets::Level;
using assets::LevelObject;
using assets::LevelParam;

const LevelParam* findParam(const std::vector<LevelParam>& params, std::uint32_t hash) {
    for (const LevelParam& param : params)
        if (param.hash == hash) return &param;
    return nullptr;
}

const LevelParam* effectiveParam(const LevelObject& object, const Level& cls, const char* name) {
    const std::uint32_t hash = assets::nameHash(name);
    const LevelParam* classParam = findParam(cls.defaults, hash);
    if (!classParam) classParam = findParam(cls.classParams, hash);
    if (!classParam) return nullptr;
    if (classParam->flags & 2)
        if (const LevelParam* instance = findParam(object.params, hash)) return instance;
    return classParam;
}

float floatParam(const LevelObject& object, const Level& cls, const char* name, float fallback) {
    const LevelParam* param = effectiveParam(object, cls, name);
    if (!param) return fallback;
    float value;
    std::memcpy(&value, &param->value, sizeof value);
    return value;
}

std::uint32_t integerParam(const LevelObject& object, const Level& cls, const char* name) {
    const LevelParam* param = effectiveParam(object, cls, name);
    if (!param) return 0;
    if (param->type == 0) {
        float value;
        std::memcpy(&value, &param->value, sizeof value);
        return std::uint32_t(int(value));
    }
    return param->value;
}
}

DoorParams doorParams(const LevelObject& object, const Level& cls, const assets::Model& model, const float zoneCentre[3], const float zoneHalf[3]) {
    DoorParams params;
    params.openFrames = doorFrames(floatParam(object, cls, "openclosetime", 0.0f));
    params.autoCloseFrames = doorFrames(floatParam(object, cls, "autoclosetime", 0.0f));
    params.soundOpenDelay = std::uint8_t(integerParam(object, cls, "DoorSFXOpenDelay"));
    const int bounce = int(floatParam(object, cls, "Bounce", 0.0f) * 255.0f);
    params.bounce = bounce > 0xfe ? 0xff : std::uint8_t(bounce);
    params.autoOpen = integerParam(object, cls, "AutoOpen") != 0;
    params.twoLeaves = model.nodeNames.size() > 1;
    params.doorType = int(integerParam(object, cls, "DoorType")) & 0x1f;
    params.lerpShape = int(integerParam(object, cls, "LerpShape"));
    params.sounds = {std::uint16_t(integerParam(object, cls, "ATTR_SFXOpening")), std::uint16_t(integerParam(object, cls, "ATTR_SFXClosing")),
                     std::uint16_t(integerParam(object, cls, "ATTR_SFXShut")), std::uint16_t(integerParam(object, cls, "ATTR_SFXOpen"))};
    const float angle = floatParam(object, cls, "OpenAngle", 90.0f) * 3.1415927f * 0.0055555557f;
    switch (params.doorType) {
    case 0: case 2: case 4: case 0x12: params.restAngle = -angle; break;
    case 1: case 3: case 0x13: params.restAngle = angle; break;
    default: break;
    }
    if (!model.meshes.empty() && !model.nodeMesh.empty() && model.nodeMesh[0] >= 0) {
        const assets::ModelMesh& mesh = model.meshes[std::size_t(model.nodeMesh[0])];
        switch (params.doorType) {
        case 5: case 9: case 0xd: params.slide[0] = mesh.half[0] * -2.0f; break;
        case 6: case 10: case 0xe: params.slide[0] = mesh.half[0] * 2.0f; break;
        case 7: case 0xb: case 0xf: params.slide[1] = mesh.half[1] * 2.0f; break;
        case 8: case 0xc: case 0x10: params.slide[1] = mesh.half[1] * -2.0f; break;
        case 0x11: params.slide[2] = mesh.half[2] * 2.0f; break;
        default: break;
        }
    }
    std::memcpy(params.zoneCentre, zoneCentre, sizeof params.zoneCentre);
    std::memcpy(params.zoneHalf, zoneHalf, sizeof params.zoneHalf);
    return params;
}

}
