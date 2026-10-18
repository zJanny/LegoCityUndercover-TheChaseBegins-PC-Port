#include "runtime/scanner.hpp"
#include "runtime/follow_camera.hpp"
#include "runtime/level_start.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <map>

namespace chase::runtime {
namespace {

constexpr float kDegrees = 0.017453294f;
constexpr float kScreenHalfWidth = 200.0f;
constexpr float kScreenHalfHeight = 120.0f;

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

int objectType(const assets::Level& cls) {
    const std::uint32_t hash = assets::nameHash("ObjectType");
    for (const assets::LevelParam& param : cls.classParams)
        if (param.hash == hash) return int(param.value);
    return -1;
}

std::string referenceName(const assets::LevelObject& object, const assets::Level& cls, const char* name, const std::map<std::uint32_t, std::string>& names) {
    const assets::LevelParam* param = effectiveParam(object, cls, assets::nameHash(name));
    if (!param) return {};
    const auto found = names.find(assets::objectReferenceHash(*param));
    return found == names.end() ? std::string() : found->second;
}

float floatParam(const assets::LevelObject& object, const assets::Level& cls, const char* name, float fallback) {
    const assets::LevelParam* param = effectiveParam(object, cls, assets::nameHash(name));
    if (!param) return fallback;
    if (param->type != 0) return float(param->value);
    float value;
    std::memcpy(&value, &param->value, 4);
    return value;
}

struct Quaternion {
    float x = 0, y = 0, z = 0, w = 1;
};

Quaternion multiply(const Quaternion& a, const Quaternion& b) {
    return {a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y, a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
            a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w, a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z};
}

Quaternion orientation(float yaw, float pitch) {
    const Quaternion turn = {0, std::sin(yaw * 0.5f), 0, std::cos(yaw * 0.5f)};
    const Quaternion tilt = {std::sin(-pitch * 0.5f), 0, 0, std::cos(-pitch * 0.5f)};
    return multiply(turn, tilt);
}

float dot(const Quaternion& a, const Quaternion& b) { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }

Quaternion slerp(const Quaternion& from, Quaternion to, float t) {
    float cosine = dot(from, to);
    if (cosine < 0) {
        to = {-to.x, -to.y, -to.z, -to.w};
        cosine = -cosine;
    }
    float a = 1.0f - t, b = t;
    if (cosine < 0.9999f) {
        const float angle = std::acos(cosine), s = std::sin(angle);
        a = std::sin((1.0f - t) * angle) / s;
        b = std::sin(t * angle) / s;
    }
    Quaternion result = {a * from.x + b * to.x, a * from.y + b * to.y, a * from.z + b * to.z, a * from.w + b * to.w};
    const float length = std::sqrt(dot(result, result));
    return {result.x / length, result.y / length, result.z / length, result.w / length};
}

void forwardOf(const Quaternion& q, float yawPitch[2]) {
    const float fx = 2.0f * (q.x * q.z + q.w * q.y);
    const float fy = 2.0f * (q.y * q.z - q.w * q.x);
    const float fz = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
    yawPitch[1] = std::atan2(fx, fz);
    yawPitch[0] = std::atan2(fy, std::sqrt(fx * fx + fz * fz));
}

bool limitAngle(float low, float high, float& value) {
    if (low != 0.0f && value < low) {
        value = low;
        return true;
    }
    if (high != 0.0f && value > high) {
        value = high;
        return true;
    }
    return false;
}

}

void Scanner::load(assets::FuseArchive& archive, const assets::Level& level, const assets::ClassLookup& classOf) {
    zones_.clear();
    objects_.clear();
    std::map<std::uint32_t, std::string> names;
    for (const assets::LevelObject& object : level.objects) names[assets::nameHash(object.instance)] = lower(object.instance);
    for (const assets::LevelObject& object : level.objects) {
        const assets::Level* cls = classOf(object.className);
        if (!cls) continue;
        const std::optional<assets::Placement> placed = assets::objectStart(level, object.instance);
        if (!placed) continue;
        if (objectType(*cls) == kScannerZoneObjectType) {
            ScannerZone zone;
            zone.instance = lower(object.instance);
            std::copy_n(placed->position, 3, zone.position);
            zone.yawDegrees = placed->orientation[1];
            for (const assets::LevelBound& bound : cls->bounds)
                if (lower(bound.name) == "usebound") {
                    zone.hasUseBound = true;
                    std::copy_n(bound.centre, 3, zone.useCentre);
                    std::copy_n(bound.half, 3, zone.useHalf);
                }
            if (const assets::LevelParam* mode = effectiveParam(object, *cls, assets::nameHash("ScannerMode"))) zone.mode = int(mode->value);
            zone.cameraYOffset = floatParam(object, *cls, "CameraYOffset", 0.0f);
            zone.preEnterTrigger = referenceName(object, *cls, "PreEnterScanTriggerObject", names);
            zone.enterTrigger = referenceName(object, *cls, "EnterScanTriggerObject", names);
            zone.exitTrigger = referenceName(object, *cls, "ExitScanTriggerObject", names);
            zone.completeExitTrigger = referenceName(object, *cls, "CompleteExitTriggerObject", names);
            zone.lockedOnTrigger = referenceName(object, *cls, "LockedOnTriggerObject", names);
            zone.zoomedInTrigger = referenceName(object, *cls, "ZoomedInTriggerObject", names);
            if (zone.mode == kScannerAudioMode) zones_.push_back(std::move(zone));
            continue;
        }
        const assets::LevelParam* audio = effectiveParam(object, *cls, assets::nameHash("ShowOnAudioScan"));
        if (!audio || audio->value == 0) continue;
        ScannableObject scannable;
        scannable.instance = lower(object.instance);
        std::copy_n(placed->position, 3, scannable.point);
        if (!placed->mesh.empty()) {
            float centre[3], half[3];
            if (assets::modelBounds(archive, assets::meshModelPath(placed->mesh), centre, half))
                for (int a = 0; a < 3; ++a) scannable.point[a] += centre[a];
        }
        scannable.triggerScanned = referenceName(object, *cls, "TriggerScanned", names);
        objects_.push_back(std::move(scannable));
    }
}

void Scanner::fire(const std::string& instance, std::vector<ScannerEvent>& events) const {
    if (instance.empty()) return;
    ScannerEvent event;
    event.kind = ScannerEventKind::Trigger;
    event.instance = instance;
    events.push_back(event);
}

void Scanner::sound(SoundRequest::Kind kind, std::uint32_t id, std::vector<ScannerEvent>& events) {
    ScannerEvent event;
    event.kind = ScannerEventKind::Sound;
    event.sound.kind = kind;
    event.sound.id = id;
    event.sound.owner = kScannerSoundOwner;
    event.sound.value = -1.0f;
    events.push_back(event);
    if (kind == SoundRequest::Kind::Play) started_.insert(id);
    else started_.erase(id);
}

bool Scanner::playing(const ScannerInput& input, std::uint32_t id) const {
    return input.soundPlaying ? input.soundPlaying(id) : started_.count(id) != 0;
}

float Scanner::zoomFraction() const { return (zoom_ - settings_.zoomMin) / (settings_.zoomMax - settings_.zoomMin); }

ScannerView Scanner::view() const {
    ScannerView result;
    std::copy_n(base_, 3, result.eye);
    result.yaw = shown_[1];
    result.pitch = shown_[0];
    result.zoomDegrees = zoom_;
    return result;
}

int Scanner::playerClip() const {
    if (phase_ == ScannerPhase::Idle) return 0;
    if (phase_ == ScannerPhase::Exiting && phaseUpdates_ >= kScannerEnterCallbackUpdates) return kAnimationScannerIn;
    return kAnimationScannerOut;
}

void Scanner::startEnter(std::size_t zone, std::vector<ScannerEvent>& events) {
    const ScannerZone& z = zones_[zone];
    zone_ = int(zone);
    phase_ = ScannerPhase::TakingOut;
    phaseUpdates_ = 0;
    canToggle_ = false;
    aligning_ = true;
    alignT_ = 0;
    alignTarget_ = std::int16_t(std::lround(z.yawDegrees * kDegrees * 10430.378f));
    ScannerEvent animation;
    animation.kind = ScannerEventKind::PlayerAnimation;
    animation.animation = kAnimationScannerOut;
    events.push_back(animation);
    fire(z.preEnterTrigger, events);
}

void Scanner::startExit(std::vector<ScannerEvent>& events) {
    phase_ = ScannerPhase::Exiting;
    phaseUpdates_ = 0;
    canToggle_ = false;
    sound(SoundRequest::Kind::Stop, kScannerSoundAmbience, events);
    sound(SoundRequest::Kind::Play, kScannerSoundExit, events);
    started_.erase(kScannerSoundExit);
}

void Scanner::align(const ScannerInput& input, std::vector<ScannerEvent>& events) {
    if (!aligning_) return;
    if (zone_ < 0 || alignT_ >= 1.0f) {
        aligning_ = false;
        return;
    }
    const ScannerZone& z = zones_[std::size_t(zone_)];
    const int difference = std::int16_t(alignTarget_ - facing_);
    facing_ = std::int16_t(facing_ + std::clamp(difference, -kScannerAlignTurnStep, kScannerAlignTurnStep));
    for (int a = 0; a < 3; ++a) player_[a] += (z.position[a] - player_[a]) * alignT_;
    alignT_ += input.dt * kScannerAlignRate;
    ScannerEvent pose;
    pose.kind = ScannerEventKind::PlayerPose;
    events.push_back(pose);
}

std::vector<ScannerEvent> Scanner::update(const ScannerInput& input) {
    std::vector<ScannerEvent> events;
    if (phase_ == ScannerPhase::Idle || !aligning_) {
        std::copy_n(input.player, 3, player_);
        facing_ = input.facing;
    }
    const int request = std::exchange(request_, -1);
    if (phase_ == ScannerPhase::Idle) {
        if (request == 1) return events;
        if (input.usePressed && input.playerFree && canToggle_)
            for (std::size_t i = 0; i < zones_.size(); ++i) {
                const ScannerZone& z = zones_[i];
                if (!input.zoneEnabled || !input.usable || !z.hasUseBound || z.mode >= 2) continue;
                bool inside = true;
                for (int a = 0; a < 3; ++a) inside = inside && std::fabs(input.player[a] - z.position[a] - z.useCentre[a]) < z.useHalf[a];
                if (!inside) continue;
                startEnter(i, events);
                break;
            }
        align(input, events);
        return events;
    }
    ++phaseUpdates_;
    align(input, events);
    switch (phase_) {
    case ScannerPhase::TakingOut:
        if (phaseUpdates_ >= input.takeOutUpdates) {
            phase_ = ScannerPhase::Entering;
            phaseUpdates_ = 0;
            activeFlag_ = true;
            cameraStarted_ = false;
            sound(SoundRequest::Kind::Play, kScannerSoundEnter, events);
        }
        break;
    case ScannerPhase::Entering:
        if (phaseUpdates_ == kScannerEnterCallbackUpdates) {
            phase_ = ScannerPhase::Active;
            const ScannerZone& z = zones_[std::size_t(zone_)];
            baseYaw_ = float(facing_) / 10430.378f;
            base_[0] = player_[0];
            base_[1] = player_[1] + input.playerHeight * kScannerEyeHeightFactor + z.cameraYOffset;
            base_[2] = player_[2];
            fire(z.enterTrigger, events);
        }
        break;
    case ScannerPhase::Active:
        if (canToggle_ && (input.exitPressed || request == 0)) {
            sound(SoundRequest::Kind::Stop, kScannerSoundScanning, events);
            startExit(events);
            break;
        }
        if (phaseUpdates_ == kScannerEnterEndUpdates) canToggle_ = true;
        align(input, events);
        if (phaseUpdates_ >= kScannerCameraStartUpdates) updateCamera(input, events);
        if (!playing(input, kScannerSoundExit) && !playing(input, kScannerSoundAmbience)) sound(SoundRequest::Kind::Play, kScannerSoundAmbience, events);
        if (phaseUpdates_ < kScannerCameraStartUpdates) break;
        if (state_ == 5 && !playing(input, kScannerSoundScanning)) sound(SoundRequest::Kind::Play, kScannerSoundScanning, events);
        updateRecords();
        updateTarget(input, events);
        break;
    case ScannerPhase::Exiting:
        if (phaseUpdates_ == kScannerEnterCallbackUpdates) {
            const ScannerZone& z = zones_[std::size_t(zone_)];
            fire(z.exitTrigger, events);
            ScannerEvent animation;
            animation.kind = ScannerEventKind::PlayerAnimation;
            animation.animation = kAnimationScannerIn;
            events.push_back(animation);
            facing_ = std::int16_t(std::lround(shown_[1] * 10430.378f));
            ScannerEvent pose;
            pose.kind = ScannerEventKind::PlayerPose;
            events.push_back(pose);
        } else if (phaseUpdates_ == kScannerEnterEndUpdates) {
            phase_ = ScannerPhase::Idle;
            activeFlag_ = false;
            canToggle_ = true;
            zone_ = -1;
            state_ = 0;
            lockProgress_ = scanProgress_ = 0;
            lockObject_ = target_ = current_ = scanning_ = -1;
            aim_[0] = aim_[1] = 0;
        }
        break;
    case ScannerPhase::Idle:
        break;
    }
    return events;
}

void Scanner::updateCamera(const ScannerInput& input, std::vector<ScannerEvent>& events) {
    const float dt = input.dt;
    if (!cameraStarted_) {
        zoom_ = settings_.zoomStart;
        aim_[0] = aim_[1] = 0;
    } else {
        const float zoomDelta = settings_.zoomDegreesPerSecond * (input.shoulderL - input.shoulderR) * dt;
        float pitch = input.stickY * settings_.turnDegreesPerSecond * dt * -kDegrees;
        float yaw = input.stickX * settings_.turnDegreesPerSecond * dt * kDegrees;
        const float step = settings_.turnDegreesPerSecond * dt;
        if (input.dpadUp) pitch = step * -kDegrees;
        if (input.dpadDown) pitch = step * kDegrees;
        if (input.dpadLeft) yaw = step * -kDegrees;
        if (input.dpadRight) yaw = step * kDegrees;
        zoom_ += zoomDelta;
        if (settings_.zoomMax <= zoom_) zoom_ = settings_.zoomMax;
        else if (zoom_ <= settings_.zoomMin) zoom_ = settings_.zoomMin;
        if (zoomDelta == 0.0f) {
            sound(SoundRequest::Kind::Stop, kScannerSoundZoomStart, events);
            sound(SoundRequest::Kind::Stop, kScannerSoundZoomLoop, events);
            zoomSound_ = 0;
        } else {
            if (zoomSound_ == 0 && !playing(input, kScannerSoundZoomStart)) {
                zoomSound_ = 1;
                sound(SoundRequest::Kind::Play, kScannerSoundZoomStart, events);
            } else if (zoomSound_ == 1 && !playing(input, kScannerSoundZoomStart)) {
                zoomSound_ = 2;
                sound(SoundRequest::Kind::Play, kScannerSoundZoomLoop, events);
            }
        }
        const float fraction = zoomFraction();
        if (fraction == 0.0f || fraction == 1.0f) {
            sound(SoundRequest::Kind::Stop, kScannerSoundZoomStart, events);
            sound(SoundRequest::Kind::Stop, kScannerSoundZoomLoop, events);
        }
        if (fraction == 0.0f && zone_ >= 0) fire(zones_[std::size_t(zone_)].zoomedInTrigger, events);
        const float scale = 1.0f - settings_.zoomTurnScale * (1.0f - fraction);
        aim_[0] += pitch * scale;
        aim_[1] += yaw * scale;
        limitAngle(-settings_.pitchMaxDegrees * kDegrees, -settings_.pitchMinDegrees * kDegrees, aim_[0]);
        limitAngle(-settings_.yawLimitDegrees * kDegrees, settings_.yawLimitDegrees * kDegrees, aim_[1]);
    }
    const Quaternion target = orientation(baseYaw_ + aim_[1], -aim_[0]);
    if (!cameraStarted_) {
        cameraStarted_ = true;
        forwardOf(target, shown_);
        return;
    }
    const Quaternion current = orientation(shown_[1], shown_[0]);
    const Quaternion next = slerp(current, target, settings_.smoothing);
    if (1.0f - std::fabs(dot(next, target)) <= 1e-5f) forwardOf(target, shown_);
    else forwardOf(next, shown_);
}

void Scanner::updateRecords() {
    records_.assign(objects_.size(), ScannerRecord{});
    const float fraction = zoomFraction();
    const float angleLimit = params_.angleZoomedOut - (params_.angleZoomedOut - params_.angleZoomedIn) * (1.0f - fraction);
    const float range = params_.rangeZoomedOut + (params_.rangeZoomedIn - params_.rangeZoomedOut) * (1.0f - fraction);
    float xScale, yScale;
    FollowCamera::projectionScales(zoom_, xScale, yScale);
    const float cy = std::cos(shown_[1]), sy = std::sin(shown_[1]), cp = std::cos(shown_[0]), sp = std::sin(shown_[0]);
    const float forward[3] = {cp * sy, sp, cp * cy};
    const float right[3] = {cy, 0.0f, -sy};
    const float up[3] = {-sp * sy, cp, -sp * cy};
    for (std::size_t i = 0; i < objects_.size(); ++i) {
        ScannerRecord& record = records_[i];
        record.object = i;
        const ScannableObject& object = objects_[i];
        float d[3];
        for (int a = 0; a < 3; ++a) d[a] = object.point[a] - base_[a];
        record.distance = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
        record.visible = true;
        if (record.distance >= (object.hasShownObject ? kScannerLinkedRange : range)) record.visible = false;
        const float depth = d[0] * forward[0] + d[1] * forward[1] + d[2] * forward[2];
        if (record.distance > 0.0f && angleLimit > 0.0f) {
            const float angle = std::acos(std::clamp(depth / record.distance, -1.0f, 1.0f)) * 180.0f * 0.31830987f;
            if (angleLimit < std::fabs(angle)) record.visible = false;
        }
        if (depth < 0.0f) {
            record.visible = false;
            continue;
        }
        const float side = d[0] * right[0] + d[1] * right[1] + d[2] * right[2];
        const float height = d[0] * up[0] + d[1] * up[1] + d[2] * up[2];
        record.screen[0] = kScreenHalfWidth + kScreenHalfWidth * xScale * side / depth;
        record.screen[1] = kScreenHalfHeight - kScreenHalfHeight * yScale * height / depth;
        record.radius = kScreenHalfWidth * xScale * object.radius / depth;
        const float dx = record.screen[0] - kScreenHalfWidth, dy = record.screen[1] - kScreenHalfHeight;
        record.distanceSquared = dx * dx + dy * dy;
    }
}

void Scanner::updateTarget(const ScannerInput& input, std::vector<ScannerEvent>& events) {
    int found = -1;
    for (std::size_t i = 0; i < records_.size(); ++i) {
        const ScannerRecord& record = records_[i];
        if (input.enabled && !input.enabled(objects_[i].instance)) continue;
        const float reach = record.radius + params_.reticleRadius;
        if (!record.visible || reach * reach < record.distanceSquared) continue;
        if (found < 0 || record.distanceSquared < records_[std::size_t(found)].distanceSquared) found = int(i);
    }
    target_ = found;
    const auto within = [&](int index, float radius) {
        if (index < 0) return false;
        const float reach = records_[std::size_t(index)].radius + radius;
        return reach * reach >= records_[std::size_t(index)].distanceSquared;
    };
    const ScannerZone* zone = zone_ >= 0 ? &zones_[std::size_t(zone_)] : nullptr;
    switch (state_) {
    case 0:
        lockProgress_ = 0;
        lockObject_ = -1;
        if (target_ >= 0) {
            state_ = 1;
            lockObject_ = target_;
        }
        break;
    case 1:
        if (target_ >= 0 && target_ == lockObject_) {
            if (0.0f <= lockProgress_ && lockProgress_ < 1.0f) lockProgress_ += params_.lockRate * input.dt;
            if (lockProgress_ >= 1.0f) {
                state_ = 2;
                lockProgress_ = 1.0f;
                if (zone) fire(zone->lockedOnTrigger, events);
                sound(SoundRequest::Kind::Play, kScannerSoundLocked, events);
            }
        } else {
            lockProgress_ = 0;
            lockObject_ = -1;
            state_ = 3;
        }
        break;
    case 2:
        if (!(target_ >= 0 && target_ == lockObject_ && within(target_, params_.reticleRadius))) {
            lockProgress_ = 0;
            lockObject_ = -1;
            state_ = 4;
            sound(SoundRequest::Kind::Play, kScannerSoundLockLost, events);
        }
        break;
    case 3:
    case 4:
        state_ = 0;
        lockObject_ = -1;
        scanProgress_ = 0;
        current_ = scanning_ = -1;
        break;
    default:
        break;
    }
    const auto cancel = [&]() {
        if (current_ >= 0) {
            state_ = state_ == 6 ? 8 : 7;
            scanning_ = -1;
        }
        sound(SoundRequest::Kind::Stop, kScannerSoundScanning, events);
    };
    bool scan = target_ >= 0 && input.scanHeld > 0.0f && within(target_, params_.reticleRadius);
    if (input.cancelPressed) {
        scan = false;
        cancel();
    }
    switch (state_) {
    case 2:
        if (scan) {
            state_ = 5;
            scanProgress_ = 0;
            current_ = scanning_ = target_;
        }
        break;
    case 5:
        if (!scan || target_ != scanning_) {
            cancel();
            break;
        }
        scanProgress_ += params_.scanRate * input.dt;
        if (scanProgress_ > 1.0f) {
            sound(SoundRequest::Kind::Stop, kScannerSoundScanning, events);
            sound(SoundRequest::Kind::Play, kScannerSoundScanned, events);
            state_ = 6;
            scanProgress_ = 1.0f;
            scanning_ = -1;
            ScannableObject& object = objects_[std::size_t(current_)];
            if (!object.triggerScanned.empty() && !object.scanned) {
                object.scanned = true;
                fire(object.triggerScanned, events);
            }
        }
        break;
    case 6:
        if (!within(current_, params_.reticleRadius)) cancel();
        break;
    case 7:
    case 8:
        scanProgress_ -= params_.scanDecay * input.dt;
        if (scanProgress_ < 0.0f) {
            current_ = scanning_ = -1;
            state_ = 0;
            scanProgress_ = 0;
        }
        break;
    default:
        break;
    }
}

}
