#include "runtime/debris_pickup.hpp"

#include "runtime/studs.hpp"
#include "runtime/super_brick.hpp"

#include <algorithm>
#include <cmath>

namespace chase::runtime {
namespace {

float distance(const DebrisVec3& a, const DebrisVec3& b) {
    const float dx = a[0] - b[0], dy = a[1] - b[1], dz = a[2] - b[2];
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

std::int8_t superValueByte(const DebrisRecord& record) { return std::int8_t(std::uint8_t(record.superValue & 0xffu)); }

SuperDebrisPlacement placement(unsigned index, const DebrisRecord& record) {
    SuperDebrisPlacement placed;
    placed.record = index;
    placed.position = record.position;
    placed.scale = record.state == DebrisState::Collected ? 0.5f + record.life * 0.5f : 1.0f;
    placed.colour = (record.colour & 0x00ffffffu) | 0xff000000u;
    placed.alpha = std::uint8_t(record.colour >> 24);
    return placed;
}

void place(unsigned index, const DebrisRecord& record, DebrisPickupStep& step) {
    if (record.object != 0xff) step.placements.push_back(placement(index, record));
}

void collect(DebrisRecord& record, const DebrisPickupInput& input, DebrisPickupStep& step) {
    record.state = DebrisState::Collected;
    record.life = kDebrisCollectTime;
    step.sounds.push_back({kDebrisCollectSoundMeasured, record.position, kDebrisCollectVolume});
    if (record.object == 0xff || !(record.flags22 & kDebrisFlagSuper) || record.kind != kSuperDebrisKind) return;
    record.colour = input.tint;
    ++step.shown;
}

void bank(unsigned index, DebrisRecord& record, DebrisPickupStep& step, std::uint32_t& count) {
    record.state = DebrisState::Free;
    step.freedRecords.push_back(index);
    step.studUnits += kDebrisStudUnits;
    const std::int8_t value = superValueByte(record);
    const std::uint32_t unsignedValue = std::uint32_t(std::int32_t(value));
    count += unsignedValue;
    step.banked.push_back({index, value, count, unsignedValue >= kSuperBrickBigValue ? kSuperBrickSoundBig : kSuperBrickSoundSmall});
}

}

DebrisPickupStep updateDebrisPickup(DebrisRecord* records, unsigned recordCount, const DebrisPickupInput& input, std::uint32_t& count) {
    DebrisPickupStep step;
    for (unsigned index = 0; index < recordCount; ++index) {
        DebrisRecord& record = records[index];
        if (record.type != kDebrisPickupType) continue;
        switch (record.state) {
        case DebrisState::Resting:
        case DebrisState::Flying: {
            if (input.blocked || !(record.flags22 & kDebrisFlagPickable)) break;
            record.skip = std::int16_t(record.skip - 1);
            if (record.skip > 0) break;
            const float d = distance(record.position, input.pickupPoint);
            if (input.magnet && kDebrisMagnetRadius > d) {
                record.state = DebrisState::Magnet;
                record.life = kDebrisMagnetLife;
            } else if (kMeasuredStudPickupRadius > d || (record.flags49 & kDebrisFlagCollectAtOnce)) {
                collect(record, input, step);
            } else {
                record.skip = std::int16_t(std::min(int((d - kMeasuredStudPickupRadius) / kStudSkipUnit), kStudSkipLimit));
                break;
            }
            record.life -= input.frameDelta;
            if (!(record.life > 0.0f)) {
                record.state = DebrisState::Free;
                step.freedRecords.push_back(index);
            } else {
                place(index, record, step);
            }
            break;
        }
        case DebrisState::Magnet: {
            record.life -= input.frameDelta;
            if (!(record.life > 0.0f)) {
                collect(record, input, step);
                break;
            }
            if (!input.magnetTarget) break;
            const DebrisVec3 towards = {(*input.magnetTarget)[0] - record.position[0], (*input.magnetTarget)[1] - record.position[1],
                                        (*input.magnetTarget)[2] - record.position[2]};
            if (std::sqrt(towards[0] * towards[0] + towards[1] * towards[1] + towards[2] * towards[2]) <
                kMeasuredStudPickupRadius * kDebrisMagnetReachFactor) {
                collect(record, input, step);
                break;
            }
            for (unsigned a = 0; a < 3; ++a) record.position[a] += towards[a] * kDebrisMagnetStepMeasured;
            place(index, record, step);
            break;
        }
        case DebrisState::Collected: {
            record.life -= input.frameDelta;
            if (!(record.life > 0.0f)) {
                bank(index, record, step, count);
                break;
            }
            if (!(record.flags22 & kDebrisFlagPickable)) break;
            if (input.flightTarget) {
                const float rate = 1.0f / (kStudFlightRate * record.life);
                for (unsigned a = 0; a < 3; ++a) record.position[a] += ((*input.flightTarget)[a] - record.position[a]) * rate;
            }
            place(index, record, step);
            break;
        }
        case DebrisState::Free:
            break;
        }
    }
    return step;
}

}
