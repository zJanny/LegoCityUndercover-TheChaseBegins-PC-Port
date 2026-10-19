#include "game_level.hpp"
#include <cmath>
#include <iostream>

namespace game_level {
using namespace chase;

namespace {
unsigned clipUpdates(const assets::Animation& clip) {
    return unsigned(std::ceil(float(assets::animationLength(clip)) * 30.0f / assets::animationFramesPerSecond(clip) - 1e-3f));
}

bool hasBrickClips(const game_player::PlayerRig& player) {
    for (const int role : runtime::kBrickGrabAnimations)
        if (!player.animations.count(role)) return false;
    return true;
}
}

// The player step with the climbing bricks (docs/re/player-traversal.md): the brick update 0x1a649c, then the brick states 141..152 own
// the player while they run; otherwise the ladder and pole step (movePlayerWithLadder) runs and, in a jump update that runs the grab tests, the brick test 0x273f6c follows
// the line test.
game_player::PlayerAnimation LevelSession::moveClimbingPlayer(const runtime::StickIntent& intent, bool jumpPressed, bool usePressed, float dt) {
    if (grabBricks.empty() || !hasBrickClips(*player)) return movePlayerWithLadder(intent, jumpPressed, dt);
    runtime::updateGrabBricks(grabBricks);
    runtime::BrickInput input;
    input.dt = dt;
    input.position[0] = player->motion.x;
    input.position[1] = player->vertical.y;
    input.position[2] = player->motion.z;
    input.facing = player->motion.facing;
    input.stickHeld = (intent.heldSet & runtime::kHeldMove) != 0;
    input.stickHeading = intent.heading;
    input.cameraYaw = follow->yaw();
    // TRAV-S13 stand-in: the camera matrix row 2 that 0x31a49c reads, built from the follow camera's yaw and pitch.
    input.cameraForward[0] = std::sin(follow->yaw()) * std::cos(follow->pitch());
    input.cameraForward[1] = -std::sin(follow->pitch());
    input.cameraForward[2] = std::cos(follow->yaw()) * std::cos(follow->pitch());
    input.jumpPressed = jumpPressed;
    input.usePressed = usePressed;
    input.enabled = [this](const runtime::GrabBrick& brick) { return !objectState.isDisabled(brick.instance); };
    for (const int role : runtime::kBrickGrabAnimations) {
        const assets::Animation& clip = player->animations.at(role);
        if (!brickMotions.count(role))
            if (const std::optional<assets::RootMotion> motion = assets::rootMotion(clip)) brickMotions[role] = *motion;
        const auto motion = brickMotions.find(role);
        input.clips[role] = {clipUpdates(clip), motion == brickMotions.end() ? nullptr : &motion->second, assets::animationFramesPerSecond(clip)};
    }
    const auto apply = [&](const runtime::BrickStep& step) -> game_player::PlayerAnimation {
        player->motion.x = step.position[0];
        player->vertical.y = step.position[1];
        player->motion.z = step.position[2];
        player->motion.facing = player->motion.lastIntent = step.facing;
        player->vertical.jump = nullptr;
        if (step.entered != runtime::BrickState::None)
            std::cout << "brick grab: state " << int(step.entered) << ' ' << grabBricks[std::size_t(brickClimber.target)].instance << " at update "
                      << updates << '\n';
        if (step.exit == runtime::BrickExit::Jump) {
            std::cout << "brick grab: jump at update " << updates << '\n';
            player->vertical.posture = runtime::Posture::Ground;
            const game_player::PlayerAnimation shown = movePlayerWithLadder(intent, true, dt);
            runtime::brickJumpTick(brickClimber, true, false);
            return shown;
        }
        player->vertical.posture = runtime::Posture::Falling;
        player->vertical.fallSpeed = step.fallSpeed;
        player->vertical.fallDistance = 0.0f;
        if (step.exit == runtime::BrickExit::Fall) {
            std::cout << "brick grab: fall at update " << updates << '\n';
            return {runtime::kAnimationFall, 1.0f};
        }
        return {step.animation, 1.0f};
    };
    if (brickClimber.active()) return apply(runtime::brickGrabUpdate(brickClimber, grabBricks, input));
    const runtime::Posture before = player->vertical.posture;
    const game_player::PlayerAnimation shown = movePlayerWithLadder(intent, jumpPressed, dt);
    const bool jumping = player->vertical.posture == runtime::Posture::Jumping;
    const bool started = before != runtime::Posture::Jumping && jumping;
    if (!runtime::brickJumpTick(brickClimber, started, before == runtime::Posture::Jumping) || !jumping || carry.carryingMode() != 0) return shown;
    const float position[3] = {player->motion.x, player->vertical.y, player->motion.z};
    const int grabbed = runtime::brickGrabTest(grabBricks, position, player->motion.facing, input.enabled);
    if (grabbed < 0) return shown;
    return apply(runtime::brickGrabEnter(brickClimber, grabBricks, grabbed));
}

}
