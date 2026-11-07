#include "game_level.hpp"

#include "runtime/director_hold.hpp"

#include <iostream>
#include <map>

namespace game_level {
namespace runtime = chase::runtime;

namespace {

constexpr const char* kPlayerName = "player1";

std::optional<std::size_t> boundNamed(const chase::assets::Level& level, const std::string& name) {
    if (name.empty()) return std::nullopt;
    const std::string key = game_doors::lowerCase(name);
    for (std::size_t i = 0; i < level.bounds.size(); ++i)
        if (game_doors::lowerCase(level.bounds[i].name) == key) return i;
    return std::nullopt;
}

std::uint8_t aiStateNumber(runtime::AiState state) { return std::uint8_t(state); }

}

// Every placed character with a class file runs the AI controller (0x26be04 -> 0x200c30); the player takes slot 6 of the list 0x58dec8.
void LevelSession::loadAiConduct() {
    std::vector<runtime::AiConductEntity> entities;
    std::size_t bounded = 0;
    if (levelCharacters)
        for (const auto& [name, character] : levelCharacters->characters()) {
            const chase::assets::LevelObject& object = level.objects[character->placement.objectIndex];
            const chase::assets::Level* cls = classOf(object.className);
            if (!cls) continue;
            runtime::AiConductEntity entity;
            entity.name = name;
            entity.params = runtime::aiConductParamsOf(object, *cls);
            entity.rangeBound = boundNamed(level, entity.params.rangeBound);
            entity.aiState = runtime::aiStartRequest(entity.params);
            bounded += entity.rangeBound.has_value();
            entities.push_back(entity);
        }
    runtime::AiConductEntity player;
    player.name = kPlayerName;
    player.player = true;
    entities.push_back(player);
    aiConduct.reset(engagedTable, std::move(entities));
    aiConductUnslottedSightings.clear();
    std::cout << "ai conduct: " << aiConduct.entities().size() - 1 << " characters, " << bounded << " with AIRangeBound\n";
}

// STAND-IN SCRIPT-S326: a character is present while it is enabled, not destroyed and its room is shown, and hidden (0x23de70) when it is not
// present; the player's target mode ctrl+0xda, its flags +0x316 and the locomotion state 82 test of 0x21102c are taken as clear.
void LevelSession::updateAiConduct() {
    if (!levelCharacters || !player) return;
    runtime::AiCore& ai = levelScripts.ai();
    const runtime::DirectorCamera& director = levelScripts.director();
    std::map<std::string, const chase::assets::CharacterObject*> characters;
    for (const auto& [name, object] : levelCharacters->characters()) characters.emplace(name, object);
    for (runtime::AiConductEntity& entity : aiConduct.entities()) {
        if (entity.player) {
            entity.position = {player->motion.x, player->vertical.y, player->motion.z};
            continue;
        }
        const auto found = characters.find(entity.name);
        const chase::assets::CharacterObject* character = found == characters.end() ? nullptr : found->second;
        const std::optional<chase::assets::Matrix> world = levelCharacters->world(entity.name);
        const bool destroyed = objectState.destroyed.count(entity.name) != 0;
        entity.present = world && !destroyed && !objectState.isDisabled(entity.name) &&
                         (!character || roomShown(runtime::roomNumber(character->placement.room)));
        entity.hidden = !entity.present;
        if (world) entity.position = {(*world)[12], (*world)[13], (*world)[14]};
        entity.thinks = !destroyed && runtime::characterControl(director.holdsControl(), false, true, director.objectActiveDuringDirector(entity.name)) ==
                                          runtime::CharacterControl::Ai;
        if (const runtime::AiCharacter* state = ai.character(entity.name)) {
            entity.aiState = aiStateNumber(state->state);
            entity.target = state->combatTarget;
            if (state->millAbout) entity.params.millAbout = *state->millAbout;
            if (state->ignorePlayer) entity.params.ignorePlayer = *state->ignorePlayer;
        }
    }
    aiConduct.update([this](std::size_t bound, const std::array<float, 3>& point) {
        const std::vector<std::uint8_t>& enabled = levelScripts.boundsEnabled();
        return bound < level.bounds.size() && runtime::pointInFallArrestBound(level.bounds[bound], bound < enabled.size() && enabled[bound] != 0, point);
    });
    for (const runtime::AiConductEvent& event : aiConduct.takeEvents()) {
        const bool unslotted = event.kind == runtime::AiConductEventKind::Sighted && !event.combat;
        if (!unslotted) aiConductUnslottedSightings.erase(event.character);
        else if (!aiConductUnslottedSightings.insert(event.character).second) continue;
        std::cout << "ai conduct " << event.character << ": ";
        switch (event.kind) {
        case runtime::AiConductEventKind::Engaged:
        case runtime::AiConductEventKind::Released:
            std::cout << (event.kind == runtime::AiConductEventKind::Engaged ? "engaged" : "released") << " (" << (event.badSide ? "bad" : "good") << " slot "
                      << event.slot << ')';
            break;
        case runtime::AiConductEventKind::Sighted:
            std::cout << "sees " << event.target << " at " << event.distance;
            if (event.combat) {
                ai.startCombat(event.character, event.target);
                std::cout << ", AI state 2 (not ported)";
            } else {
                std::cout << ", no free slot";
            }
            break;
        case runtime::AiConductEventKind::MillAbout:
            ai.run(runtime::AiNativeKind::AIMillAbout, {event.character}, {1.0f});
            std::cout << "mill about";
            break;
        }
        std::cout << " at update " << updatesForLog << '\n';
    }
}

}
