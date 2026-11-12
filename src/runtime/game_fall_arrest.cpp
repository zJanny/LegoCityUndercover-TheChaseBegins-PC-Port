#include "game_level.hpp"

#include <algorithm>
#include <iostream>

namespace game_level {
namespace runtime = chase::runtime;

// STAND-IN SCRIPT-S328: a character joins the allegiance lists when it moves under the AI (the AIMove front end 0x22d85c creates the path record at
// the call; the port takes the first update that moved it) or at the aggro; the airborne flag ctrl+0x314 bit 6 is written by the character mover
// (game_character_air.cpp) only for the characters in the grab and takedown states, every other character stays "on the ground"; the sound's owner id
// is 0 (entity +0x14 is not read); the update runs after the particle objects, an order the port chose.
void LevelSession::updateFallArrest(const std::function<void(const runtime::SoundCall&)>& emit) {
    if (!levelCharacters) return;
    const runtime::AiCore& ai = levelScripts.ai();
    for (const AttackCharacter& character : attackCharacters) {
        const bool badGuy = (character.entity.word & runtime::kEntityWordBadGuy) != 0;
        if (objectState.isDisabled(character.name)) {
            engagedTable.release(character.name, badGuy, (character.entity.word & runtime::kEntityWordBoss) != 0, false);
            continue;
        }
        if (ai.movedLastUpdate(character.name)) engagedTable.engage(character.name, badGuy);
    }
    const auto describe = [this](const std::string& name) -> std::optional<runtime::FallArrestCharacter> {
        const std::optional<chase::assets::Matrix> world = levelCharacters->world(name);
        if (!world) return std::nullopt;
        runtime::FallArrestCharacter described;
        described.enabled = !objectState.isDisabled(name);
        described.position = {(*world)[12], (*world)[13], (*world)[14]};
        return described;
    };
    for (const runtime::FallArrestHit& hit : engagedTable.fallArrestUpdate(level, fallArrestBoundList, levelScripts.boundsEnabled(), describe)) {
        std::cout << "character " << hit.character << ": falls into fall arrest bound " << level.bounds[hit.bound].name << ", auto arrested at update "
                  << updatesForLog << '\n';
        const auto found = std::find_if(attackCharacters.begin(), attackCharacters.end(), [&](const AttackCharacter& c) { return c.name == hit.character; });
        const std::optional<chase::assets::Matrix> world = levelCharacters->world(hit.character);
        if (found == attackCharacters.end() || !world || found->fallArrestSound == 0) continue;
        emit({found->fallArrestSound, {(*world)[12], (*world)[13], (*world)[14]}, 0});
    }
}

}
