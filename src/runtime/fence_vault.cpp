#include "runtime/fence_vault.hpp"
#include "assets/object_classes.hpp"
#include "runtime/level_start.hpp"
#include "runtime/lever_use.hpp"
#include "runtime/player_intent.hpp"
#include "runtime/player_locomotion.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>

namespace chase::runtime {
namespace {

using Vec = std::array<float, 3>;

constexpr float kHeadingUnitsPerRadian = 10430.378f;

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

int intParam(const assets::LevelObject& object, const assets::Level& cls, const char* name, int fallback) {
    const assets::LevelParam* found = effectiveParam(object, cls, assets::nameHash(name));
    if (!found) return fallback;
    if (found->type == 0) {
        float value;
        std::memcpy(&value, &found->value, 4);
        return int(value);
    }
    return int(found->value);
}

float dot(const float* a, const Vec& b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }

std::int16_t headingOf(float x, float z) { return std::int16_t(std::uint16_t(std::int32_t(gameAtan2(x, z) * kHeadingUnitsPerRadian))); }

bool faceTestPasses(int mode, const Vec& playerForward, const float* objectForward, const Vec& toObject) {
    const float facing = dot(objectForward, playerForward);
    switch (mode) {
    case 1: return playerForward[0] * toObject[0] + playerForward[1] * toObject[1] + playerForward[2] * toObject[2] >= 0.0f;
    case 2: return !(facing > 0.0f) && !(dot(objectForward, toObject) > 0.0f);
    case 3: return !(facing < 0.0f) && !(dot(objectForward, toObject) < 0.0f);
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
    for (std::size_t a = 0; a < 3; ++a) out[a] = t[index][a] + (t[index + 1][a] - t[index][a]) * f;
    return out;
}

}

std::vector<FenceVaultObject> loadFenceVaults(const assets::Level& level, const assets::ClassLookup& classOf) {
    std::vector<FenceVaultObject> fences;
    for (const assets::LevelObject& object : level.objects) {
        const assets::Level* cls = classOf(object.className);
        if (!cls || assets::objectKind(object, *cls) != kFenceVaultObjectKind) continue;
        const std::optional<assets::Placement> placed = assets::objectStart(level, object.instance);
        if (!placed) continue;
        FenceVaultObject fence;
        fence.instance = lower(object.instance);
        fence.matrix = assets::placementMatrix(*placed);
        fence.vaultType = intParam(object, *cls, "VaultType", 0);
        fence.faceTowards = intParam(object, *cls, "FaceTowards", 0) & 7;
        for (const assets::LevelBound& bound : cls->bounds) {
            if (lower(bound.name) != "usebound") continue;
            fence.hasUseBound = true;
            std::copy_n(bound.centre, 3, fence.useCentre);
            std::copy_n(bound.half, 3, fence.useHalf);
        }
        fences.push_back(std::move(fence));
    }
    return fences;
}

bool fenceVaultZoneHolds(const FenceVaultObject& fence, const float player[3], std::int16_t facing) {
    if (!fence.hasUseBound) return false;
    const Vec d = {player[0] - fence.position()[0], player[1] - fence.position()[1], player[2] - fence.position()[2]};
    if (fence.faceTowards != 0) {
        const float size = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
        const Vec toFence = size > 1.1920929e-7f ? Vec{-d[0] / size, -d[1] / size, -d[2] / size} : Vec{-d[0], -d[1], -d[2]};
        const float heading = headingRadians(facing);
        if (!faceTestPasses(fence.faceTowards, {std::sin(heading), 0.0f, std::cos(heading)}, fence.forward(), toFence)) return false;
    }
    const float local[3] = {dot(fence.right(), d), dot(fence.up(), d), dot(fence.forward(), d)};
    for (int a = 0; a < 3; ++a)
        if (!(std::fabs(local[a] - fence.useCentre[a]) < fence.useHalf[a])) return false;
    return true;
}

int fenceVaultOffer(const std::vector<FenceVaultObject>& fences, const float player[3], std::int16_t facing,
                    const std::function<bool(const FenceVaultObject&)>& disabled) {
    for (std::size_t i = 0; i < fences.size(); ++i) {
        if (disabled && disabled(fences[i])) continue;
        if (fenceVaultZoneHolds(fences[i], player, facing)) return int(i);
    }
    return -1;
}

FenceVaultApproach fenceVaultApproach(const FenceVaultObject& fence, const float player[3], std::int16_t facing) {
    const Vec d = {player[0] - fence.position()[0], player[1] - fence.position()[1], player[2] - fence.position()[2]};
    FenceVaultApproach approach;
    const float along = dot(fence.right(), d);
    for (std::size_t a = 0; a < 3; ++a) approach.position[a] = fence.position()[a] + fence.right()[a] * along;
    approach.position[1] = player[1] + fence.right()[1] * along;
    const float offset = fence.vaultType == 1 ? kFenceVaultOffsetType1 : kFenceVaultOffset;
    const float* z = fence.forward();
    if (dot(z, d) > 0.0f) {
        for (std::size_t a = 0; a < 3; ++a) approach.position[a] += z[a] * offset;
        approach.heading = headingOf(-z[0], -z[2]);
    } else {
        for (std::size_t a = 0; a < 3; ++a) approach.position[a] -= z[a] * offset;
        approach.heading = headingOf(z[0], z[2]);
    }
    const float heading = headingRadians(facing);
    if (std::sin(heading) * d[0] + std::cos(heading) * d[2] < 0.0f) approach.seconds = kFenceVaultLerpSecondsFacing;
    return approach;
}

int fenceVaultClip(const FenceVaultObject& fence) { return fence.vaultType == 1 ? kAnimationVault : kAnimationVaultFence; }

float fenceVaultRiseScale(const FenceVaultObject& fence, float playerY, const assets::RootMotion& motion) {
    const float extent = motion.maximum[1] - motion.minimum[1];
    return extent != 0.0f ? (fence.position()[1] - playerY) / extent : 0.0f;
}

FenceVaultStep FenceVaulter::step() const {
    FenceVaultStep out;
    out.position = position_;
    out.facing = facing_;
    out.animation = state_ == kPlayerStateFenceVault ? animation_ : 0;
    return out;
}

void FenceVaulter::stepLerp(float dt) {
    facing_ = turnTowards(facing_, approach_.heading, kUseLerpTurnStep);
    if (lerp_ == 1.0f) {
        facing_ = approach_.heading;
        pending_ = kPlayerStateFenceVault;
        return;
    }
    lerp_ = std::min(1.0f, lerp_ + dt / approach_.seconds);
    for (std::size_t a = 0; a < 3; ++a) position_[a] = start_[a] + (approach_.position[a] - start_[a]) * lerp_;
}

FenceVaultStep FenceVaulter::start(const std::vector<FenceVaultObject>& fences, std::size_t fence, const float position[3], std::int16_t facing, float dt) {
    fence_ = fence;
    approach_ = fenceVaultApproach(fences[fence], position, facing);
    state_ = kPlayerStateFenceVaultLerp;
    pending_ = 0;
    lerp_ = 0.0f;
    start_ = {position[0], position[1], position[2]};
    position_ = start_;
    facing_ = facing;
    animation_ = fenceVaultClip(fences[fence]);
    stepLerp(dt);
    FenceVaultStep out = step();
    out.entered = kPlayerStateFenceVaultLerp;
    return out;
}

FenceVaultStep FenceVaulter::update(const std::vector<FenceVaultObject>& fences, const FenceVaultClipData& clip, float dt) {
    std::optional<unsigned> entered;
    if (pending_ != 0) {
        state_ = pending_;
        pending_ = 0;
        entered = state_;
        start_ = position_;
        frame_ = 0.0f;
        rise_ = clip.motion ? fenceVaultRiseScale(fences[fence_], start_[1], *clip.motion) : 0.0f;
    }
    if (state_ == kPlayerStateFenceVaultLerp) {
        stepLerp(dt);
        return step();
    }
    if (state_ != kPlayerStateFenceVault) return step();
    const float frame = frame_;
    frame_ += dt * clip.framesPerSecond;
    const float before = position_[1];
    if (clip.motion) {
        const Vec root = rootAt(*clip.motion, frame);
        const float heading = headingRadians(facing_);
        const float s = std::sin(heading), c = std::cos(heading);
        position_[0] = start_[0] + root[0] * c + root[2] * s;
        position_[1] = start_[1] + root[1] * rise_;
        position_[2] = start_[2] - root[0] * s + root[2] * c;
    }
    FenceVaultStep out = step();
    out.entered = entered;
    if (frame < clip.lastFrame) return out;
    out.ended = true;
    out.fallSpeed = before - position_[1];
    out.position[1] -= out.fallSpeed;
    out.animation = 0;
    state_ = 0;
    return out;
}

}
