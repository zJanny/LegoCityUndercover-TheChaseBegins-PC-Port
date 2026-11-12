#include "game_level.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <stdexcept>

namespace game_level {
namespace runtime = chase::runtime;

namespace {

std::string lowerName(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
    return text;
}

}

// STAND-IN COMBAT-S9: the event of a character leaving a vehicle makes it arrestable by an A tap (runtime/arrest_after_crash.hpp).
void LevelSession::characterLeftVehicle(const runtime::CharacterLeftVehicle& event) {
    const std::string name = lowerName(event.character);
    const auto object = std::find_if(level.objects.begin(), level.objects.end(),
                                     [&](const chase::assets::LevelObject& o) { return lowerName(o.instance) == name; });
    const chase::assets::Level* cls = object == level.objects.end() ? nullptr : classOf(object->className);
    if (!cls || !levelCharacters || !levelCharacters->world(name)) throw std::runtime_error("left-vehicle: no level character named " + event.character);
    if (characterHits.find(name) == characterHits.end()) characterHits.emplace(name, CharacterHitEntry{runtime::characterHitTargetOf(*object, *cls), {}});
    if (characterTakedowns.find(name) == characterTakedowns.end()) {
        CharacterTakedownEntry created;
        created.character.timing = runtime::takedownTimingOf(*object, *cls);
        characterTakedowns.emplace(name, created);
    }
    crashArrests.characterLeft({name, lowerName(event.vehicle), event.position});
    std::cout << "arrest: character " << name << " left vehicle " << lowerName(event.vehicle) << ", arrestable at update " << updates << '\n';
}

void LevelSession::applyScriptVehicleExits(unsigned index) {
    unsigned start = 0;
    for (const game_options::ScriptStep& step : script) {
        if (!step.leftVehicleCharacter.empty() && start == index) {
            runtime::CharacterLeftVehicle event{step.leftVehicleCharacter, step.leftVehicleVehicle};
            objectState.destroyed.erase(lowerName(event.character));
            objectState.disabled.erase(lowerName(event.character));
            if (levelCharacters)
                if (const std::optional<chase::assets::Matrix> world = levelCharacters->world(lowerName(event.character)))
                    event.position = {(*world)[12], (*world)[13], (*world)[14]};
            characterLeftVehicle(event);
        }
        start += step.updates;
        if (start > index) break;
    }
}

}
