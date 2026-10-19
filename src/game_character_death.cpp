#include "game_level.hpp"
#include "runtime/character_death.hpp"
#include "runtime/character_respawn.hpp"

#include <algorithm>
#include <iostream>
#include <string>

namespace game_level {
namespace runtime = chase::runtime;

bool LevelSession::characterDeathMessage(const std::string& lowerInstance) {
    const auto kind = levelScripts.objectNatives().kindOf(lowerInstance);
    if (!kind || runtime::destroyEffectSet(*kind) != runtime::DestroyEffectSet::CharacterFallApart) return false;
    const auto object = std::find_if(level.objects.begin(), level.objects.end(),
                                     [&](const chase::assets::LevelObject& o) { return game_doors::lowerCase(o.instance) == lowerInstance; });
    const chase::assets::Level* cls = object == level.objects.end() ? nullptr : classOf(object->className);
    if (!cls) return true;
    const runtime::CharacterDeathInput input = runtime::characterDeathInput(*kind, *object, *cls);
    const runtime::CharacterDeathSteps steps = runtime::characterDeathSteps(input);
    const auto character = std::find_if(drawable.characters.begin(), drawable.characters.end(),
                                        [&](const chase::assets::CharacterObject& c) { return game_doors::lowerCase(c.placement.instance) == lowerInstance; });
    const auto pose = levelCharacters ? levelCharacters->bodyPose(lowerInstance) : std::nullopt;
    const runtime::FallApartMatrix world = pose ? pose->world : character != drawable.characters.end() ? chase::assets::placementMatrix(character->placement)
                                                                                                          : runtime::FallApartMatrix{};
    const std::array<float, 3> position{world[12], world[13], world[14]};
    bool fell = false;
    if (steps.fallApart && character != drawable.characters.end()) {
        fell = fallApart.start(scene, shared.archive,
                               {lowerInstance, character->appearance, character->hat, world, pose ? pose->head : std::nullopt, pose ? pose->hat : std::nullopt});
        if (fell) playPropSound(runtime::kFallApartSound, position);  // STAND-IN SCRIPT-S264: at the node position (0x23fb78 not read)
    }
    // STAND-IN SCRIPT-S264: the bricks start at the node position; the character's entity box (+0x60 / +0x70) is not known.
    if (steps.smashBricks && effects.debris())
        effects.debris()->burst(runtime::characterSmashBurst(*input.smashColours, *object, *cls, position), debrisLineStandIn(), debrisLineStandIn());
    std::string studLog;
    if (const auto drop = runtime::characterStudDropRequest(steps.studDrop, {position[0], position[1], position[2]}); drop && studDrops.field()) {
        const runtime::StudDropMode mode = studDrops.field()->drop(*drop, debrisLineStandIn(), debrisLineStandIn());
        studLog = ", stud drop mode " + std::to_string(unsigned(mode)) + " count " + std::to_string(drop->count);
    }
    std::string afterLog;
    if (steps.afterDeathAi) {
        const runtime::CharacterAfterDeath after = runtime::characterAfterDeath({input.friendly, input.millAbout || input.respawnEveryone, false});
        if (after == runtime::CharacterAfterDeath::BackToSpawn && levelCharacters && character != drawable.characters.end())
            levelCharacters->place(lowerInstance, chase::assets::placementMatrix(character->placement));
        afterLog = after == runtime::CharacterAfterDeath::BackToSpawn ? ", back to its spawn place"
                   : after == runtime::CharacterAfterDeath::MillAbout ? ", mill-about hand-over (not ported, SCRIPT-328)"
                                                                        : ", leaves the engaged table (not ported, SCRIPT-328)";
    }
    std::cout << "character " << lowerInstance << ": dies at update " << updatesForLog << (fell ? ", falls apart" : "") << (steps.smashBricks ? ", bricks" : "")
              << studLog << afterLog << (steps.comesBack ? ", comes back (not shown, SCRIPT-326)" : "")
              << " (stand-in SCRIPT-S264: no character sound or dropped weapons)\n";
    return true;
}

}
