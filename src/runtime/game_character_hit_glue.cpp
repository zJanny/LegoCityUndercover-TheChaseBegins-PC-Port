#include "game_level.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace game_level {
namespace runtime = chase::runtime;

namespace {

bool hitStatePorted(std::uint16_t state) {
    return state == runtime::kCharacterStatePunched || state == runtime::kCharacterStatePunchedRepeat || state == runtime::kCharacterStateKnockedDown ||
           state == runtime::kCharacterStateGetUp;
}

const char* hitStateName(std::uint16_t state) {
    switch (state) {
    case runtime::kCharacterStateIdle: return "IDLE";
    case runtime::kCharacterStateFall: return "FALL";
    case runtime::kCharacterStateGetUp: return "GET UP";
    case runtime::kCharacterStateKnockedDown: return "KNOCKED DOWN";
    case runtime::kCharacterStateKnockBack: return "KNOCK BACK";
    case runtime::kCharacterStateKnockBackCont: return "KNOCK BACK CONT";
    case runtime::kCharacterStatePunched: return "PUNCHED";
    case runtime::kCharacterStatePunchedRepeat: return "PUNCHED REPEAT";
    default: return "not ported";
    }
}

}

void LevelSession::characterHit(const std::string& lowerInstance, const runtime::CharacterHitMessage& hit) {
    if (!levelCharacters) return;
    auto entry = characterHits.find(lowerInstance);
    if (entry == characterHits.end()) {
        const auto object = std::find_if(level.objects.begin(), level.objects.end(),
                                         [&](const chase::assets::LevelObject& o) { return game_doors::lowerCase(o.instance) == lowerInstance; });
        const chase::assets::Level* cls = object == level.objects.end() ? nullptr : classOf(object->className);
        if (!cls || !levelCharacters->world(lowerInstance)) return;
        entry = characterHits.emplace(lowerInstance, CharacterHitEntry{runtime::characterHitTargetOf(*object, *cls), {}}).first;
    }
    runtime::CharacterHitTarget& target = entry->second.target;
    target.disabled = objectState.isDisabled(lowerInstance);
    target.state = entry->second.states.state();
    target.pending = entry->second.states.pending();
    const runtime::CharacterHitResult result = runtime::receiveHit(target, hit);
    std::cout << "character " << lowerInstance << ": hit (kind " << unsigned(hit.kind) << ", damage " << hit.damage << ") at update " << updatesForLog;
    if (result.outcome == runtime::CharacterHitOutcome::Ignored) {
        std::cout << ", ignored\n";
        return;
    }
    std::cout << ", health " << target.health;
    if (result.reaction) std::cout << ", reaction " << *result.reaction << ' ' << hitStateName(*result.reaction);
    // STAND-IN COMBAT-S3: KNOCK BACK, KNOCK BACK CONT, FALL and state 224 are not ported; the character stays in its state.
    if (result.reaction && !hitStatePorted(*result.reaction)) std::cout << " (not ported, stand-in COMBAT-S3)";
    std::cout << '\n';
    if (result.reaction && hitStatePorted(*result.reaction)) entry->second.states.request(*result.reaction);
    target.pending.reset();
    if (result.outcome != runtime::CharacterHitOutcome::Destroyed) return;
    levelScripts.destroyEntity(lowerInstance);
    // The DEAD enter hook 0x19d3b0 of a character that comes back refills ctrl+0x70 from sub +0x128 and +0x122 from +0x123: the port drops the
    // entry, so the next hit starts again from the class values.
    levelCharacters->holdIdle(lowerInstance, false);
    characterHits.erase(entry);
}

void LevelSession::updateCharacterHits() {
    if (!levelCharacters || !player) return;
    for (auto& [instance, entry] : characterHits) {
        if (!entry.states.active() && !entry.states.pending()) continue;
        const bool finished = levelCharacters->clipFinished(instance).value_or(true);
        const runtime::CharacterHitStep step = entry.states.update(finished, entry.target.boss);
        levelCharacters->holdIdle(instance, entry.states.active());
        if (step.clip) {
            std::cout << "character " << instance << ": state " << entry.states.state() << ' ' << hitStateName(entry.states.state()) << " at update "
                      << updatesForLog << '\n';
            if (const auto* animations = player->carryAnimations ? &*player->carryAnimations : nullptr)
                levelCharacters->startStateClip(instance, animations->name(*step.clip));
        }
        if (step.facesSender) {
            // STAND-IN COMBAT-S2: the sender is the player and the turn of 0x205fbc is taken whatever the distance (entity +0x6c not read).
            if (const auto world = levelCharacters->world(instance)) {
                const float dx = player->motion.x - (*world)[12], dz = player->motion.z - (*world)[14];
                const float degrees = std::atan2(dx, dz) * 180.0f / 3.14159265f;
                chase::assets::Placement turned;
                std::copy_n(&(*world)[12], 3, turned.position);
                turned.orientation[1] = degrees;
                levelCharacters->place(instance, chase::assets::placementMatrix(turned));
            }
        }
        if (!entry.states.active() && !entry.states.pending())
            std::cout << "character " << instance << ": state 1 IDLE at update " << updatesForLog << '\n';
    }
}

}
