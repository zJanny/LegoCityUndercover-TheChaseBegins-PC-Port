#include "runtime/stud_drop_classes.hpp"

#include "runtime/character_death.hpp"
#include "runtime/level_start.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace chase::runtime {
namespace {

StudSpawnParameters spawnParameters(const StudSpawnSource& source, std::int32_t count, bool forward, bool modeCount, const std::array<float, 16>& node) {
    StudSpawnParameters result;
    result.count = count > 0 ? unsigned(count) : 0u;
    result.forward = forward;
    result.modeCount = modeCount;
    result.spawnCone = source.spawnCone;
    result.spawnDir = source.spawnDir;
    result.spawnLocal = source.spawnLocal;
    result.node = source.hasSpawnLoc ? source.spawnLocNode : node;
    return result;
}

std::int32_t saturatingCast(float value) {
    if (std::isnan(value)) return 0;
    if (value >= 2147483648.0f) return std::numeric_limits<std::int32_t>::max();
    if (value <= -2147483648.0f) return std::numeric_limits<std::int32_t>::min();
    return std::int32_t(value);
}

unsigned digitSum(unsigned value) { return value / 100 + (value % 100) / 10 + value % 10; }

unsigned lastSlotWhere(const std::array<HitSequenceSlot, 4>& slots, bool (*used)(const HitSequenceSlot&)) {
    unsigned last = 0;
    for (unsigned index = 1; index < slots.size(); ++index)
        if (used(slots[index])) last = index;
    return last;
}

bool frameInUpdate(std::uint8_t frame, float clipFrame) {
    if (frame == 255) return false;
    const float start = float(frame);
    return start <= clipFrame && start + 1.0f > clipFrame;
}

}

bool runsGenericStudStateMachine(std::uint8_t kind) {
    return std::find(kGenericStudStateKinds.begin(), kGenericStudStateKinds.end(), kind) != kGenericStudStateKinds.end();
}

bool genericStateChangeDropsStuds(std::uint16_t targetState) { return targetState == 2 || targetState == 3; }

std::int32_t studValueParameter(const assets::LevelObject& object, const assets::Level& cls, std::int32_t fallback) {
    const assets::LevelParam* value = effectiveParam(object, cls, assets::nameHash("StudValue"));
    if (!value) return fallback;
    if (value->type == 0) {
        float result = 0;
        std::memcpy(&result, &value->value, 4);
        return saturatingCast(result);
    }
    if (value->type == 5 || value->type == 7) return std::int32_t(value->value);
    return fallback;
}

std::optional<StudDropRequest> genericStateStudRequest(const StudSpawnSource& source, std::int32_t studValue, const std::array<float, 16>& node) {
    return studSpawnRequest(spawnParameters(source, studValue, true, false, node));
}

const StudRewardSite* studRewardSite(std::uint8_t kind) {
    for (const StudRewardSite& site : kStudRewardSites)
        if (site.kind == kind) return &site;
    return nullptr;
}

std::optional<StudDropRequest> studRewardRequest(const StudRewardSite& site, const StudSpawnSource& source, std::int32_t studValue, const std::array<float, 16>& node) {
    return studSpawnRequest(spawnParameters(source, studValue, site.forward, false, node));
}

StudDropRequest playerHurtStudRequest(const DebrisVec3& nodePosition) {
    return *characterStudDropRequest(CharacterStudDrop::Thirty, nodePosition);
}

PlayerDeathStudLoss playerDeathStudLoss(std::uint64_t studCounter, const DebrisVec3& nodePosition) {
    PlayerDeathStudLoss loss;
    const std::uint64_t quotient = studCounter / 10;
    unsigned count = quotient >= kPlayerDeathStudLimit ? kPlayerDeathStudLimit : unsigned(quotient);
    while (count != kPlayerDeathStudLimit && digitSum(count) > kPlayerDeathDigitSumLimit) count -= count % 10 != 0 ? 1 : 10;
    loss.count = count;
    loss.counterAfter = studCounter - std::uint64_t(count) * kPlayerDeathUnitValue;
    if (count != 0) {
        StudDropRequest request;
        request.mode = StudDropMode::Count;
        request.count = count;
        request.position = nodePosition;
        request.skip = kStudDropDefaultSkip;
        loss.request = request;
    }
    return loss;
}

std::optional<StudDropRequest> hitSequenceStudRequest(const HitSequenceStudInput& input, const StudSpawnSource& source, const std::array<float, 16>& node) {
    if (!input.hasClip) return std::nullopt;
    const unsigned frameSlot = std::min<unsigned>(input.counter, lastSlotWhere(input.slots, [](const HitSequenceSlot& slot) { return slot.frame != 255; }));
    const unsigned valueSlot = std::min<unsigned>(input.counter, lastSlotWhere(input.slots, [](const HitSequenceSlot& slot) { return slot.value != 0; }));
    if (!frameInUpdate(input.slots[frameSlot].frame, input.clipFrame) || input.health == 1) return std::nullopt;
    const std::uint8_t value = input.slots[valueSlot].value;
    if (value == 0) return std::nullopt;
    const bool forward = input.boxHalfX > input.boxHalfZ * 2.0f;
    return studSpawnRequest(spawnParameters(source, value, forward, true, node));
}

}
