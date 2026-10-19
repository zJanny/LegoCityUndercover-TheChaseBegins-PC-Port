#include "runtime/stud_drop.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace chase::runtime {
namespace {

constexpr float kPi = 3.14159274f;
// 0x21e698..0x21e6a8 and 0x199484..0x199488: the direction byte.
constexpr float kDirectionSteps = 64.0f;
constexpr float kInversePi = 0.318309873f;
constexpr float kBurstLift = 0.5f;
// 0x199484..0x1994d0 (literal pool of 0x19913c).
constexpr float kSpawnLife = 8.5f;
constexpr float kSpreadScale = 0.1f;
constexpr float kSpawnRise = 0.5f;
constexpr float kGroundMargin = 0.05f;
constexpr float kSlowSpread = 6.0f;
// 0x180484..0x1804b4 (literal pool of 0x180168) and the type table rows 0..4 of 0x2ed0b4 (+0x7c damping 0, +0x80 gravity 1.0, +0x84 bounce 0.3).
constexpr float kFall = 0.05f;
constexpr float kSlowFall = 0.0025f;
constexpr float kTypeGravity = 1.0f;
constexpr float kTypeBounce = 0.3f;
constexpr std::int32_t kRestSpeedBits = 0x3d4ccccd;
constexpr std::int32_t kStillRiseBits = 0x3f000000;
constexpr float kRestLife = 8.5f;
constexpr float kRestLifeStill = 1.0f;
// 0x214bd0 / 0x214bd4: SpawnDir degrees to radians.
constexpr float kDegrees = 0.0055555557f;
constexpr std::int32_t kNoSpawnDir = 999;

std::int32_t bitsOf(float value) {
    std::int32_t bits = 0;
    std::memcpy(&bits, &value, 4);
    return bits;
}

bool isStud(std::uint8_t type) { return type < kStudDropHeartType; }

}  // namespace

std::optional<DebrisVec3> studDropDirection(const DebrisVec3& direction, float scale) {
    if (direction[0] == 0.0f && direction[2] == 0.0f) return std::nullopt;
    const float angle = std::atan2(direction[0], direction[2]);
    const std::uint16_t step = std::uint16_t(std::int32_t((angle + kPi) * kDirectionSteps * kInversePi));
    const float back = float(step) * kPi * (1.0f / kDirectionSteps) - kPi;
    return DebrisVec3{std::sin(back) * scale, 0.0f, std::cos(back) * scale};
}

StudDropMode StudDropField::drop(const StudDropRequest& request, const DebrisLineTest& wall, const DebrisLineTest& ground) {
    StudDropMode mode = request.mode;
    if (mode == StudDropMode::Random) mode = random_.next() % 3u == 0 ? StudDropMode::Heart : StudDropMode::Count;
    std::array<PickupSortKey, kStudDropRecordCount> keys{};
    for (unsigned i = 0; i < kStudDropRecordCount; ++i) keys[i] = {std::uint8_t(records_[i].state), records_[i].type, records_[i].life};
    const std::array<std::uint8_t, kDebrisBurstLimit> chosen = choosePickupRecords(keys.data(), kStudDropRecordCount);
    DebrisVec3 position = request.position;
    position[1] += kBurstLift;
    random_.next();

    const std::optional<DebrisVec3> direction = studDropDirection(request.direction, scale);
    const DebrisVec3 heading = direction.value_or(DebrisVec3{0, 0, 0});
    const DebrisBurstRays rays = debrisBurstRays(position, heading, false, scale, wall, ground, planes_);
    const StudDropMode resolved = mode;
    unsigned remaining = request.count;
    for (unsigned i = 0; remaining != 0 && i < kDebrisBurstLimit; ++i) {
        StudDropRecord& record = records_[chosen[i]];
        record.state = StudDropState::Flying;
        record.life = kSpawnLife;
        record.flags49 = std::uint8_t((record.flags49 & ~0x03u) | (request.slow ? 2u : 0u));
        record.skip = request.skip;
        record.position = position;
        record.plane = rays.plane;
        if (request.directionIsVelocity) {
            record.velocity = request.direction;
        } else {
            const float rx = gameRandomUnit(random_);
            record.velocity[1] = kSpawnRise;
            record.velocity[0] = (heading[0] + request.spread * (rx - 0.5f)) * kSpreadScale;
            const float rz = gameRandomUnit(random_);
            record.velocity[2] = (heading[2] + request.spread * (rz - 0.5f)) * kSpreadScale;
        }
        for (float& v : record.velocity) v *= scale;
        record.ground = debrisAheadGround(rays, record.velocity, scale, ground, rays.restHeight - kGroundMargin);
        if (mode == StudDropMode::Heart) {
            record.type = kStudDropHeartType;
            mode = StudDropMode::Count;
        } else {
            record.flags49 = std::uint8_t((record.flags49 & ~0x04u) | (request.collectAtOnce ? 4u : 0u));
            if (remaining >= kStudDropBlueUnits) {
                record.type = std::uint8_t(StudKind::Blue);
                remaining -= kStudDropBlueUnits;
            } else if (remaining >= kStudDropGoldUnits) {
                record.type = std::uint8_t(StudKind::Gold);
                remaining -= kStudDropGoldUnits;
            } else {
                record.type = std::uint8_t(StudKind::Silver);
                remaining -= 1;
            }
        }
        if (record.flags49 & 2) {
            record.velocity[0] *= kSlowSpread;
            record.velocity[2] *= kSlowSpread;
            for (float& v : record.velocity) v *= kSpreadScale;
        }
        record.velocity[1] *= kTypeGravity;
    }
    return resolved;
}

