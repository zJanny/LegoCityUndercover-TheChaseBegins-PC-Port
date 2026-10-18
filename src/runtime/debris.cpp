#include "runtime/debris.hpp"

#include <cmath>
#include <cstring>
#include <utility>

namespace chase::runtime {
namespace {

constexpr float kTwoPi = 6.2831855f;
// 0x199484..0x1994c8 (literal pool of 0x19913c).
constexpr float kStartReach = 2.5f;
constexpr float kGroundRayLength = 10.0f;
constexpr float kGroundMissDrop = 20.0f;
constexpr float kRestHeight = 0.56f;
constexpr float kWallNudge = 0.01f;
constexpr float kAheadReach = 2.0f;
constexpr float kSpawnLife = 8.5f;
constexpr float kSpreadScale = 0.1f;
constexpr float kSpawnRise = 0.5f;
constexpr float kGroundMargin = 0.05f;
constexpr float kBrickRiseBase = 0.3f;
constexpr float kBrickRiseRange = 0.4f;
constexpr float kBrickLifeBase = 2.0f;
constexpr float kSpinBase = 0.8f;
constexpr float kSpinRange = 0.4f;
constexpr float kSlowSpread = 6.0f;
// 0x21e2e4: 0x21e6a8 = 0.5 added to the descriptor's y.
constexpr float kBurstLift = 0.5f;
// 0x180484..0x1804b4 (literal pool of 0x180168).
constexpr float kFall = 0.05f;
constexpr float kSlowFall = 0.0025f;
constexpr std::int32_t kRestSpeedBits = 0x3d4ccccd;  // 0.05 compared as a signed integer (0x180998)
constexpr float kRestLife = 8.5f;
constexpr float kRestLifeStud = 1.0f;
// 0x28a4b0: a resting brick blinks with bit 2 of rate x life during its last second.
constexpr int kBlinkBit = 4;

std::int32_t bitsOf(float value) {
    std::int32_t bits = 0;
    std::memcpy(&bits, &value, 4);
    return bits;
}

float dot(const DebrisVec3& a, const DebrisVec3& b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }

float length(const DebrisVec3& v) { return std::sqrt(dot(v, v)); }

// 0x257ea0: (0, 0, 1) for a length of 2^-23 or less.
DebrisVec3 normalised(const DebrisVec3& v) {
    const float l = length(v);
    if (l <= 1.0f / 8388608.0f) return {0, 0, 1};
    return {v[0] / l, v[1] / l, v[2] / l};
}

bool hasSecondColour(std::uint32_t colour) { return (colour & 0xff) != 0xff || ((colour >> 8) & 0xff) != 0 || ((colour >> 16) & 0xff) != 0xff; }

}  // namespace

std::uint32_t debrisColourOf(const std::array<float, 3>& value) {
    std::uint32_t colour = 0xff000000u;
    for (unsigned c = 0; c < 3; ++c) colour |= (std::uint32_t(std::int32_t(value[c])) & 0xffu) << (8 * c);
    return colour;
}

std::array<float, 9> debrisRotation(float a, float b, float c) {
    const float sa = std::sin(a), ca = std::cos(a), sb = std::sin(b), cb = std::cos(b), sc = std::sin(c), cc = std::cos(c);
    return {cb * cc, sa * sb * cc + ca * sc, sa * sc - ca * sb * cc, -(cb * sc), ca * cc - sa * sb * sc, sa * cc + ca * sb * sc, sb, -(sa * cb), ca * cb};
}

// 0x21e2e4 lines 0x21e3c0..0x21e598: a counting sort of the mode's records by state (stable, by index), then the records of states 1 to 4 by
// pickup type and remaining life, both ascending; 0x342d04 keeps the first ten.
std::array<std::uint8_t, kDebrisBurstLimit> DebrisField::chooseRecords() const {
    std::array<unsigned, 5> ends{};
    for (const DebrisRecord& record : records_) ++ends[std::uint8_t(record.state)];
    for (unsigned s = 1; s < ends.size(); ++s) ends[s] += ends[s - 1];
    std::array<unsigned, 5> cursor = ends;
    std::array<std::uint8_t, kDebrisRecordCount> order{};
    for (unsigned i = kDebrisRecordCount; i-- > 0;) order[--cursor[std::uint8_t(records_[i].state)]] = std::uint8_t(i);
    for (unsigned bucket = 0; bucket + 1 < ends.size(); ++bucket)
        for (unsigned pass = ends[bucket]; pass < ends[bucket + 1]; ++pass)
            for (unsigned j = ends[bucket] + 1; j < ends[bucket + 1]; ++j) {
                const DebrisRecord& earlier = records_[order[j - 1]];
                const DebrisRecord& later = records_[order[j]];
                if (later.type < earlier.type || (later.type == earlier.type && later.life < earlier.life)) std::swap(order[j - 1], order[j]);
            }
    std::array<std::uint8_t, kDebrisBurstLimit> chosen{};
    for (unsigned i = 0; i < kDebrisBurstLimit; ++i) chosen[i] = order[i];
    return chosen;
}

void DebrisField::setRecord(unsigned index, const DebrisRecord& record) {
    records_.at(index) = record;
    if (record.object != 0xff) objects_.at(record.object).enabled = true;
}

void DebrisField::release(DebrisRecord& record) {
    if (record.type != kDebrisPickupType || record.object == 0xff) return;
    objects_[record.object].enabled = false;
}

void DebrisField::burst(const DebrisBurst& request, const DebrisLineTest& wall, const DebrisLineTest& ground) {
    DebrisVec3 position = request.position;
    position[1] += kBurstLift;
    random_.next();
    const std::array<std::uint8_t, kDebrisBurstLimit> chosen = chooseRecords();

    DebrisVec3 start = position;
    DebrisVec3 end = start;
    end[1] -= kGroundRayLength;
    DebrisVec3 groundAt = start;
    groundAt[1] = end[1];
    std::int8_t plane = -1;
    if (wall)
        if (const std::optional<DebrisLineHit> hit = wall(position, start)) {
            planes_[planeIndex_] = {hit->normal[0], hit->normal[1], hit->normal[2], -dot(hit->point, hit->normal)};
            plane = std::int8_t(planeIndex_);
            planeIndex_ = (planeIndex_ + 1) & (kDebrisPlaneCount - 1);
            end[0] = hit->point[0] + (hit->point[0] - end[0]) * kWallNudge;
            start[0] = end[0];
            end[2] = hit->point[2] + (hit->point[2] - end[2]) * kWallNudge;
            start[2] = end[2];
        }
    if (const std::optional<DebrisLineHit> hit = ground ? ground(start, end) : std::nullopt) groundAt = hit->point;
    else groundAt[1] -= kGroundMissDrop;
    const float groundY = groundAt[1] + settings.scale * kRestHeight;

    unsigned remaining = request.count;
    for (unsigned i = 0; remaining != 0 && i < kDebrisBurstLimit; ++i) {
        DebrisRecord& record = records_[chosen[i]];
        if (record.state != DebrisState::Free) {
            record.state = DebrisState::Free;
            release(record);
        }
        record.state = DebrisState::Flying;
        record.life = kSpawnLife;
        record.flags49 = std::uint8_t((record.flags49 & 0xfcu) | (request.slow ? 2u : 0u));
        record.position = position;
        record.plane = plane;
        const float rx = gameRandomUnit(random_);
        record.velocity = {request.spread * (rx - 0.5f) * kSpreadScale, kSpawnRise, 0};
        const float rz = gameRandomUnit(random_);
        record.velocity[2] = request.spread * (rz - 0.5f) * kSpreadScale;
        for (float& v : record.velocity) v *= settings.scale;
        record.ground = groundY - kGroundMargin;
        const DebrisVec3 flat = normalised({record.velocity[0], 0, record.velocity[2]});
        const DebrisVec3 aheadFrom = {start[0] + flat[0] * kAheadReach, start[1] + flat[1] * kAheadReach, start[2] + flat[2] * kAheadReach};
        const DebrisVec3 aheadTo = {end[0] + flat[0] * kAheadReach, end[1] + flat[1] * kAheadReach, end[2] + flat[2] * kAheadReach};
        if (ground)
            if (const std::optional<DebrisLineHit> hit = ground(aheadFrom, aheadTo)) record.ground = hit->point[1] + settings.scale * kRestHeight;

        record.type = kDebrisPickupType;
        record.flags22 = std::uint8_t((record.flags22 & ~0x07u) | (random_.next() & 1u));
        record.velocity[0] *= 2.0f;
        record.velocity[1] = (gameRandomUnit(random_) * kBrickRiseRange + kBrickRiseBase) * settings.scale;
        record.velocity[2] *= 2.0f;
        record.colour = request.colour;
        if (hasSecondColour(request.secondColour)) {
            bool second = false;
            if (request.ratio == 0.0f) second = random_.next() % 2u != 0;
            if (!second && request.ratio > 0.0f) second = !(request.ratio > float(i) / float(remaining + i));
            if (second) record.colour = request.secondColour;
        }
        record.object = 0xff;
        record.life = gameRandomUnit(random_) + kBrickLifeBase;
        for (unsigned slot = 0; slot < kDebrisObjectCount; ++slot) {
            DebrisObject& object = objects_[slot];
            if (object.enabled) continue;
            object.enabled = true;
            object.alpha = std::uint8_t(request.colour >> 24);
            record.object = std::uint8_t(slot);
            const float a = gameRandomUnit(random_) * kTwoPi;
            const float b = gameRandomUnit(random_) * kTwoPi;
            const float c = gameRandomUnit(random_) * kTwoPi;
            const std::array<float, 9> rotation = debrisRotation(a, b, c);
            object.matrix = {rotation[0], rotation[1], rotation[2], 0, rotation[3], rotation[4], rotation[5], 0,
                             rotation[6], rotation[7], rotation[8], 0, record.position[0], record.position[1], record.position[2], 1};
            object.spinSpeed = gameRandomUnit(random_) * kSpinRange + kSpinBase;
            object.spinMode = (record.flags22 & 1) ? 5 : 1;
            break;
        }
        --remaining;
        if (record.flags49 & 2) {
            record.velocity[0] *= kSlowSpread;
            record.velocity[2] *= kSlowSpread;
            for (float& v : record.velocity) v *= kSpreadScale;
        }
        record.velocity[1] *= kDebrisTypeGravity;
    }
}

void DebrisField::place(DebrisRecord& record) {
    if (record.type != kDebrisPickupType || record.object == 0xff || record.state == DebrisState::Free) return;
    DebrisObject& object = objects_[record.object];
    if (!object.enabled) {
        record.object = 0xff;
        return;
    }
    if (record.state == DebrisState::Resting) {
        if (bitsOf(record.life) < bitsOf(1.0f) && (std::int32_t(settings.rate * record.life) & kBlinkBit) == 0) object.alpha = 0;
        else object.alpha = std::uint8_t(record.colour >> 24);
        return;
    }
    object.matrix[12] = record.position[0];
    object.matrix[13] = record.position[1];
    object.matrix[14] = record.position[2];
    float fade = 1.0f;
    DebrisVec3 toCamera = {record.position[0] - cameraPosition_[0], record.position[1] - cameraPosition_[1], record.position[2] - cameraPosition_[2]};
    const float f = (length(toCamera) - settings.fadeNear) / (settings.fadeFar - settings.fadeNear);
    if (bitsOf(f) < bitsOf(1.0f)) fade = f <= 0.0f ? 0.0f : f;
    const std::uint32_t alpha = std::uint32_t(std::int32_t(fade * 255.0f));
    object.colour = (record.colour & 0x00ffffffu) | (alpha << 24);
    object.alpha = std::uint8_t(alpha & 0xff);
}

void DebrisField::update(float frameDelta, const DebrisVec3& camera) {
    cameraPosition_ = camera;
    for (DebrisRecord& record : records_) {
        if (record.state != DebrisState::Resting && record.state != DebrisState::Flying) continue;
        if (record.state == DebrisState::Flying) {
            if (record.plane != -1) {
                const std::array<float, 4>& p = planes_[unsigned(record.plane)];
                const DebrisVec3 normal = {p[0], p[1], p[2]};
                const DebrisVec3 next = {record.position[0] + record.velocity[0], record.position[1] + record.velocity[1],
                                         record.position[2] + record.velocity[2]};
                if (dot(next, normal) + p[3] < 0.0f) {
                    DebrisVec3 back = normalised(record.velocity);
                    for (float& v : back) v *= -1.0f;
                    const float k = dot(back, normal) * 2.0f;
                    const float speed = length(record.velocity);
                    for (unsigned a = 0; a < 3; ++a) record.velocity[a] = (normal[a] * k - back[a]) * speed;
                }
            }
            for (unsigned a = 0; a < 3; ++a) record.position[a] += record.velocity[a];
            if (!(record.position[1] < record.ground)) {
                if (kDebrisTypeGravity > 0.0f) record.velocity[1] -= ((record.flags49 & 2) ? kSlowFall : kFall) * kDebrisTypeGravity;
            } else {
                record.position[1] = record.ground;
                if (record.velocity[0] == 0.0f && record.velocity[2] == 0.0f) {
                    record.state = DebrisState::Resting;
                    record.life = record.velocity[1] == 0.5f ? kRestLifeStud : kRestLife;
                    if (bitsOf(record.velocity[1]) < kRestSpeedBits) record.state = DebrisState::Resting;
                } else {
                    record.velocity[1] = -(record.velocity[1] * kDebrisTypeBounce);
                    if (bitsOf(record.velocity[1]) < kRestSpeedBits) record.state = DebrisState::Resting;
                }
            }
        }
        record.life -= frameDelta;
        if (record.life <= 0.0f) {
            record.state = DebrisState::Free;
            release(record);
        }
        place(record);
    }
}

}
