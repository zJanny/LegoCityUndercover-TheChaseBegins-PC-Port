#include "game_level.hpp"
#include <cmath>
#include <iostream>

namespace game_level {
using namespace chase;

namespace {
bool hasAcrobatPoleClips(const game_player::PlayerRig& player) {
    return player.animations.count(runtime::kAnimationAcrobatPoleIdle) && player.animations.count(runtime::kAnimationAcrobatPoleFlip) &&
           player.animations.count(runtime::kAnimationAcrobatPoleJump);
}
}

// The arrival on an acrobat pole is done (state 88 requests 93): the acrobat pole states take the player over (docs/re/player-traversal-rails.md).
bool LevelSession::acrobatPoleHandOver() {
    if (climbBarRider.phase != runtime::ClimbPhase::Idle || !climbBarRider.enteredThisUpdate || climbBarRider.bar < 0) return false;
    if (!climbBars.bars[std::size_t(climbBarRider.bar)].acrobatPole || !hasAcrobatPoleClips(*player)) return false;
    if (!acrobatPoleClips)
        acrobatPoleClips = runtime::acrobatPoleClips(player->animations.at(runtime::kAnimationAcrobatPoleIdle),
                                                     player->animations.at(runtime::kAnimationAcrobatPoleFlip),
                                                     player->animations.at(runtime::kAnimationAcrobatPoleJump));
    runtime::acrobatPoleStart(acrobatPoleRider, climbBarRider.bar);
    climbBarRider.phase = runtime::ClimbPhase::None;
    climbBarRider.enteredThisUpdate = false;
    return true;
}

game_player::PlayerAnimation LevelSession::acrobatPoleMovePlayer(const runtime::StickIntent& intent, float dt) {
    runtime::AcrobatPoleInput input;
    input.dt = dt;
    input.position[0] = player->motion.x;
    input.position[1] = player->vertical.y;
    input.position[2] = player->motion.z;
    input.facing = player->motion.facing;
    input.bHeld = pad.slot(kSlotJumpButton).value > 0.0f;
    input.stickHeld = intent.active;
    input.stickHeading = intent.heading;
    input.cameraYaw = follow ? follow->yaw() : 0.0f;
    input.linkEnabled = [this](const runtime::ClimbBarLink& link) { return !objectState.isDisabled(link.instance); };
    input.ground = [&](const float position[3], float reach) {
        runtime::GroundProbe probe;
        probe.x = position[0];
        probe.y = position[1];
        probe.z = position[2];
        probe.forwardX = std::sin(runtime::headingRadians(player->motion.facing));
        probe.forwardZ = std::cos(runtime::headingRadians(player->motion.facing));
        probe.distance = reach;
        return runtime::groundHeight(collision, probe);
    };
    const int before = acrobatPoleRider.state;
    const runtime::AcrobatPoleStep step = runtime::acrobatPoleUpdate(acrobatPoleRider, climbBars, *acrobatPoleClips, input);
    for (const runtime::ClimbBarSound& sound : step.sounds) player->sounds.push_back({sound.id, player->position(), 0});
    player->motion.x = step.position[0];
    player->vertical.y = step.position[1];
    player->motion.z = step.position[2];
    player->motion.facing = player->motion.lastIntent = step.facing;
    player->vertical.jump = nullptr;
    player->vertical.fallSpeed = 0.0f;
    player->vertical.fallDistance = 0.0f;
    if (!step.riding) {
        std::cout << "acrobat pole: fall at update " << updates << '\n';
        climbBarRider.ignoreBar = acrobatPoleRider.from;  // ctrl+0x138 as for a plain bar left (TRAV-29)
        player->vertical.posture = runtime::Posture::Falling;
        return {runtime::kAnimationFall, 1.0f};
    }
    if (step.state != before) std::cout << "acrobat pole: state " << step.state << " at update " << updates << '\n';
    player->vertical.posture = runtime::Posture::Ground;
    player->vertical.groundRecord = nullptr;
    return {step.animation, 1.0f};
}

}
