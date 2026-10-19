#include "game_level.hpp"
#include <iostream>

namespace game_level {
using namespace chase;

namespace {
bool hasLadderClips(const game_player::PlayerRig& player) {
    for (const int role : {runtime::kAnimationLadderIdle, runtime::kAnimationLadderUp, runtime::kAnimationLadderDown, runtime::kAnimationLadderDismount})
        if (!player.animations.count(role)) return false;
    return true;
}
}

// The ladder states around the pole step: the A press and the walk-in (docs/re/player-traversal-ladders.md) start the mount, the ladder
// states own the player while they run.
game_player::PlayerAnimation LevelSession::movePlayerWithLadder(const runtime::StickIntent& intent, bool jumpPressed, float dt) {
    if (ladder.ladders.empty() || !hasLadderClips(*player) || ziplineRider.active() || pole.climber.active()) return movePlayerWithPole(intent, jumpPressed, dt);
    const assets::Animation& clamberClip = player->animations.at(runtime::kAnimationLadderDismount);
    if (!ladder.clamber) ladder.clamber = assets::rootMotion(clamberClip);
    runtime::LadderInput input;
    input.dt = dt;
    input.stickActive = intent.active;
    input.stickHeading = intent.heading;
    input.cameraYaw = pole.cameraYaw;
    input.aPressed = pole.usePressed;
    input.bPressed = jumpPressed;
    input.clamber = ladder.clamber ? &*ladder.clamber : nullptr;
    input.clamberRise = ladder.clamber && !ladder.clamber->translation.empty() ? ladder.clamber->translation.back()[1] : 0.0f;
    input.clamberFramesPerSecond = assets::animationFramesPerSecond(clamberClip);
    input.clamberFrames = assets::animationLength(clamberClip);
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
    const auto disabled = [this](const runtime::LadderObject& object) { return objectState.isDisabled(object.instance); };
    const auto readPlayer = [&]() {
        input.position[0] = player->motion.x;
        input.position[1] = player->vertical.y;
        input.position[2] = player->motion.z;
        input.facing = player->motion.facing;
    };
    const auto apply = [&](const runtime::LadderStep& step) -> game_player::PlayerAnimation {
        player->motion.x = step.position[0];
        player->vertical.y = step.position[1];
        player->motion.z = step.position[2];
        player->motion.facing = step.facing;
        player->motion.lastIntent = step.targetHeading;
        player->vertical.fallSpeed = 0;
        player->vertical.fallDistance = 0;
        player->vertical.jump = nullptr;
        switch (step.exit) {
        case runtime::LadderExit::None:
            player->vertical.posture = runtime::Posture::Ground;
            return {step.animation, 1.0f};
        case runtime::LadderExit::Fall:
            std::cout << "ladder: fall at update " << updates << '\n';
            player->vertical.posture = runtime::Posture::Falling;
            return {runtime::kAnimationFall, 1.0f};
        case runtime::LadderExit::Jump:
            std::cout << "ladder: jump at update " << updates << '\n';
            player->vertical.posture = runtime::Posture::Jumping;
            player->vertical.jump = &player->jumpStanding;
            player->vertical.jumpElapsed = 0;
            player->vertical.jumpBase = player->vertical.y;
            player->jumpMovingAnimation = false;
            return {runtime::kAnimationJump, 1.0f};
        case runtime::LadderExit::Ground:
            std::cout << "ladder: clambered off at update " << updates << '\n';
            player->vertical.posture = runtime::Posture::Ground;
            return {0, 1.0f};
        }
        return {0, 1.0f};
    };
    const auto mount = [&](int index, bool byUse) {
        std::cout << "ladder: " << (byUse ? "use " : "walk-in ") << ladder.ladders[std::size_t(index)].instance << " at update " << updates << '\n';
        return apply(runtime::ladderMount(ladder.climber, ladder.ladders, index, input, byUse));
    };
    readPlayer();
    if (ladder.climber.active()) {
        const runtime::LadderPhase before = ladder.climber.phase;
        const game_player::PlayerAnimation shown = apply(runtime::ladderUpdate(ladder.climber, ladder.ladders, input));
        if (before != runtime::LadderPhase::Clamber && ladder.climber.phase == runtime::LadderPhase::Clamber)
            std::cout << "ladder: clamber at update " << updates << '\n';
        return shown;
    }
    // TRAV-S34: the A press offers the ladders before the poles and the ziplines (the registry 0x1fef10 walks one list in creation order).
    if (player->vertical.posture == runtime::Posture::Ground && pole.usePressed) {
        const int used = runtime::ladderUseTest(ladder.ladders, input.position, input.facing, input.clamberRise, disabled);
        if (used >= 0) return mount(used, true);
    }
    const game_player::PlayerAnimation shown = movePlayerWithPole(intent, jumpPressed, dt);
    // TRAV-S34: the gate 0x1ff208 is taken as "carrying nothing" (DAT_00377162 not read).
    if (player->vertical.posture != runtime::Posture::Ground || ziplineRider.active() || pole.climber.active() || carry.carryingMode() != 0) return shown;
    readPlayer();
    const int touched = runtime::ladderWalkInTest(ladder.ladders, input.position, input.facing, input.clamberRise, disabled);
    return touched < 0 ? shown : mount(touched, false);
}

}