StudDropUpdate StudDropField::update(float frameDelta, const std::array<float, 3>& pickupPoint, const std::optional<std::array<float, 3>>& flightTarget,
                                     StudField& counter) {
    StudDropUpdate result;
    if (landingWait_ != 0) --landingWait_;
    for (StudDropRecord& record : records_) {
        if (record.state == StudDropState::Collecting) {
            record.life -= frameDelta;
            if (!(record.life > 0.0f)) {
                record.state = StudDropState::Free;
                counter.bank(StudKind(record.type));
            } else if (flightTarget) {
                const float step = 1.0f / (kStudFlightRate * record.life);
                for (unsigned a = 0; a < 3; ++a) record.position[a] += ((*flightTarget)[a] - record.position[a]) * step;
            }
            continue;
        }
        if (record.state != StudDropState::Resting && record.state != StudDropState::Flying) continue;
        if (isStud(record.type)) {
            record.skip = std::int16_t(record.skip - 1);
            if (record.skip <= 0) {
                const float dx = record.position[0] - pickupPoint[0], dy = record.position[1] - pickupPoint[1], dz = record.position[2] - pickupPoint[2];
                const float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
                if (kMeasuredStudPickupRadius > distance || (record.flags49 & 4)) {
                    record.state = StudDropState::Collecting;
                    record.life = kStudCollectSeconds;
                    result.sounds.push_back({kStudCollectSounds[record.type], record.position, 1.0f, kStudCollectVolume});
                    ++result.collected;
                } else {
                    record.skip = std::int16_t(std::min(int((distance - kMeasuredStudPickupRadius) / kStudSkipUnit), kStudSkipLimit));
                }
            }
        }
        if (record.state == StudDropState::Flying) {
            if (record.plane != -1) debrisReflectAtPlane(record.position, record.velocity, planes_.planes[unsigned(record.plane)]);
            for (unsigned a = 0; a < 3; ++a) record.position[a] += record.velocity[a];
            if (!(record.position[1] < record.ground)) {
                record.velocity[1] -= ((record.flags49 & 2) ? kSlowFall : kFall) * kTypeGravity;
            } else {
                if (landingWait_ == 0 && isStud(record.type)) {
                    landingWait_ = kStudLandingSoundWait;
                    const float r = gameRandomUnit(random_);
                    result.sounds.push_back({kStudLandingSound, record.position, kStudLandingFrequencyBase + r * kStudLandingFrequencyRange});
                }
                record.position[1] = record.ground;
                if (record.velocity[0] == 0.0f && record.velocity[2] == 0.0f) {
                    record.state = StudDropState::Resting;
                    record.life = bitsOf(record.velocity[1]) == kStillRiseBits ? kRestLifeStill : kRestLife;
                } else {
                    record.velocity[1] = -(record.velocity[1] * kTypeBounce);
                    if (bitsOf(record.velocity[1]) < kRestSpeedBits) record.state = StudDropState::Resting;
                }
            }
        }
        record.life -= frameDelta;
        if (!(record.life > 0.0f) && record.state != StudDropState::Free) record.state = StudDropState::Free;
    }
    return result;
}

bool StudDropField::drawn(unsigned index) const {
    const StudDropRecord& record = records_.at(index);
    if (record.state == StudDropState::Flying || record.state == StudDropState::Collecting) return true;
    if (record.state != StudDropState::Resting) return false;
    if (bitsOf(record.life) >= bitsOf(kStudDropBlinkLife)) return true;
    return (std::int32_t(kStudFlightRate * record.life) & 4) != 0;
}

std::optional<StudDropRequest> studSpawnRequest(const StudSpawnParameters& parameters) {
    if (parameters.count == 0) return std::nullopt;
    StudDropRequest request;
    request.mode = parameters.modeCount ? StudDropMode::Count : StudDropMode::Random;
    request.count = parameters.count;
    request.position = {parameters.node[12], parameters.node[13], parameters.node[14]};
    request.spread = parameters.spawnCone;
    request.slow = parameters.slow;
    if (parameters.spawnDir != kNoSpawnDir) {
        const float angle = float(parameters.spawnDir) * kPi * kDegrees;
        DebrisVec3 direction = {std::sin(angle), 0.0f, std::cos(angle)};
        if (parameters.spawnLocal != 0) {
            const std::array<float, 16>& m = parameters.node;
            const float x = direction[0] * m[0] + direction[1] * m[4] + direction[2] * m[8] + m[12];
            const float z = direction[0] * m[2] + direction[1] * m[6] + direction[2] * m[10] + m[14];
            const float l = std::sqrt(x * x + z * z);
            direction = l <= 1.0f / 8388608.0f ? DebrisVec3{0, 0, 1} : DebrisVec3{x / l, 0.0f, z / l};
        }
        request.direction = direction;
    } else if (parameters.forward) {
        request.direction = {parameters.node[8], parameters.node[9], parameters.node[10]};
    }
    return request;
}

}
