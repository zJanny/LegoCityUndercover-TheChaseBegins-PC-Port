#include "game_level.hpp"
#include <cmath>
#include <iostream>

namespace game_level {
using namespace chase;

namespace {
bool hasLedgeClips(const game_player::PlayerRig& player) {
    return player.animations.count(runtime::kAnimationLedgeIdle) && player.animations.count(runtime::kAnimationLedgeLeft) &&
           player.animations.count(runtime::kAnimationLedgeRight);
}
}

// The player step with the climb bar states around it (docs/re/player-traversal.md): while a bar or corner state owns the player it
// replaces the locomotion; otherwise the climbing-brick step runs and the bar grab test of the jump update follows it.
game_player::PlayerAnimation LevelSession::climbBarMovePlayer(const runtime::StickIntent& intent, bool jumpPressed, bool usePressed, float dt) {
    if (acrobatPoleRider.active()) return acrobatPoleMovePlayer(intent, dt);
    if (fenceVaulter.active()) return fenceVaultMovePlayer(dt);
    if (climbBars.bars.empty() || !hasLedgeClips(*player) || ziplineRider.active()) return moveClimbingPlayer(intent, jumpPressed, usePressed, dt);
    runtime::ClimbBarInput input;
    input.dt = dt;
    input.stickHeld = intent.active;
    input.stickHeading = intent.heading;
    input.bHeld = pad.slot(kSlotJumpButton).value > 0.0f;
    input.aPressed = pad.slot(kSlotButtonA).press > 0;
    if (!ledgeLeftMotion) ledgeLeftMotion = assets::rootMotion(player->animations.at(runtime::kAnimationLedgeLeft));
    if (!ledgeRightMotion) ledgeRightMotion = assets::rootMotion(player->animations.at(runtime::kAnimationLedgeRight));
    input.leftMotion = ledgeLeftMotion ? &*ledgeLeftMotion : nullptr;
    input.rightMotion = ledgeRightMotion ? &*ledgeRightMotion : nullptr;
    input.clipFramesPerSecond = assets::animationFramesPerSecond(player->animations.at(runtime::kAnimationLedgeLeft));
    input.enabled = [this](const runtime::ClimbBar& bar) { return !objectState.isDisabled(bar.instance); };
    const auto readPlayer = [&]() {
        input.position[0] = player->motion.x;
        input.position[1] = player->vertical.y;
        input.position[2] = player->motion.z;
        input.facing = player->motion.facing;
    };
    const auto apply = [&](const runtime::ClimbBarStep& step) -> game_player::PlayerAnimation {
        player->motion.x = step.position[0];
        player->vertical.y = step.position[1];
        player->motion.z = step.position[2];
        player->motion.facing = player->motion.lastIntent = step.facing;
        player->vertical.posture = runtime::Posture::Ground;
        player->vertical.groundRecord = nullptr;
        player->vertical.jump = nullptr;
        player->vertical.fallSpeed = 0.0f;
        player->vertical.fallDistance = 0.0f;
        for (const runtime::ClimbBarSound& sound : step.sounds) player->sounds.push_back({sound.id, player->position(), 0});
        return {step.animation, 1.0f};
    };
    const auto leave = [&](const runtime::ClimbBarStep& step) -> game_player::PlayerAnimation {
        if (step.release == runtime::ClimbBarRelease::Fall) {
            std::cout << "climb bar: drop at update " << updates << '\n';
            player->vertical.posture = runtime::Posture::Falling;
            player->vertical.jump = nullptr;
            player->vertical.fallSpeed = 0.0f;
            player->vertical.fallDistance = 0.0f;
            player->motion.x = step.position[0];
            player->vertical.y = step.position[1];
            player->motion.z = step.position[2];
            return {runtime::kAnimationFall, 1.0f};
        }
        std::cout << "climb bar: jump off at update " << updates << '\n';
        player->motion.x = step.position[0];
        player->vertical.y = step.position[1];
        player->motion.z = step.position[2];
        player->vertical.posture = runtime::Posture::Ground;
        climbBarRider.jumpCount = 0;
        return game_player::updatePlayer(*player, collision, intent, true, dt);
    };
    if (climbBarRider.active()) {
        readPlayer();
        const runtime::ClimbBarStep step = runtime::climbBarUpdate(climbBarRider, climbBars, input);
        const game_player::PlayerAnimation shown = step.riding ? apply(step) : leave(step);
        acrobatPoleHandOver();
        return shown;
    }
    const runtime::Posture before = player->vertical.posture;
    const bool flightBefore = player->vertical.bouncing || player->vertical.bounceSettling;
    const game_player::PlayerAnimation shown = moveClimbingPlayer(intent, jumpPressed, usePressed, dt);
    if (ziplineRider.active() || brickClimber.active()) return shown;
    const bool started = before != runtime::Posture::Jumping && player->vertical.posture == runtime::Posture::Jumping;
    const bool jumpTest = runtime::climbBarJumpTick(climbBarRider, started, before == runtime::Posture::Jumping);
    const bool fallTest = before == runtime::Posture::Falling && player->vertical.posture == runtime::Posture::Falling && !flightBefore;
    if (!jumpTest && !fallTest) return shown;
    readPlayer();
    const int grabbed = runtime::climbBarGrabTest(climbBars, climbBarRider, input.position, input.facing, input.enabled);
    if (grabbed < 0) return shown;
    std::cout << "climb bar: grab " << climbBars.bars[std::size_t(grabbed)].instance << (jumpTest ? "" : " from the fall") << " at update " << updates << '\n';
    const game_player::PlayerAnimation grabbedShown = apply(runtime::climbBarGrab(climbBarRider, climbBars, grabbed, input));
    if (jumpTest) return grabbedShown;
    climbBarRider.jumpLatch = input.bHeld;
    readPlayer();
    const runtime::ClimbBarStep arrival = runtime::climbBarUpdate(climbBarRider, climbBars, input);
    const game_player::PlayerAnimation arrived = arrival.riding ? apply(arrival) : leave(arrival);
    acrobatPoleHandOver();
    return arrived;
}

}
