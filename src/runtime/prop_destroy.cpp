#include "runtime/prop_destroy.hpp"
#include "runtime/level_start.hpp"

#include <cstring>

namespace chase::runtime {
namespace {

const assets::LevelParam* param(const assets::LevelObject& object, const assets::Level& cls, const char* name) {
    return effectiveParam(object, cls, assets::nameHash(name));
}

float floatValue(const assets::LevelParam* value, float fallback) {
    if (!value) return fallback;
    if (value->type == 0) {
        float result = 0;
        std::memcpy(&result, &value->value, 4);
        return result;
    }
    if (value->type == 5 || value->type == 7) return float(std::int32_t(value->value));
    return fallback;
}

std::int32_t intValue(const assets::LevelParam* value, std::int32_t fallback) {
    if (!value) return fallback;
    if (value->type == 5 || value->type == 7) return std::int32_t(value->value);
    if (value->type == 0) return std::int32_t(floatValue(value, 0));
    return fallback;
}

void colourValue(const assets::LevelParam* value, std::uint32_t& colour) {
    if (!value || value->type != 2 || value->blocks.empty() || value->blocks[0].size() < 12) return;
    std::array<float, 3> rgb{};
    std::memcpy(rgb.data(), value->blocks[0].data(), 12);
    colour = debrisColourOf(rgb);
}

}

PropDestroyEffects propDestroyEffects(const assets::LevelObject& object, const assets::Level& cls, DestroyEffectSet set) {
    PropDestroyEffects effects;
    const bool defaultHandler = set == DestroyEffectSet::DefaultHandler;
    effects.explosionParticle = intValue(param(object, cls, "ExplosionParticleDef"), defaultHandler ? kDefaultDestroyExplosion : -1);
    effects.sound = defaultHandler ? kDefaultDestroySound : std::uint16_t(intValue(param(object, cls, "ATTR_SFX_DESTROYED"), 0));
    effects.spawnDebris = intValue(param(object, cls, "SpawnDebris"), 0) != 0;
    const float amount = floatValue(param(object, cls, "DebrisAmount"), 0.0f);
    if (amount > 0.0f) effects.debrisAmount = unsigned(std::int32_t(amount) & 0xff);
    colourValue(param(object, cls, "DebrisColour"), effects.colour);
    colourValue(param(object, cls, "DebrisColour2"), effects.secondColour);
    effects.ratio = floatValue(param(object, cls, "DebrisRatio"), 0.0f);
    effects.respawn = intValue(param(object, cls, "Respawn"), 0) != 0;
    return effects;
}

DebrisVec3 propEffectCentre(const std::array<float, 3>& boxCentre, const std::array<float, 16>& node) {
    DebrisVec3 out{};
    for (unsigned i = 0; i < 3; ++i) out[i] = boxCentre[0] * node[i] + boxCentre[1] * node[4 + i] + boxCentre[2] * node[8 + i] + node[12 + i];
    return out;
}

DebrisVec3 propDebrisPosition(const std::array<float, 3>& boxCentre, const std::array<float, 3>& boxHalf, const std::array<float, 16>& node) {
    DebrisVec3 out = propEffectCentre(boxCentre, node);
    out[1] -= boxHalf[1];
    return out;
}

DebrisBurst propDebrisBurst(const PropDestroyEffects& effects, const DebrisVec3& position) {
    DebrisBurst burst;
    burst.position = position;
    burst.count = effects.debrisAmount;
    burst.colour = effects.colour;
    burst.secondColour = effects.secondColour;
    burst.ratio = effects.ratio;
    return burst;
}

}
