#include "game_level.hpp"
#include <iostream>

namespace game_level {
using namespace chase;

namespace {
bool hasPoleClips(const game_player::PlayerRig& player) {
    for (const int role : {runtime::kAnimationPoleIdle, runtime::kAnimationPoleUp, runtime::kAnimationPoleDown, runtime::kAnimationPoleDismount})
        if (!player.animations.count(role)) return false;
    return true;
}
}

// The pole states around the player step: the A press and the walk-in (docs/re/player-traversal.md) start the mount, the pole states own
// the player while they run.
game_player::PlayerAnimation LevelSession::movePlayerWithPole(const runtime::StickIntent& intent, bool jumpPressed, float dt) {
    if (pole.poles.empty() || !hasPoleClips(*player) || ziplineRider.active()) return movePlayerWithVault(intent, jumpPressed, dt);
    const auto clipOf = [&](int role) { return assets::rootMotion(player->animations.at(role)); };
    if (!pole.up) pole.up = clipOf(runtime::kAnimationPoleUp);
    if (!pole.down) pole.down = clipOf(runtime::kAnimationPoleDown);
    if (!pole.dismount) pole.dismount = clipOf(runtime::kAnimationPoleDismount);
    const auto& animations = player->animations;
    runtime::PoleInput input;
    input.dt = dt;
    input.facing = player->motion.facing;
    input.stickActive = intent.active;
    input.stickHeading = intent.heading;
    input.cameraYaw = pole.cameraYaw;
    input.aPressed = pole.usePressed;
    input.bPressed = jumpPressed;
    input.clips.up = pole.up ? &*pole.up : nullptr;
    input.clips.down = pole.down ? &*pole.down : nullptr;
    input.clips.dismount = pole.dismount ? &*pole.dismount : nullptr;
    input.clips.upFramesPerSecond = assets::animationFramesPerSecond(animations.at(runtime::kAnimationPoleUp));
    input.clips.downFramesPerSecond = assets::animationFramesPerSecond(animations.at(runtime::kAnimationPoleDown));
    input.clips.dismountFramesPerSecond = assets::animationFramesPerSecond(animations.at(runtime::kAnimationPoleDismount));
    input.clips.upFrames = assets::animationLength(animations.at(runtime::kAnimationPoleUp));
    input.clips.downFrames = assets::animationLength(animations.at(runtime::kAnimationPoleDown));
    input.clips.dismountFrames = assets::animationLength(animations.at(runtime::kAnimationPoleDismount));
    const auto probeAt = [&](const float position[3], float reach) {
        runtime::GroundProbe probe;
        probe.x = position[0];
        probe.y = position[1];
        probe.z = position[2];
        probe.forwardX = std::sin(runtime::headingRadians(player->motion.facing));
        probe.forwardZ = std::cos(runtime::headingRadians(player->motion.facing));
        probe.dt = 0;
        probe.distance = reach;
        return runtime::groundHeight(collision, probe);
    };
    input.floorBelow = [&](const float position[3], float length) { return probeAt(position, length).has_value(); };
    input.floorHeight = [&](const float position[3], float reach) { return probeAt(position, reach); };
    const auto disabled = [this](const runtime::PoleObject& object) { return objectState.isDisabled(object.instance); };
    const auto readPlayer = [&]() {
        input.position[0] = player->motion.x;
        input.position[1] = player->vertical.y;
        input.position[2] = player->motion.z;
        input.facing = player->motion.facing;
    };
    const auto apply = [&](const runtime::PoleStep& step) -> game_player::PlayerAnimation {
        player->motion.x = step.position[0];
        player->vertical.y = step.position[1];
        player->motion.z = step.position[2];
        player->motion.facing = player->motion.lastIntent = step.facing;
        player->vertical.fallSpeed = 0;
        player->vertical.fallDistance = 0;
        switch (step.exit) {
        case runtime::PoleExit::None:
            player->vertical.posture = runtime::Posture::Ground;
            player->vertical.jump = nullptr;
            return {step.animation, 1.0f};
        case runtime::PoleExit::Fall:
            std::cout << "pole: fall at update " << updates << '\n';
            player->vertical.posture = runtime::Posture::Falling;
            player->vertical.jump = nullptr;
            return {runtime::kAnimationFall, 1.0f};
        case runtime::PoleExit::Jump:
            std::cout << "pole: jump at update " << updates << '\n';
            player->vertical.posture = runtime::Posture::Jumping;
            player->vertical.jump = &player->jumpStanding;
            player->vertical.jumpElapsed = 0;
            player->vertical.jumpBase = player->vertical.y;
            player->jumpMovingAnimation = false;
            return {runtime::kAnimationJump, 1.0f};
        case runtime::PoleExit::Ground:
            std::cout << "pole: dismounted at update " << updates << '\n';
            player->vertical.posture = runtime::Posture::Ground;
            player->vertical.jump = nullptr;
            return {0, 1.0f};
        }
        return {0, 1.0f};
    };
    readPlayer();
    if (pole.climber.active()) return apply(runtime::poleUpdate(pole.climber, pole.poles, input));
    if (player->vertical.posture == runtime::Posture::Ground && pole.usePressed) {
        const int used = runtime::poleUseTest(pole.poles, input.position, input.facing, disabled);
        if (used >= 0) {
            std::cout << "pole: use " << pole.poles[std::size_t(used)].instance << " at update " << updates << '\n';
            return apply(runtime::poleMount(pole.climber, pole.poles, used, input, true));
        }
    }
    const game_player::PlayerAnimation shown = movePlayerWithVault(intent, jumpPressed, dt);
    if (player->vertical.posture != runtime::Posture::Ground || ziplineRider.active()) return shown;
    readPlayer();
    const int touched = runtime::poleWalkInTest(pole.poles, input.position, input.facing, intent.active, intent.heading, disabled);
    if (touched < 0) return shown;
    std::cout << "pole: walk-in " << pole.poles[std::size_t(touched)].instance << " at update " << updates << '\n';
    return apply(runtime::poleMount(pole.climber, pole.poles, touched, input, false));
}

}
