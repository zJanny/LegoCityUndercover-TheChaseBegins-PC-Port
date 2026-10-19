#include "runtime/stud_callers.hpp"

#include "runtime/level_start.hpp"

#include <cstring>

namespace chase::runtime {
namespace {

std::int32_t intValue(const assets::LevelParam* value, std::int32_t fallback) {
    if (!value) return fallback;
    if (value->type == 0) {
        float result = 0;
        std::memcpy(&result, &value->value, 4);
        return std::int32_t(result);
    }
    return std::int32_t(value->value);
}

StudSpawnParameters parameters(const StudSpawnSource& source, unsigned count, bool modeCount, const std::array<float, 16>& node) {
    StudSpawnParameters result;
    result.count = count;
    result.forward = true;
    result.modeCount = modeCount;
    result.spawnCone = source.spawnCone;
    result.spawnDir = source.spawnDir;
    result.spawnLocal = source.spawnLocal;
    result.node = node;
    return result;
}

}

StudSpawnSource studSpawnSource(const assets::Level& level, const assets::LevelObject& object, const assets::Level& cls) {
    StudSpawnSource source;
    if (const assets::LevelParam* cone = effectiveParam(object, cls, assets::nameHash("SpawnCone"))) {
        if (cone->type == 0) std::memcpy(&source.spawnCone, &cone->value, 4);
        else source.spawnCone = float(std::int32_t(cone->value));
    }
    source.spawnDir = intValue(effectiveParam(object, cls, assets::nameHash("SpawnDir")), 999);
    source.spawnLocal = intValue(effectiveParam(object, cls, assets::nameHash("SpawnLocal")), 0);
    if (const assets::LevelParam* location = effectiveParam(object, cls, assets::nameHash("SpawnLoc"))) {
        const std::uint32_t target = assets::objectReferenceHash(*location);
        for (const assets::LevelObject& other : level.objects) {
            if (assets::nameHash(other.instance) != target) continue;
            if (const std::optional<assets::Placement> placed = assets::objectStart(level, other.instance)) {
                source.hasSpawnLoc = true;
                source.spawnLocNode = assets::placementMatrix(*placed);
            }
            break;
        }
    }
    return source;
}

std::optional<StudDropRequest> wobbleHitStudRequest(const StudSpawnSource& source, unsigned wobbleStudValue, const std::array<float, 16>& node) {
    return studSpawnRequest(parameters(source, wobbleStudValue, true, source.hasSpawnLoc ? source.spawnLocNode : node));
}

std::optional<StudDropRequest> builditStudRequest(const StudSpawnSource& source, float studValue, const std::array<float, 16>& finalPropNode) {
    const std::int32_t count = std::int32_t(studValue);
    return studSpawnRequest(parameters(source, count > 0 ? unsigned(count) : 0u, false, finalPropNode));
}

}
