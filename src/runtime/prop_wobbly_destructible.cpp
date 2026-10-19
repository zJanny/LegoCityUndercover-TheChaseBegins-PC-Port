#include "runtime/prop_wobbly_destructible.hpp"

#include "assets/object_collision.hpp"
#include "runtime/level_start.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <map>

namespace chase::runtime {
namespace {

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

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

void colourValue(const assets::LevelParam* value, std::uint32_t& colour) {
    if (!value || value->type != 2 || value->blocks.empty() || value->blocks[0].size() < 12) return;
    std::array<float, 3> rgb{};
    std::memcpy(rgb.data(), value->blocks[0].data(), 12);
    colour = debrisColourOf(rgb);
}

}

WobbleHitResult wobblyHit(std::int16_t health, const WobbleHitMessage& message, bool alreadyWobbling) {
    WobbleHitResult result;
    result.health = health;
    if (!message.senderIsPlayer && alreadyWobbling) {
        result.dropped = true;
        return result;
    }
    if (message.damage == 0) {
        result.dropped = true;
        return result;
    }
    result.health = std::int16_t(health - message.damage);
    result.hitEvent = true;
    if (message.senderPresent && !message.senderIsPlayer && message.senderType == kCharacterTypeByte) result.health = 0;
    if (result.health > 0) result.wobble = true;
    else result.destroyed = true;
    return result;
}

void WobblyProps::load(assets::FuseArchive& archive, const assets::Level& level, const assets::ClassLookup& classOf) {
    props_.clear();
    std::map<std::uint32_t, std::string> names;
    for (const assets::LevelObject& object : level.objects) names[assets::nameHash(object.instance)] = lower(object.instance);
    for (const assets::LevelObject& object : level.objects) {
        if (object.className != kWobblyClass) continue;
        const assets::Level* cls = classOf(object.className);
        const std::optional<assets::Placement> placed = assets::objectStart(level, object.instance);
        if (!cls || !placed) continue;
        WobblyProp prop;
        prop.instance = lower(object.instance);
        prop.health = std::int16_t(std::int32_t(floatValue(param(object, *cls, "WobbleHealth"), 0.0f)));
        prop.destroyEffects = propDestroyEffects(object, *cls);
        if (const assets::LevelParam* reference = param(object, *cls, "TriggerObject")) {
            const auto found = names.find(assets::objectReferenceHash(*reference));
            if (found != names.end()) prop.triggerObject = found->second;
        }
        prop.node = assets::placementMatrix(*placed);
        if (!placed->mesh.empty()) {
            float centre[3] = {0, 0, 0}, half[3] = {0, 0, 0};
            if (assets::modelBounds(archive, assets::meshModelPath(placed->mesh), centre, half)) {
                std::copy(centre, centre + 3, prop.boundsCentre.begin());
                std::copy(half, half + 3, prop.boundsHalf.begin());
            }
        }
        const float amount = floatValue(param(object, *cls, "WobbleDebrisAmount"), 0.0f);
        prop.burst.count = amount > 0.0f ? unsigned(std::int32_t(amount) & 0xff) : 2u;
        colourValue(param(object, *cls, "DebrisColour"), prop.burst.colour);
        colourValue(param(object, *cls, "DebrisColour2"), prop.burst.secondColour);
        prop.burst.ratio = floatValue(param(object, *cls, "DebrisRatio"), 0.0f);
        const float studs = floatValue(param(object, *cls, "WobbleStudValue"), 2.0f);
        prop.studValue = unsigned(std::int32_t(studs));
        prop.studSpawn = studSpawnSource(level, object, *cls);
        if (const assets::LevelParam* sound = param(object, *cls, "ATTR_SFX_HIT")) prop.soundHit = std::uint16_t(sound->value);
        if (const assets::LevelParam* sound = param(object, *cls, "ATTR_SFX_DESTROYED")) prop.soundDestroyed = std::uint16_t(sound->value);
        props_.push_back(std::move(prop));
    }
}

const WobblyProp* WobblyProps::find(const std::string& lowerInstance) const {
    for (const WobblyProp& prop : props_)
        if (prop.instance == lowerInstance) return &prop;
    return nullptr;
}

std::optional<WobblyReaction> WobblyProps::deliver(const std::string& lowerInstance, const WobbleHitMessage& message) {
    for (WobblyProp& prop : props_) {
        if (prop.instance != lowerInstance) continue;
        if (prop.destroyed) return std::nullopt;
        WobblyReaction reaction;
        reaction.instance = prop.instance;
        reaction.hit = wobblyHit(prop.health, message, false);
        if (reaction.hit.dropped) return reaction;
        prop.health = reaction.hit.health;
        reaction.soundPosition = {prop.node[12], prop.node[13], prop.node[14]};
        if (reaction.hit.wobble) {
            DebrisBurst burst;
            burst.position = propEffectCentre(prop.boundsCentre, prop.node);
            burst.count = prop.burst.count;
            burst.colour = prop.burst.colour;
            burst.secondColour = prop.burst.secondColour;
            burst.ratio = prop.burst.ratio;
            reaction.wobbleBricks = burst;
            reaction.wobbleStudValue = prop.studValue;
            reaction.wobbleStuds = wobbleHitStudRequest(prop.studSpawn, prop.studValue, prop.node);
            reaction.sound = prop.soundHit;
        } else {
            prop.destroyed = true;
            reaction.triggerObject = prop.triggerObject;
            reaction.sound = prop.soundDestroyed;
        }
        reaction.prop = prop;
        return reaction;
    }
    return std::nullopt;
}

}
