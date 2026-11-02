#include "game_level.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>

namespace game_level {
namespace runtime = chase::runtime;

namespace {

std::string lower(std::string text) {
    for (char& c : text) c = char(std::tolower(static_cast<unsigned char>(c)));
    return text;
}

const char* takedownStateName(std::uint16_t state) {
    switch (state) {
    case runtime::kCharacterStateIdle: return "IDLE";
    case runtime::kTakedownMeleePutdown: return "MELEE PUTDOWN";
    case runtime::kTakedownPutdownStun: return "PUTDOWN STUN";
    case runtime::kTakedownAutoArrested: return "AUTO ARRESTED";
    case runtime::kTakedownGenericGetUp: return "GENERIC GETUP";
    case runtime::kTakedownAvoidArrest: return "AVOID ARREST";
    default: return "takedown state";
    }
}

}

// The takedown tables (tables/tablescombat.bxls) and the fight sink of the attack: the fight move's target enters MELEE PUTDOWN.
void LevelSession::loadCharacterTakedowns(chase::assets::FuseArchive& archive) {
    characterTakedowns.clear();
    arrestables = runtime::ArrestableList{};
    if (const chase::assets::FuseEntry* entry = archive.find("tables/tablescombat.bxls"))
        takedownTables = runtime::takedownTables(chase::assets::parseTable(archive.read(*entry)));
    characterFightSink = [this](const runtime::CharacterFightStart& fight) {
        std::cout << "attack: fight move " << fight.move << " on character " << fight.name << " (state " << fight.state << ") at update " << updates << '\n';
        characterTakedownStart(fight);
    };
    // 0x23cee8(event, entity, entity, 255, 0): the first enabled trigger of the event owned by the character starts.
    if (!characterArrestEventSink)
        characterArrestEventSink = [this](const std::string& event, const std::string& character) {
            std::cout << "character " << character << ": event " << event << " at update " << updatesForLog << '\n';
            levelScripts.fireObjectEvent(lower(event), character);
        };
    std::cout << "takedown: " << takedownTables.moves.size() << " takedown moves, " << takedownTables.timings.size() << " timing rows\n";
}

void LevelSession::characterTakedownStart(const runtime::CharacterFightStart& fight) {
    if (!levelCharacters || fight.state != runtime::kTakedownMeleePutdown) {
        std::cout << "character " << fight.name << ": state " << fight.state << " not ported (COMBAT-7)\n";
        return;
    }
    const auto object = std::find_if(level.objects.begin(), level.objects.end(),
                                     [&](const chase::assets::LevelObject& o) { return lower(o.instance) == fight.name; });
    const chase::assets::Level* cls = object == level.objects.end() ? nullptr : classOf(object->className);
    if (!cls || !levelCharacters->world(fight.name)) return;
    if (characterHits.find(fight.name) == characterHits.end())
        characterHits.emplace(fight.name, CharacterHitEntry{runtime::characterHitTargetOf(*object, *cls), {}});
    auto entry = characterTakedowns.find(fight.name);
    if (entry == characterTakedowns.end()) {
        CharacterTakedownEntry created;
        created.character.timing = runtime::takedownTimingOf(*object, *cls);
        entry = characterTakedowns.emplace(fight.name, created).first;
    }
    entry->second.states.startFightMove(fight.move);
    entry->second.startedAt = updates;
}

// STAND-IN COMBAT-S4: the character stays where it stands (no pair motion of 0x1fda20, no mover 0x283b10) and the takedown runs on its own
// machine beside the hit states.
void LevelSession::updateCharacterTakedowns(float dt) {
    if (!levelCharacters || !player) return;
    for (auto& [name, entry] : characterTakedowns) {
        if (!entry.states.active() && !entry.states.pending()) continue;
        if (entry.states.pending() == runtime::kTakedownMeleePutdown && entry.startedAt == updates) continue;
        auto hit = characterHits.find(name);
        if (hit == characterHits.end()) continue;
        entry.character.destroyed = hit->second.target.destroyed || objectState.destroyed.count(name) != 0;
        entry.character.avoidArrestTrigger = enabledObjectTrigger("characteravoidarrest", name);
        const bool finished = levelCharacters->clipFinished(name).value_or(true);
        const std::uint16_t before = entry.states.state();
        // STAND-IN COMBAT-S4: the random words come from the stand-in generator of the attack, not the game's shared generator state.
        const runtime::TakedownStep step = entry.states.update(entry.character, hit->second.target, takedownTables, arrestables, name, finished, dt,
                                                               [this] { return breakRandomStandIn.next(); });
        levelCharacters->holdIdle(name, entry.states.active());
        if (step.unlisted) std::cout << "character " << name << ": not arrestable any more at update " << updatesForLog << '\n';
        if (step.avoidArrest && characterArrestEventSink) characterArrestEventSink("CharacterAvoidArrest", name);
        if (entry.states.state() != before) {
            std::cout << "character " << name << ": state " << entry.states.state() << ' ' << takedownStateName(entry.states.state());
            if (step.notPorted) std::cout << " (not ported, COMBAT-7)";
            std::cout << " at update " << updatesForLog << '\n';
        }
        if (step.clip && player->carryAnimations) levelCharacters->startStateClip(name, player->carryAnimations->name(*step.clip));
        if (step.arrestHealthBefore)
            std::cout << "character " << name << ": arrest health " << *step.arrestHealthBefore << " -> " << unsigned(hit->second.target.arrestHealth)
                      << ", timer " << entry.character.timer << '\n';
        if (step.arrestable) {
            std::cout << "character " << name << ": arrestable (" << arrestables.size() << " listed" << (step.listFull ? ", list full" : "") << ") at update "
                      << updatesForLog << '\n';
            if (characterArrestEventSink) characterArrestEventSink("CharacterArrestable", name);
        }
    }
}

// 0x23cff4(event, entity): an enabled trigger of the event owned by the character.
bool LevelSession::enabledObjectTrigger(const std::string& lowerEvent, const std::string& lowerInstance) const {
    const std::vector<runtime::ScriptTriggerState>& states = levelScripts.triggers();
    for (std::size_t i = 0; i < level.triggers.size() && i < states.size(); ++i)
        if (states[i].enabled && lower(level.triggers[i].event) == lowerEvent && lower(level.triggers[i].objectName) == lowerInstance) return true;
    return false;
}

std::optional<std::uint8_t> LevelSession::characterTakedownTargetMode(const std::string& lowerInstance) const {
    const auto entry = characterTakedowns.find(lowerInstance);
    if (entry == characterTakedowns.end() || !entry->second.states.active()) return std::nullopt;
    return std::uint8_t(entry->second.character.selectByte & 7);
}

// COMBAT-3: the aggro 0x1d1170 of a character the receiver took a hit from a character for. STAND-IN COMBAT-S5: of the target only the side bit
// is known (the AI flags, its target ctrl+0xc4, the counter ctrl+0x72 and the range ctrl+0x80 are not modelled), and the result is only logged: the
// AI pursuit (AI state 2) is not ported (COMBAT-9).
void LevelSession::characterAggro(const std::string& lowerInstance, const runtime::CharacterHitSender& sender) {
    runtime::AggroTarget target;
    const auto character = std::find_if(attackCharacters.begin(), attackCharacters.end(), [&](const AttackCharacter& c) { return c.name == lowerInstance; });
    if (character != attackCharacters.end()) target.badGuy = (character->entity.word & runtime::kEntityWordBadGuy) != 0;
    runtime::AggroSender from;
    from.player = sender.player;
    from.name = sender.player ? "player" : "character";
    const runtime::AggroResult result = runtime::aggroOnHit(target, from);
    if (!result.pursuit && !result.switchTarget && !result.counter) return;
    std::cout << "character " << lowerInstance << ": aggro on " << from.name << (result.pursuit ? " (pursuit call" : " (target switch")
              << ", not ported, COMBAT-9) at update " << updatesForLog << '\n';
}

}
