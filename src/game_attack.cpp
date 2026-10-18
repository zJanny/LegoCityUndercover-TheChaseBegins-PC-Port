#include "game_level.hpp"
#include "runtime/player_motion.hpp"
#include <cctype>
#include <iostream>

namespace game_level {
using namespace chase;

namespace {
std::string lowerCase(std::string text) {
    for (char& c : text) c = char(std::tolower(static_cast<unsigned char>(c)));
    return text;
}

bool groundLocomotionRole(int role) {
    return role == 0 || role == runtime::kAnimationWalk || role == runtime::kAnimationRun || role == runtime::kAnimationSprint;
}
}

// The combo and running clips of the attack (class_generic, tablesanimations sheet 1 rows 0x7a..0x7e) and the wobbly props it can hit.
void LevelSession::loadAttack(assets::FuseArchive& archive, const std::vector<assets::ObjectCollision>& placed) {
    attackTargets = runtime::attackTargetObjects(placed);
    attackClips.clips.clear();
    if (!player || !player->carryAnimations) return;
    for (int id = runtime::kAttackFirstComboClip; id <= runtime::kAttackRunningClip; ++id) {
        const std::string path = std::string(game_player::kCarryAnimationDirectory) + player->carryAnimations->name(id) + game_player::kPlayerAnimationSuffix;
        const assets::FuseEntry* entry = archive.find(path);
        if (!entry) continue;
        if (!player->animations.count(id)) player->animations[id] = assets::parseAnimation(archive.read(*entry));
        attackClips.clips.push_back(runtime::makeAttackClip(id, player->animations.at(id)));
    }
    std::cout << "attack: " << attackClips.clips.size() << " clips, " << attackTargets.size() << " targets\n";
}

// The player step with the attack on foot (docs/re/player-attack.md): a Y tap in ground locomotion enters the attack state after the locomotion
// step of the same update; while it runs the attack moves the player by its root step and collects its messages for their targets.
game_player::PlayerAnimation LevelSession::attackMovePlayer(const runtime::StickIntent& intent, bool jumpPressed, bool usePressed, bool attackHeld,
                                                            bool attackAllowed, float dt) {
    const bool tapped = attackTap.update(attackHeld);
    attackMessages.clear();
    if (!player || attackClips.clips.empty()) return climbBarMovePlayer(intent, jumpPressed, usePressed, dt);
    std::vector<runtime::AttackCandidate> candidates;
    candidates.reserve(attackTargets.size());
    for (const runtime::AttackTargetObject& object : attackTargets)
        candidates.push_back({object.body, !objectState.isDisabled(object.instance) && objectState.collisionEnabled(object.instance, object.collisionDefault)});
    runtime::AttackInput input;
    input.tap = tapped;
    input.intent = intent;
    input.candidates = &candidates;
    const auto readPosition = [&]() {
        input.x = player->motion.x;
        input.y = player->vertical.y;
        input.z = player->motion.z;
    };
    std::optional<game_player::PlayerAnimation> shown;
    if (!attack.active()) {
        shown = climbBarMovePlayer(intent, jumpPressed, usePressed, dt);
        if (!tapped) return *shown;
        input.allowed = attackAllowed && player->vertical.posture == runtime::Posture::Ground && groundLocomotionRole(shown->role);
        input.running = intent.active && (shown->role == runtime::kAnimationRun || shown->role == runtime::kAnimationSprint);
    }
    readPosition();
    const runtime::AttackStep step = attack.update(input, player->motion, attackClips);
    for (const runtime::AttackMessage& message : step.messages) {
        const std::string instance = lowerCase(attackTargets[message.target].instance);
        attackMessages.emplace_back(instance, message);
        if (message.id == runtime::kAttackMessageHit)
            std::cout << "attack: hit " << instance << " (message 0, damage " << message.damage << ") at update " << updates << '\n';
        else
            std::cout << "attack: message " << message.id << " to " << instance << " at update " << updates << '\n';
    }
    if (step.started) std::cout << "attack: state " << step.state << " clip " << step.clip << " at update " << updates << '\n';
    if (step.finished) {
        std::cout << "attack: ended at update " << updates << '\n';
        return climbBarMovePlayer(intent, jumpPressed, usePressed, dt);
    }
    if (!step.active) return shown ? *shown : climbBarMovePlayer(intent, jumpPressed, usePressed, dt);
    if (shown) return {step.clip, 1.0f};
    runtime::playerRootStepUpdate(player->motion, player->vertical, collision, step.deltaX, step.deltaZ, dt);
    return {step.clip, 1.0f};
}

}
