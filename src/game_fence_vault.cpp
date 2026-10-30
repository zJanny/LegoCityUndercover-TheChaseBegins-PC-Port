#include "game_level.hpp"
#include <iostream>

namespace game_level {
using namespace chase;

// The B press gate of 0x20b984 in front of the jump (docs/re/player-traversal-fence-vault.md): a fence whose zone holds the player on the ground takes
// the press and starts USE LERP and the vault; otherwise the push and the usual step run. TRAV-S38: the gate is the port's jump press on the ground
// (not the states 1..3, 0x23a70c and *(ctrl+0xe0)+0x130 bit 2), only the fences of the list 0x58ea1c are asked, and the player's posture is Falling
// while the sequence owns him.
game_player::PlayerAnimation LevelSession::movePlayerWithVault(const runtime::StickIntent& intent, bool jumpPressed, float dt) {
    if (fenceVaults.empty() || !jumpPressed || player->vertical.posture != runtime::Posture::Ground || ziplineRider.active() || pushGrip.active ||
        !player->animations.count(runtime::kAnimationVault) || !player->animations.count(runtime::kAnimationVaultFence))
        return movePlayerWithPush(intent, jumpPressed, dt);
    const float position[3] = {player->motion.x, player->vertical.y, player->motion.z};
    const auto disabled = [this](const runtime::FenceVaultObject& fence) {
        return objectState.isDisabled(fence.instance) || objectState.isHidden(fence.instance);
    };
    const int taken = runtime::fenceVaultOffer(fenceVaults, position, player->motion.facing, disabled);
    if (taken < 0) return movePlayerWithPush(intent, jumpPressed, dt);
    std::cout << "fence vault: " << fenceVaults[std::size_t(taken)].instance << " takes the B press at update " << updates << '\n';
    const runtime::FenceVaultStep step = fenceVaulter.start(fenceVaults, std::size_t(taken), position, player->motion.facing, dt);
    player->motion.x = step.position[0];
    player->vertical.y = step.position[1];
    player->motion.z = step.position[2];
    player->motion.facing = player->motion.lastIntent = step.facing;
    player->vertical.posture = runtime::Posture::Falling;
    player->vertical.jump = nullptr;
    player->vertical.fallSpeed = 0.0f;
    player->vertical.fallDistance = 0.0f;
    std::cout << "fence vault: player state " << *step.entered << " at update " << updates << '\n';
    return {step.animation, 1.0f};
}

// USE LERP and state 326 own the player while they run.
game_player::PlayerAnimation LevelSession::fenceVaultMovePlayer(float dt) {
    const int role = runtime::fenceVaultClip(fenceVaults[fenceVaulter.fence()]);
    const assets::Animation& clip = player->animations.at(role);
    if (!vaultMotions.count(role))
        if (const std::optional<assets::RootMotion> motion = assets::rootMotion(clip)) vaultMotions[role] = *motion;
    const auto motion = vaultMotions.find(role);
    const runtime::FenceVaultClipData data{motion == vaultMotions.end() ? nullptr : &motion->second, assets::animationFramesPerSecond(clip),
                                              float(assets::animationLength(clip)) - 1.0f};
    const runtime::FenceVaultStep step = fenceVaulter.update(fenceVaults, data, dt);
    player->motion.x = step.position[0];
    player->vertical.y = step.position[1];
    player->motion.z = step.position[2];
    player->motion.facing = player->motion.lastIntent = step.facing;
    player->vertical.posture = runtime::Posture::Falling;
    player->vertical.jump = nullptr;
    player->vertical.fallDistance = 0.0f;
    player->vertical.fallSpeed = step.fallSpeed;
    if (step.entered) std::cout << "fence vault: player state " << *step.entered << " at update " << updates << '\n';
    if (step.ended) {
        std::cout << "fence vault: fall at " << step.position[0] << ' ' << step.position[1] << ' ' << step.position[2] << " at update " << updates << '\n';
        return {runtime::kAnimationFall, 1.0f};
    }
    return {step.animation, 1.0f};
}

}
