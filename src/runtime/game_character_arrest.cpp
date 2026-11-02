#include "game_level.hpp"
#include "runtime/attack_facts.hpp"

#include <iostream>

namespace game_level {
namespace runtime = chase::runtime;

namespace {

constexpr float kHeadingUnitsPerRadian = 10430.378f;

bool groundLocomotionRole(int role) {
    return role == 0 || role == runtime::kAnimationWalk || role == runtime::kAnimationRun || role == runtime::kAnimationSprint;
}

const char* arrestStateName(std::uint16_t state) {
    switch (state) {
    case runtime::kArrestState: return "ARREST";
    case runtime::kArrestedState: return "ARRESTED";
    default: return "arrest state";
    }
}

}

// The player's clip of ARRESTING (tablesanimations sheet 1 row 314 `cuffing` of class_generic).
void LevelSession::loadCharacterArrest(chase::assets::FuseArchive& archive) {
    arrestClip.reset();
    if (!player || !player->carryAnimations) return;
    const std::string path = std::string(game_player::kCarryAnimationDirectory) + player->carryAnimations->name(runtime::kArrestingClip) +
                             game_player::kPlayerAnimationSuffix;
    const chase::assets::FuseEntry* entry = archive.find(path);
    if (!entry) return;
    if (!player->animations.count(runtime::kArrestingClip)) player->animations[runtime::kArrestingClip] = chase::assets::parseAnimation(archive.read(*entry));
    arrestClip = runtime::makeAttackClip(runtime::kArrestingClip, player->animations.at(runtime::kArrestingClip));
    std::cout << "arrest: clip " << runtime::kArrestingClip << ", " << arrestClip->updates << " updates\n";
}

// 0x1fdbd4 for the A tap after the Y tap: in ground locomotion with nothing carried an A tap on an arrestable character puts the player into
// ARRESTING and asks the character for ARREST. STAND-IN COMBAT-S7: an A tap whose target is not arrestable starts nothing (the grab paths 0x19bfc0,
// 0x1d38fc and 0x16bf8c are not ported).
game_player::PlayerAnimation LevelSession::arrestMovePlayer(const runtime::StickIntent& intent, bool jumpPressed, bool usePressed, bool attackHeld,
                                                            bool arrestHeld, bool attackAllowed, float dt) {
    const bool arrestTapped = arrestTap.update(arrestHeld);
    if (player && playerArrest.active()) {
        attackTap.update(attackHeld);
        std::optional<std::int16_t> heading;
        if (const std::optional<chase::assets::Matrix> world = levelCharacters ? levelCharacters->world(playerArrest.target()) : std::nullopt)
            heading = std::int16_t(std::uint16_t(std::int32_t(runtime::gameAtan2((*world)[12] - player->motion.x, (*world)[14] - player->motion.z) * kHeadingUnitsPerRadian)));
        if (playerArrest.update(player->motion, heading).finished) {
            std::cout << "arrest: player state 1 IDLE at update " << updates << '\n';
            return {0, 1.0f};
        }
        return {runtime::kArrestingClip, 1.0f};
    }
    const game_player::PlayerAnimation shown = attackMovePlayer(intent, jumpPressed, usePressed, attackHeld, attackAllowed, dt);
    if (!player || !arrestTapped || !arrestClip || attack.active() || !attackAllowed || player->vertical.posture != runtime::Posture::Ground ||
        !groundLocomotionRole(shown.role))
        return shown;
    std::vector<runtime::AttackCandidate> candidates;
    appendAttackCharacters(candidates);
    runtime::markAttackNearList(candidates, {player->motion.x, player->vertical.y, player->motion.z});
    const auto arrestState = [this](const std::string& name) -> std::optional<runtime::ArrestCandidate> {
        const auto entry = characterTakedowns.find(name);
        const auto hit = characterHits.find(name);
        if (entry == characterTakedowns.end() || hit == characterHits.end() || entry->second.arrest.active()) return std::nullopt;
        runtime::ArrestCandidate candidate;
        candidate.state = entry->second.states.state();
        candidate.arrestHealth = hit->second.target.arrestHealth;
        candidate.avoidArrestTrigger = enabledObjectTrigger("characteravoidarrest", name);
        const std::uint8_t row = entry->second.character.moveRow;
        if (row < takedownTables.moves.size()) candidate.arrestDamage = takedownTables.moves[row].arrestDamage;
        return candidate;
    };
    const runtime::AttackBody body = runtime::playerAttackBody(player->motion.x, player->vertical.y, player->motion.z, player->motion.facing);
    const std::optional<std::size_t> target =
        runtime::chooseArrestTarget(body, attackPlayer, intent.active ? intent.heading : player->motion.facing, candidates, arrestState);
    if (!target) return shown;
    const std::string& name = candidates[*target].name;
    CharacterTakedownEntry& entry = characterTakedowns.at(name);
    playerArrest.start(name, arrestClip->updates);
    entry.arrest.start();
    entry.arrestStartedAt = updates;
    std::cout << "arrest: player state " << runtime::kArrestingState << " ARRESTING on character " << name << " at update " << updates << '\n';
    return {runtime::kArrestingClip, 1.0f};
}

// The target's ARREST and ARRESTED. STAND-IN COMBAT-S6: the character stays where it stands (no mover 0x215804 in ARREST), the looping clip of
// ARRESTED plays once and holds, CharacterArrested is raised only for the character's own triggers (not the second raise of 0x25ecb4), and of ARRESTED's
// end only the destroyed and disabled bits are applied (the AI request 0x241304 and 0x24ce78 are not).
void LevelSession::updateCharacterArrests(float dt) {
    if (!levelCharacters || !player) return;
    for (auto& [name, entry] : characterTakedowns) {
        if (!entry.arrest.active() || entry.arrest.done()) continue;
        if (entry.arrest.pending() && entry.arrestStartedAt == updates) continue;
        auto hit = characterHits.find(name);
        if (hit == characterHits.end()) continue;
        const bool finished = levelCharacters->clipFinished(name).value_or(true);
        const std::uint16_t before = entry.arrest.state();
        const runtime::ArrestStep step =
            entry.arrest.update(entry.character, hit->second.target, takedownTables, arrestables, entry.states, name, finished, dt);
        levelCharacters->holdIdle(name, true);
        if (step.unlisted) std::cout << "character " << name << ": not arrestable any more at update " << updatesForLog << '\n';
        if (entry.arrest.state() != before)
            std::cout << "character " << name << ": state " << entry.arrest.state() << ' ' << arrestStateName(entry.arrest.state()) << " at update " << updatesForLog << '\n';
        if (step.clip && player->carryAnimations) levelCharacters->startStateClip(name, player->carryAnimations->name(*step.clip));
        if (step.arrested && characterArrestEventSink) characterArrestEventSink("CharacterArrested", name);
        if (step.done) {
            objectState.destroyed.insert(name);
            objectState.disabled.insert(name);
            std::cout << "character " << name << ": arrested (destroyed, disabled) at update " << updatesForLog << '\n';
        }
    }
}

}
