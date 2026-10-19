#include "runtime/character_death.hpp"
#include "runtime/level_start.hpp"

#include <cstring>

namespace chase::runtime {
namespace {

constexpr std::int32_t kDefaultCharacterEnum = 192;
constexpr std::uint8_t kNotMinifigCharacter = 145;

const assets::LevelParam* param(const assets::LevelObject& object, const assets::Level& cls, const char* name) {
    return effectiveParam(object, cls, assets::nameHash(name));
}

// 0x2451d0: an integer parameter (types 5 and 7 raw, type 0 truncated), else the default.
std::int32_t intParam(const assets::LevelObject& object, const assets::Level& cls, const char* name, std::int32_t fallback) {
    const assets::LevelParam* value = param(object, cls, name);
    if (!value) return fallback;
    if (value->type == 5 || value->type == 7) return std::int32_t(value->value);
    if (value->type == 0) {
        float f = 0;
        std::memcpy(&f, &value->value, 4);
        return std::int32_t(f);
    }
    return fallback;
}

std::optional<std::uint32_t> colourParam(const assets::LevelObject& object, const assets::Level& cls, const char* name) {
    const assets::LevelParam* value = param(object, cls, name);
    if (!value || value->type != 2 || value->blocks.empty() || value->blocks[0].size() < 12) return std::nullopt;
    std::array<float, 3> rgb{};
    std::memcpy(rgb.data(), value->blocks[0].data(), 12);
    return debrisColourOf(rgb);
}

bool characterByteIsMinifig(const assets::LevelObject& object, const assets::Level& cls) {
    return std::uint8_t(intParam(object, cls, "CharacterEnum", kDefaultCharacterEnum)) != kNotMinifigCharacter;
}

}

CharacterDeathSteps characterDeathSteps(const CharacterDeathInput& input) {
    CharacterDeathSteps steps;
    const auto smashes = [&] {
        steps.smashBricks = input.smashColours.has_value();
        steps.dropsBodyParts = true;
        steps.hidden = true;
        steps.deathSound = true;
    };
    if (!input.skipsDeathBranch) {
        switch (input.mode) {
        case CharacterDeathMode::FallApart:
            if (input.minifig) {
                steps.fallApart = true;
                steps.dropsBodyParts = true;
                steps.hidden = true;
            } else {
                smashes();
            }
            break;
        case CharacterDeathMode::Smash: smashes(); break;
        case CharacterDeathMode::Vanish: steps.hidden = true; break;
        case CharacterDeathMode::None: break;
        }
    }
    if (input.player || input.respawnEveryone || (input.friendly && input.respawnIfFriendly)) {
        steps.comesBack = true;
        steps.hidden = false;
        return steps;
    }
    steps.studDrop = input.skipsDeathBranch ? CharacterStudDrop::Thirty : CharacterStudDrop::Dropped;
    steps.hidden = true;
    steps.afterDeathAi = true;
    return steps;
}

CharacterDeathInput characterDeathInput(std::uint8_t kind, const assets::LevelObject& object, const assets::Level& cls) {
    CharacterDeathInput input;
    input.mode = CharacterDeathMode(intParam(object, cls, "DisableFallapart", 0) & 3);
    if (kind == 27 || kind == 149 || intParam(object, cls, "Boss_IsBoss", 0) != 0)
        input.minifig = characterByteIsMinifig(object, cls);
    else if (kind == 121)
        input.minifig = intParam(object, cls, "IsMinifig", 0) != 0;
    else
        input.minifig = false;
    if (const auto first = colourParam(object, cls, "SmashDebrisColour"))
        input.smashColours = std::pair{*first, colourParam(object, cls, "SmashDebrisColour2").value_or(*first)};
    input.friendly = (intParam(object, cls, "AIFriendly", 0) & 1) != 0;
    input.respawnIfFriendly = intParam(object, cls, "AIRespawnIfFriendly", 1) != 0;
    return input;
}

DebrisBurst characterSmashBurst(const std::pair<std::uint32_t, std::uint32_t>& colours, const assets::LevelObject& object, const assets::Level& cls,
                                const DebrisVec3& position) {
    DebrisBurst burst;
    burst.position = position;
    burst.colour = colourParam(object, cls, "DebrisColour").value_or(colours.first);
    burst.secondColour = colourParam(object, cls, "DebrisColour2").value_or(colours.second);
    burst.count = kCharacterSmashBricks;
    if (const assets::LevelParam* amount = param(object, cls, "DebrisAmount"); amount && amount->type == 0) {
        float value = 0;
        std::memcpy(&value, &amount->value, 4);
        if (value > 0.0f) burst.count = unsigned(std::int32_t(value) & 0xff);
    }
    if (const assets::LevelParam* ratio = param(object, cls, "DebrisRatio"); ratio && ratio->type == 0) std::memcpy(&burst.ratio, &ratio->value, 4);
    return burst;
}

}
